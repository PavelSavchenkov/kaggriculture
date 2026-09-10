#!/usr/bin/env python3
"""Train one independent recurrent PPO descendant in an exact-engine league."""

from __future__ import annotations

import argparse
from dataclasses import asdict
import hashlib
import json
from pathlib import Path
import sys
import time

import numpy as np
import torch

from kaggriculture_rl import (
    KaggricultureActor, ModelConfig, Progress, ReplayCursor, VectorEnv, market_price,
)
from kaggriculture_rl.selfplay import (
    CentralCritic,
    OpponentEntry,
    OpponentPool,
    central_features,
    collect_rollout,
    evaluate_against_parents,
    ppo_update,
)
from kaggriculture_rl.action_codec import ITEM_NAMES, MARKET_NAMES, UNIT_NAMES


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "corpus"))
sys.path.insert(0, str(ROOT / "fast_game_engine"))
import kagz_format  # noqa: E402
import export_trace  # noqa: E402


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def atomic_json(path: Path, value) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2) + "\n")
    temporary.replace(path)


def atomic_torch(path: Path, value) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    torch.save(value, temporary)
    temporary.replace(path)


def _action_dict(joint: dict[str, np.ndarray], time_index: int,
                 environment: int, seat: int) -> dict:
    units = []
    count = int(joint["n_units"][time_index, environment, seat])
    for unit in range(count):
        operation = int(joint["unit_op"][time_index, environment, seat, unit])
        item = int(joint["unit_arg"][time_index, environment, seat, unit])
        quantity = int(joint["unit_n"][time_index, environment, seat, unit])
        action = [UNIT_NAMES[operation]]
        if operation in (5, 7):
            action.extend((ITEM_NAMES[item], quantity))
        elif operation == 8:
            action.append(ITEM_NAMES[item])
        units.append(action)
    market = []
    count_orders = int(joint["n_orders"][time_index, environment, seat])
    for order in range(count_orders):
        operation = int(joint["order_op"][time_index, environment, seat, order])
        item = int(joint["order_item"][time_index, environment, seat, order])
        quantity = int(joint["order_n"][time_index, environment, seat, order])
        action = [MARKET_NAMES[operation]]
        if operation in (3, 4, 5, 6):
            action.extend((ITEM_NAMES[item], quantity))
        market.append(action)
    return {
        "farmer": units[0] if units else ["PASS"],
        "hands": units[1:],
        "market": market,
    }


def _hash_chain(values: np.ndarray) -> int:
    result = export_trace.FNV_OFFSET
    for value in values:
        word = int(value)
        for shift in range(0, 64, 8):
            result ^= (word >> shift) & 0xFF
            result = result * export_trace.FNV_PRIME & ((1 << 64) - 1)
    return result


def record_rollout_kagz(rollout, *, output: Path, seeds: list[int],
                        learner: str, opponent: str, update: int,
                        episode_steps: int) -> list[str]:
    """Persist exact self-play requests plus state-hash certificates."""
    directory = output / "rollouts" / f"update_{update:06d}"
    directory.mkdir(parents=True, exist_ok=True)
    config = {
        "episodeSteps": episode_steps,
        "boardSize": 10,
        "startingMoney": 3000,
        "maxMarketOrdersPerTurn": 10,
        "turnsPerDay": 24,
        "shedCapacity": 100,
        "weedSpawnChance": 0.005,
        "townShopUnlockInterval": 3,
        "townShopSellInterval": 4,
        "townCenterSellInterval": 24,
        "farmHandCostMult": 1,
        "actTimeout": 1,
        "runTimeout": 1200,
    }
    teams = ([learner, opponent] if rollout.learner_seat == 0
             else [opponent, learner])
    progress = Progress("record self-play kagz", rollout.environments)
    paths = []
    for environment, seed in enumerate(seeds):
        actions = [[
            {"farmer": ["PASS"], "hands": [], "market": []},
            {"farmer": ["PASS"], "hands": [], "market": []},
        ]]
        for time_index in range(rollout.steps):
            actions.append([
                _action_dict(rollout.joint_actions, time_index, environment, 0),
                _action_dict(rollout.joint_actions, time_index, environment, 1),
            ])
        identity = f"{learner}|{opponent}|{update}|{seed}|{rollout.learner_seat}"
        episode_id = int(hashlib.sha256(identity.encode()).hexdigest()[:16], 16)
        path = directory / f"{episode_id}.kagz"
        temporary = path.with_suffix(".kagz.tmp")
        hashes = rollout.parity_hashes[:, environment]
        extra = {
            "source": "selfplay",
            "schema_version": 1,
            "name": "kaggriculture",
            "title": "Kaggriculture self-play",
            "version": "0.1.0",
            "description": "",
            "specification": {},
            "configuration": config,
            "id": episode_id,
            "ancestry": {
                "learner": learner,
                "opponent": opponent,
                "update": update,
                "learner_seat": rollout.learner_seat,
            },
            "info": {
                "TeamNames": teams,
                "seed": int(seed),
                "EpisodeId": episode_id,
            },
        }
        kagz_format.write(
            temporary, episode_id=episode_id, seed=int(seed), config=config,
            teams=teams, statuses=["DONE", "DONE"],
            rewards=rollout.money[environment].tolist(), actions=actions,
            engine_version=export_trace.ENGINE_VERSION,
            engine_sha256=export_trace.ENGINE_SOURCE_HASH,
            anchors=list(enumerate(map(int, hashes))),
            chain_hash=_hash_chain(hashes),
            extra=json.dumps(extra, sort_keys=True))
        if not ReplayCursor(str(temporary)).verify():
            temporary.unlink(missing_ok=True)
            raise RuntimeError(f"generated replay certificate failed: {path}")
        temporary.replace(path)
        paths.append(str(path.resolve()))
        progress.update(environment + 1, detail=path.name)
    return paths


def exported_parents(manifest_path: Path, lb_root: Path):
    manifest = json.loads(manifest_path.read_text())
    rows = []
    for agent_id in manifest.get("accepted", []):
        export_path = lb_root / "agents" / agent_id / "export_manifest.json"
        if not export_path.is_file():
            raise FileNotFoundError(f"missing export manifest for promoted agent: {export_path}")
        export = json.loads(export_path.read_text())
        checkpoint = Path(export["source_checkpoint"])
        if not checkpoint.is_file():
            raise FileNotFoundError(f"missing source checkpoint: {checkpoint}")
        rows.append({
            "agent_id": agent_id,
            "cluster": export["cluster"],
            "checkpoint": checkpoint.resolve(),
        })
    if not rows:
        raise ValueError("promotion manifest contains no accepted agents")
    return rows


def load_actor_checkpoint(path: str | Path, model, device: torch.device,
                          expected_config: dict | None = None):
    checkpoint = torch.load(path, map_location=device, weights_only=False)
    if expected_config is not None and checkpoint["model_config"] != expected_config:
        raise ValueError(f"model configuration differs in opponent checkpoint {path}")
    model.load_state_dict(checkpoint["model"])
    return checkpoint


def save_snapshot(path: Path, actor, config: ModelConfig, cluster: str, update: int):
    atomic_torch(path, {
        "format": 2,
        "model": {name: value.detach().cpu() for name, value in actor.state_dict().items()},
        "model_config": asdict(config),
        "cluster": cluster,
        "members": [],
        "recipe": {"stage": "selfplay_snapshot"},
        "history": [],
        "seed": 0,
        "completed_updates": update,
    })


def save_training_state(path: Path, *, actor, critic, actor_optimizer,
                        critic_optimizer, config, cluster, completed_updates,
                        history, pool, rng, recipe, best):
    atomic_torch(path, {
        "format": 3,
        "stage": "selfplay",
        "model": {name: value.detach().cpu() for name, value in actor.state_dict().items()},
        "model_config": asdict(config),
        "critic": {name: value.detach().cpu() for name, value in critic.state_dict().items()},
        "critic_config": {
            "input_width": critic.input_width,
            "width": critic.width,
        },
        "optimizer": actor_optimizer.state_dict(),
        "critic_optimizer": critic_optimizer.state_dict(),
        "cluster": cluster,
        "completed_updates": completed_updates,
        "history": history,
        "pool": pool.state(),
        "numpy_rng_state": rng.bit_generator.state,
        "torch_rng_state": torch.get_rng_state(),
        "cuda_rng_state": (torch.cuda.get_rng_state_all()
                           if torch.cuda.is_available() else None),
        "recipe": recipe,
        "best": best,
    })


def report(output: Path, *, cluster: str, checkpoint: Path, latest: Path,
           completed: int, history, best, pool, args, config, started):
    value = {
        "purpose": "independent exact-engine recurrent PPO descendant",
        "device": args.device,
        "model_config": asdict(config),
        "clusters": {
            cluster: {
                "cluster": cluster,
                "checkpoint": str(checkpoint.resolve()),
                "final_checkpoint": str(latest.resolve()),
                "completed_updates": completed,
                "best_evaluation": best,
            }
        },
        "history": history,
        "opponent_pool": pool.state(),
        "elapsed_seconds": time.time() - started,
        "arguments": {
            key: str(value) if isinstance(value, Path) else value
            for key, value in vars(args).items()
        },
    }
    atomic_json(output / "training_report.json", value)
    return value


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--promotion-manifest", type=Path, required=True)
    parser.add_argument("--lb-root", type=Path,
                        default=Path.home() / "Kaggle/kaggriculture-localLB")
    parser.add_argument("--learner-agent", required=True,
                        help="one accepted LocalLB agent ID from the promotion manifest")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--updates", type=int, default=100)
    parser.add_argument("--environments", type=int, default=32)
    parser.add_argument("--episode-steps", type=int, default=720)
    parser.add_argument("--sequence-length", type=int, default=24)
    parser.add_argument("--sequences-per-minibatch", type=int, default=32)
    parser.add_argument("--ppo-epochs", type=int, default=1)
    parser.add_argument("--actor-learning-rate", type=float, default=1e-6)
    parser.add_argument("--critic-learning-rate", type=float, default=1e-4)
    parser.add_argument("--clip-ratio", type=float, default=0.1)
    parser.add_argument("--value-coefficient", type=float, default=0.5)
    parser.add_argument("--entropy-coefficient", type=float, default=1e-4)
    parser.add_argument("--max-grad-norm", type=float, default=0.5)
    parser.add_argument("--target-kl", type=float, default=0.01)
    parser.add_argument("--temperature", type=float, default=1.0)
    parser.add_argument("--snapshot-every", type=int, default=5)
    parser.add_argument("--max-snapshots", type=int, default=8)
    parser.add_argument("--evaluation-every", type=int, default=5)
    parser.add_argument("--evaluation-seeds", type=int, default=2)
    parser.add_argument("--record-kagz-every", type=int, default=1,
                        help="record every N rollout updates as certified kagz; 0 disables")
    parser.add_argument("--seed", type=int, default=20260910)
    parser.add_argument("--device", choices=("cuda", "cpu"), default="cuda")
    parser.add_argument("--resume", action="store_true")
    args = parser.parse_args()

    positive = (
        "updates", "environments", "sequence_length", "sequences_per_minibatch",
        "ppo_epochs", "snapshot_every", "max_snapshots", "evaluation_every",
        "evaluation_seeds",
    )
    for name in positive:
        if getattr(args, name) < 1:
            raise SystemExit(f"--{name.replace('_', '-')} must be positive")
    if not 2 <= args.episode_steps <= 720:
        raise SystemExit("--episode-steps must be in [2, 720]")
    if args.sequence_length > args.episode_steps - 1:
        raise SystemExit("--sequence-length cannot exceed the decision horizon")
    if args.temperature <= 0:
        raise SystemExit("--temperature must be positive")
    if args.target_kl <= 0:
        raise SystemExit("--target-kl must be positive")
    if args.record_kagz_every < 0:
        raise SystemExit("--record-kagz-every cannot be negative")
    if args.device == "cuda" and not torch.cuda.is_available():
        raise SystemExit("CUDA requested but unavailable")
    device = torch.device(args.device)

    parents = exported_parents(args.promotion_manifest, args.lb_root)
    by_id = {row["agent_id"]: row for row in parents}
    if args.learner_agent not in by_id:
        raise SystemExit(f"--learner-agent is not promoted: {args.learner_agent}")
    parent = by_id[args.learner_agent]
    parent_checkpoint = torch.load(
        parent["checkpoint"], map_location="cpu", weights_only=False)
    config_dict = parent_checkpoint["model_config"]
    for row in parents:
        candidate = torch.load(row["checkpoint"], map_location="cpu", weights_only=False)
        if candidate["model_config"] != config_dict:
            raise ValueError("all promoted parents must use one actor configuration")
    config = ModelConfig(**config_dict)
    cluster = f"selfplay descendant of {parent['cluster']}"
    args.output.mkdir(parents=True, exist_ok=True)
    latest_path = args.output / "latest.pt"
    best_path = args.output / "best.pt"
    snapshots = args.output / "opponents"
    snapshots.mkdir(exist_ok=True)
    if latest_path.exists() and not args.resume:
        raise SystemExit(f"refusing to overwrite {latest_path}; pass --resume")

    rng = np.random.default_rng(args.seed)
    torch.manual_seed(args.seed)
    if device.type == "cuda":
        torch.cuda.manual_seed_all(args.seed)
    actor = KaggricultureActor(config).to(device)
    opponent_model = KaggricultureActor(config).to(device)
    load_actor_checkpoint(parent["checkpoint"], actor, device, config_dict)
    sample_environment = VectorEnv([args.seed], episode_steps=args.episode_steps)
    feature_width = central_features(
        sample_environment.observe(0), sample_environment.observe(1)).shape[1]
    critic = CentralCritic(feature_width).to(device)
    actor_optimizer = torch.optim.AdamW(
        actor.parameters(), lr=args.actor_learning_rate, weight_decay=1e-5)
    critic_optimizer = torch.optim.AdamW(
        critic.parameters(), lr=args.critic_learning_rate, weight_decay=1e-5)
    pool = OpponentPool([
        OpponentEntry(row["agent_id"], str(row["checkpoint"]), frozen_parent=True)
        for row in parents
    ])
    history = []
    completed = 0
    best = {"score": -1.0, "mean_money_margin": float("-inf"),
            "update": None, "evaluation": None}
    recipe = {
        "version": 1,
        "learner_agent": args.learner_agent,
        "parent_checkpoint": str(parent["checkpoint"]),
        "parent_sha256": sha256(parent["checkpoint"]),
        "parents": [row["agent_id"] for row in parents],
        "environments": args.environments,
        "episode_steps": args.episode_steps,
        "sequence_length": args.sequence_length,
        "sequences_per_minibatch": args.sequences_per_minibatch,
        "ppo_epochs": args.ppo_epochs,
        "actor_learning_rate": args.actor_learning_rate,
        "critic_learning_rate": args.critic_learning_rate,
        "clip_ratio": args.clip_ratio,
        "value_coefficient": args.value_coefficient,
        "entropy_coefficient": args.entropy_coefficient,
        "max_grad_norm": args.max_grad_norm,
        "target_kl": args.target_kl,
        "temperature": args.temperature,
    }
    if args.resume and latest_path.exists():
        saved = torch.load(latest_path, map_location=device, weights_only=False)
        if saved.get("stage") != "selfplay" or saved.get("recipe") != recipe:
            raise ValueError("self-play recipe differs from resume checkpoint")
        actor.load_state_dict(saved["model"])
        critic.load_state_dict(saved["critic"])
        actor_optimizer.load_state_dict(saved["optimizer"])
        critic_optimizer.load_state_dict(saved["critic_optimizer"])
        completed = int(saved["completed_updates"])
        history = list(saved["history"])
        pool = OpponentPool.restore(saved["pool"])
        rng.bit_generator.state = saved["numpy_rng_state"]
        torch.set_rng_state(saved["torch_rng_state"].cpu())
        if device.type == "cuda" and saved.get("cuda_rng_state") is not None:
            torch.cuda.set_rng_state_all(
                [value.cpu() for value in saved["cuda_rng_state"]])
        best = dict(saved["best"])
        print(f"[self-play resume] update={completed}", flush=True)
    remaining = max(0, args.updates - completed)
    started = time.time()
    parent_entries = [entry for entry in pool.entries if entry.frozen_parent]
    evaluation_seeds = [
        args.seed + 900_000_000 + index
        for index in range(args.evaluation_seeds)
    ]
    if best["update"] is None:
        print("[self-play baseline] evaluating immutable update-zero seed", flush=True)
        baseline = evaluate_against_parents(
            actor, opponent_model, parent_entries, load_actor_checkpoint,
            seeds=evaluation_seeds, episode_steps=args.episode_steps,
            device=device, rng=rng, environment_type=VectorEnv,
            price_function=market_price)
        best = {
            "score": baseline["score"],
            "mean_money_margin": baseline["mean_money_margin"],
            "update": 0,
            "evaluation": baseline,
        }
        save_snapshot(best_path, actor, config, cluster, 0)
        print(f"[self-play baseline] score={best['score']:.3f} "
              f"mean_margin={best['mean_money_margin']:+.1f}", flush=True)
    updates = Progress("self-play updates", remaining)
    for relative in range(1, remaining + 1):
        update = completed + relative
        selected = pool.sample(rng)
        load_actor_checkpoint(selected.checkpoint, opponent_model, device, config_dict)
        seeds = [args.seed + update * 1_000_000 + index
                 for index in range(args.environments)]
        learner_seat = (update - 1) % 2
        environment = VectorEnv(seeds, episode_steps=args.episode_steps)
        rollout = collect_rollout(
            actor, opponent_model, critic, environment,
            learner_seat=learner_seat, device=device, rng=rng,
            temperature=args.temperature, price_function=market_price,
            episode_steps=args.episode_steps)
        recorded_replays = []
        if (args.record_kagz_every and
                update % args.record_kagz_every == 0):
            recorded_replays = record_rollout_kagz(
                rollout, output=args.output, seeds=seeds,
                learner=args.learner_agent, opponent=selected.name,
                update=update, episode_steps=args.episode_steps)
        pool.record(selected.name, rollout.outcomes)
        metrics = ppo_update(
            actor, critic, actor_optimizer, critic_optimizer, rollout,
            device=device, rng=rng, sequence_length=args.sequence_length,
            sequences_per_minibatch=args.sequences_per_minibatch,
            epochs=args.ppo_epochs, clip_ratio=args.clip_ratio,
            value_coefficient=args.value_coefficient,
            entropy_coefficient=args.entropy_coefficient,
            max_grad_norm=args.max_grad_norm, temperature=args.temperature,
            target_kl=args.target_kl)
        row = {
            "update": update,
            "opponent": selected.name,
            "learner_seat": learner_seat,
            "rollout_score": float(((rollout.outcomes + 1.0) * 0.5).mean()),
            "rollout_mean_money_margin": float(np.mean(
                rollout.money[:, learner_seat] -
                rollout.money[:, 1 - learner_seat])),
            "recorded_replays": recorded_replays,
            **metrics,
        }
        if update % args.snapshot_every == 0:
            snapshot_path = snapshots / f"update_{update:06d}.pt"
            save_snapshot(snapshot_path, actor, config, cluster, update)
            removed = pool.add_snapshot(
                f"{args.learner_agent}@{update}",
                snapshot_path.resolve(), args.max_snapshots)
            for stale in removed:
                stale_path = Path(stale.checkpoint)
                if stale_path.parent == snapshots.resolve():
                    stale_path.unlink(missing_ok=True)
        evaluation = None
        if update % args.evaluation_every == 0 or update == args.updates:
            evaluation = evaluate_against_parents(
                actor, opponent_model, parent_entries, load_actor_checkpoint,
                seeds=evaluation_seeds, episode_steps=args.episode_steps,
                device=device, rng=rng, environment_type=VectorEnv,
                price_function=market_price)
            row["evaluation"] = evaluation
            score = (evaluation["score"], evaluation["mean_money_margin"])
            if score > (best["score"], best["mean_money_margin"]):
                best = {
                    "score": score[0],
                    "mean_money_margin": score[1],
                    "update": update,
                    "evaluation": evaluation,
                }
                save_snapshot(best_path, actor, config, cluster, update)
                print(f"[self-play best] update={update} score={score[0]:.3f} "
                      f"mean_margin={score[1]:+.1f}", flush=True)
        history.append(row)
        save_training_state(
            latest_path, actor=actor, critic=critic,
            actor_optimizer=actor_optimizer, critic_optimizer=critic_optimizer,
            config=config, cluster=cluster, completed_updates=update,
            history=history, pool=pool, rng=rng, recipe=recipe, best=best)
        selected_checkpoint = best_path if best_path.exists() else latest_path
        report(
            args.output, cluster=cluster, checkpoint=selected_checkpoint,
            latest=latest_path, completed=update, history=history, best=best,
            pool=pool, args=args, config=config, started=started)
        updates.update(relative, detail=(
            f"score={row['rollout_score']:.3f} opponent={selected.name} "
            f"loss={row['loss']:.4f}"), force=True)
    selected_checkpoint = best_path if best_path.exists() else latest_path
    final = report(
        args.output, cluster=cluster, checkpoint=selected_checkpoint,
        latest=latest_path, completed=completed + remaining, history=history,
        best=best, pool=pool, args=args, config=config, started=started)
    print(json.dumps({
        "learner": args.learner_agent,
        "completed_updates": completed + remaining,
        "checkpoint": str(selected_checkpoint),
        "best": best,
        "report": str((args.output / "training_report.json").resolve()),
    }, indent=2), flush=True)


if __name__ == "__main__":
    main()
