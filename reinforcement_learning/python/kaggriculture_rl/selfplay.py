"""Recurrent PPO building blocks for exact-engine self-play."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any

import numpy as np
import torch
from torch import Tensor, nn

from . import schema
from .league import pfsp_probabilities
from .model import ActorState
from .policy import act_batch, evaluate_actions, joint_engine_arrays
from .progress import Progress


def _symlog_numpy(value: np.ndarray) -> np.ndarray:
    value = value.astype(np.float32, copy=False)
    return np.sign(value) * np.log1p(np.abs(value))


def central_features(learner: dict[str, np.ndarray],
                     opponent: dict[str, np.ndarray]) -> np.ndarray:
    """Compact privileged critic input; never passed through the actor."""
    tiles = learner["tiles"].astype(np.float32, copy=True)
    tiles[..., 0] /= 5.0
    tiles[..., 1] /= max(1, schema.N_ITEMS - 1)
    tiles[..., 7:13] = _symlog_numpy(tiles[..., 7:13])
    positions = learner["positions"].astype(np.float32) / (schema.BOARD - 1)
    clock = learner["clock"].astype(np.float32) / np.asarray(
        (720.0, 30.0, 24.0, 1.0), dtype=np.float32)
    parts = (
        tiles.reshape(len(tiles), -1),
        positions.reshape(len(tiles), -1),
        learner["active_units"].astype(np.float32).reshape(len(tiles), -1),
        _symlog_numpy(learner["farm"]).reshape(len(tiles), -1),
        _symlog_numpy(learner["market"]).reshape(len(tiles), -1),
        learner["shops"].astype(np.float32).reshape(len(tiles), -1),
        clock,
        _symlog_numpy(learner["own_shed"]),
        _symlog_numpy(learner["own_seeds"]),
        _symlog_numpy(learner["own_inventory"].sum(axis=1)),
        _symlog_numpy(opponent["own_shed"]),
        _symlog_numpy(opponent["own_seeds"]),
        _symlog_numpy(opponent["own_inventory"].sum(axis=1)),
    )
    return np.concatenate(parts, axis=-1).astype(np.float32, copy=False)


class CentralCritic(nn.Module):
    """Training-only critic with both players' private inventory summaries."""

    def __init__(self, input_width: int, width: int = 512):
        super().__init__()
        self.input_width = int(input_width)
        self.width = int(width)
        self.network = nn.Sequential(
            nn.LayerNorm(input_width),
            nn.Linear(input_width, width),
            nn.SiLU(),
            nn.Linear(width, width // 2),
            nn.SiLU(),
            nn.Linear(width // 2, 1),
        )

    def forward(self, features: Tensor) -> Tensor:
        return self.network(features.float()).squeeze(-1)


@dataclass
class OpponentEntry:
    name: str
    checkpoint: str
    games: int = 0
    score: float = 0.0
    frozen_parent: bool = True

    @property
    def win_rate(self) -> float:
        return (self.score + 1.0) / (self.games + 2.0)


class OpponentPool:
    def __init__(self, entries: list[OpponentEntry]):
        if not entries:
            raise ValueError("opponent pool cannot be empty")
        self.entries = entries

    def probabilities(self) -> np.ndarray:
        rates = [entry.win_rate for entry in self.entries]
        competitive = pfsp_probabilities(rates, mode="variance", floor=0.02)
        exploiters = pfsp_probabilities(rates, mode="squared", floor=0.02)
        return 0.5 * competitive + 0.5 * exploiters

    def sample(self, rng: np.random.Generator) -> OpponentEntry:
        return self.entries[int(rng.choice(len(self.entries), p=self.probabilities()))]

    def record(self, name: str, outcomes: np.ndarray) -> None:
        entry = next(row for row in self.entries if row.name == name)
        entry.games += int(len(outcomes))
        entry.score += float(((outcomes + 1.0) * 0.5).sum())

    def add_snapshot(self, name: str, checkpoint: Path,
                     max_snapshots: int) -> list[OpponentEntry]:
        self.entries.append(OpponentEntry(name, str(checkpoint), frozen_parent=False))
        snapshots = [row for row in self.entries if not row.frozen_parent]
        removed = []
        while len(snapshots) > max_snapshots:
            victim = snapshots.pop(0)
            self.entries.remove(victim)
            removed.append(victim)
        return removed

    def state(self) -> list[dict[str, Any]]:
        return [entry.__dict__.copy() for entry in self.entries]

    @classmethod
    def restore(cls, rows: list[dict[str, Any]]) -> "OpponentPool":
        return cls([OpponentEntry(**row) for row in rows])


@dataclass
class Rollout:
    observations: dict[str, np.ndarray]
    critic_features: np.ndarray
    actions: dict[str, np.ndarray]
    masks: dict[str, np.ndarray]
    old_log_prob: np.ndarray
    old_value: np.ndarray
    hourly_state: np.ndarray
    daily_state: np.ndarray
    joint_actions: dict[str, np.ndarray]
    parity_hashes: np.ndarray
    outcomes: np.ndarray
    money: np.ndarray
    learner_seat: int

    @property
    def steps(self) -> int:
        return int(self.old_log_prob.shape[0])

    @property
    def environments(self) -> int:
        return int(self.old_log_prob.shape[1])


def _stack(rows: dict[str, list[np.ndarray]]) -> dict[str, np.ndarray]:
    return {name: np.stack(values) for name, values in rows.items()}


def _checkpoint_model(checkpoint: str | Path, model, device: torch.device) -> None:
    row = torch.load(checkpoint, map_location=device, weights_only=False)
    model.load_state_dict(row["model"])


def collect_rollout(actor, opponent, critic, environment, *, learner_seat: int,
                    device: torch.device, rng: np.random.Generator,
                    temperature: float, price_function,
                    episode_steps: int = 720) -> Rollout:
    """Collect complete games; only the learner's sampled actions enter PPO."""
    if learner_seat not in (0, 1):
        raise ValueError("learner_seat must be zero or one")
    actor.eval()
    opponent.eval()
    critic.eval()
    actor_state = opponent_state = None
    observation_rows: dict[str, list[np.ndarray]] = {}
    action_rows: dict[str, list[np.ndarray]] = {}
    mask_rows: dict[str, list[np.ndarray]] = {}
    features_rows, log_prob_rows, value_rows = [], [], []
    hourly_rows, daily_rows = [], []
    joint_action_rows: dict[str, list[np.ndarray]] = {}
    parity_hash_rows = [np.asarray(environment.status()["parity_hash"]).copy()]
    progress = Progress("self-play rollout", episode_steps - 1)
    for step in range(episode_steps - 1):
        learner_obs = environment.observe(learner_seat)
        opponent_obs = environment.observe(1 - learner_seat)
        # A configurable short engine reports done earlier; stop before asking
        # it for actions on terminal states.
        status_before = environment.status()
        if np.asarray(status_before["done"]).all():
            break
        for name, value in learner_obs.items():
            observation_rows.setdefault(name, []).append(np.array(value, copy=True))
        features = central_features(learner_obs, opponent_obs)
        features_rows.append(features.astype(np.float16))
        batch = len(features)
        if actor_state is None:
            hourly_rows.append(np.zeros((batch, actor.config.memory_width), dtype=np.float32))
            daily_rows.append(np.zeros((batch, actor.config.memory_width), dtype=np.float32))
        else:
            hourly_rows.append(actor_state.hourly.detach().float().cpu().numpy())
            daily_rows.append(actor_state.daily.detach().float().cpu().numpy())
        with torch.no_grad():
            values = critic(torch.from_numpy(features).to(device)).cpu().numpy()
        value_rows.append(values.astype(np.float32))
        learner_policy = act_batch(
            actor, learner_obs, actor_state, device=device, rng=rng,
            deterministic=False, temperature=temperature,
            price_function=price_function)
        opponent_policy = act_batch(
            opponent, opponent_obs, opponent_state, device=device, rng=rng,
            deterministic=True, temperature=1.0, price_function=price_function)
        actor_state, opponent_state = learner_policy.state, opponent_policy.state
        for name, value in learner_policy.actions.arrays().items():
            action_rows.setdefault(name, []).append(value)
        for name, value in learner_policy.masks.arrays().items():
            mask_rows.setdefault(name, []).append(value)
        log_prob_rows.append(learner_policy.log_prob)
        first, second = ((learner_policy.actions, opponent_policy.actions)
                         if learner_seat == 0
                         else (opponent_policy.actions, learner_policy.actions))
        engine_arrays = joint_engine_arrays(first, second)
        for name, value in zip(
                ("unit_op", "unit_arg", "unit_n", "n_units",
                 "order_op", "order_item", "order_n", "n_orders"),
                engine_arrays):
            joint_action_rows.setdefault(name, []).append(np.array(value, copy=True))
        status = environment.step(*engine_arrays)
        parity_hash_rows.append(np.asarray(status["parity_hash"]).copy())
        progress.update(step + 1)
        if np.asarray(status["done"]).all():
            break
    if not log_prob_rows:
        raise RuntimeError("self-play rollout produced no decisions")
    status = environment.status()
    done = np.asarray(status["done"]).astype(bool)
    if not done.all():
        raise RuntimeError("rollout stopped before every environment reached terminal state")
    money = np.asarray(status["money"], dtype=np.float32)
    delta = money[:, learner_seat] - money[:, 1 - learner_seat]
    outcomes = np.sign(delta).astype(np.float32)
    return Rollout(
        _stack(observation_rows),
        np.stack(features_rows),
        _stack(action_rows),
        _stack(mask_rows),
        np.stack(log_prob_rows),
        np.stack(value_rows),
        np.stack(hourly_rows),
        np.stack(daily_rows),
        _stack(joint_action_rows),
        np.stack(parity_hash_rows),
        outcomes,
        money,
        learner_seat,
    )


def _sequence_starts(steps: int, length: int) -> list[int]:
    if length > steps:
        raise ValueError("sequence length exceeds rollout")
    starts = list(range(0, steps - length + 1, length))
    final = steps - length
    if starts[-1] != final:
        starts.append(final)
    return starts


def ppo_update(actor, critic, actor_optimizer, critic_optimizer, rollout: Rollout,
               *, device: torch.device, rng: np.random.Generator,
               sequence_length: int, sequences_per_minibatch: int,
               epochs: int, clip_ratio: float, value_coefficient: float,
               entropy_coefficient: float, max_grad_norm: float,
               temperature: float, target_kl: float = 0.01) -> dict[str, float]:
    """Recurrent PPO over fixed stored hidden states and exact behavior masks."""
    actor.train()
    critic.train()
    returns = np.broadcast_to(
        rollout.outcomes[None], rollout.old_value.shape).astype(np.float32).copy()
    advantages = returns - rollout.old_value
    advantages = (advantages - advantages.mean()) / (advantages.std() + 1e-6)
    sequences = [(env, start) for env in range(rollout.environments)
                 for start in _sequence_starts(rollout.steps, sequence_length)]
    records = []
    progress = Progress("PPO minibatches",
                        epochs * ((len(sequences) + sequences_per_minibatch - 1)
                                  // sequences_per_minibatch))
    completed = 0
    stopped_early = False
    for _epoch in range(epochs):
        rng.shuffle(sequences)
        for offset in range(0, len(sequences), sequences_per_minibatch):
            rows = sequences[offset:offset + sequences_per_minibatch]
            hourly = torch.from_numpy(np.stack([
                rollout.hourly_state[start, env] for env, start in rows
            ]).astype(np.float32)).to(device)
            daily = torch.from_numpy(np.stack([
                rollout.daily_state[start, env] for env, start in rows
            ]).astype(np.float32)).to(device)
            state = ActorState(hourly, daily)
            new_log_probs, entropies, values = [], [], []
            for time_index in range(sequence_length):
                observations = {
                    name: torch.from_numpy(np.stack([
                        value[start + time_index, env] for env, start in rows
                    ])).to(device)
                    for name, value in rollout.observations.items()
                }
                actions = {
                    name: torch.from_numpy(np.stack([
                        value[start + time_index, env] for env, start in rows
                    ])).to(device)
                    for name, value in rollout.actions.items()
                }
                masks = {
                    name: torch.from_numpy(np.stack([
                        value[start + time_index, env] for env, start in rows
                    ])).to(device)
                    for name, value in rollout.masks.items()
                }
                features = torch.from_numpy(np.stack([
                    rollout.critic_features[start + time_index, env]
                    for env, start in rows
                ]).astype(np.float32)).to(device)
                with torch.autocast(device_type=device.type, dtype=torch.bfloat16,
                                    enabled=device.type == "cuda"):
                    log_prob, entropy, state = evaluate_actions(
                        actor, observations, state, actions, masks,
                        temperature=temperature)
                    value = critic(features)
                new_log_probs.append(log_prob.float())
                entropies.append(entropy.float())
                values.append(value.float())
            new_log_prob = torch.stack(new_log_probs)
            entropy = torch.stack(entropies)
            value = torch.stack(values)
            old_log_prob = torch.from_numpy(np.stack([
                rollout.old_log_prob[start:start + sequence_length, env]
                for env, start in rows
            ], axis=1)).to(device)
            target = torch.from_numpy(np.stack([
                returns[start:start + sequence_length, env] for env, start in rows
            ], axis=1)).to(device)
            advantage = torch.from_numpy(np.stack([
                advantages[start:start + sequence_length, env] for env, start in rows
            ], axis=1)).to(device)
            log_ratio = new_log_prob - old_log_prob
            if _epoch == 0 and offset == 0:
                mismatch = float(log_ratio.detach().abs().max())
                if mismatch > 0.05:
                    raise RuntimeError(
                        f"rollout/PPO log-probability mismatch {mismatch:.6f}")
            approximate_kl = float((old_log_prob - new_log_prob).mean().detach())
            if completed and approximate_kl > 1.5 * target_kl:
                print(f"[PPO early stop] approximate_kl={approximate_kl:.5f} "
                      f"limit={1.5 * target_kl:.5f}", flush=True)
                stopped_early = True
                break
            ratio = torch.exp(log_ratio.clamp(-20.0, 20.0))
            unclipped = ratio * advantage
            clipped = ratio.clamp(1.0 - clip_ratio, 1.0 + clip_ratio) * advantage
            policy_loss = -torch.minimum(unclipped, clipped).mean()
            value_loss = 0.5 * (value - target).square().mean()
            entropy_mean = entropy.mean()
            loss = (policy_loss + value_coefficient * value_loss
                    - entropy_coefficient * entropy_mean)
            if not torch.isfinite(loss):
                raise FloatingPointError("non-finite PPO loss")
            actor_optimizer.zero_grad(set_to_none=True)
            critic_optimizer.zero_grad(set_to_none=True)
            loss.backward()
            actor_grad = float(nn.utils.clip_grad_norm_(actor.parameters(), max_grad_norm))
            critic_grad = float(nn.utils.clip_grad_norm_(critic.parameters(), max_grad_norm))
            if not np.isfinite(actor_grad) or not np.isfinite(critic_grad):
                raise FloatingPointError("non-finite PPO gradient")
            actor_optimizer.step()
            critic_optimizer.step()
            clip_fraction = float(
                ((ratio - 1.0).abs() > clip_ratio).float().mean().detach())
            records.append({
                "loss": float(loss.detach()), "policy_loss": float(policy_loss.detach()),
                "value_loss": float(value_loss.detach()), "entropy": float(entropy_mean.detach()),
                "approximate_kl": approximate_kl, "clip_fraction": clip_fraction,
                "actor_grad_norm": actor_grad, "critic_grad_norm": critic_grad,
            })
            completed += 1
            progress.update(completed, detail=(
                f"loss={records[-1]['loss']:.4f} kl={approximate_kl:+.5f} "
                f"clip={clip_fraction:.3f}"))
        if stopped_early:
            break
    result = {name: float(np.mean([row[name] for row in records]))
              for name in records[0]}
    result["minibatches_completed"] = completed
    result["early_stopped"] = stopped_early
    return result


@torch.inference_mode()
def evaluate_against_parents(actor, opponent_model, parent_entries,
                             model_loader, *, seeds: list[int], episode_steps: int,
                             device: torch.device, rng: np.random.Generator,
                             environment_type, price_function) -> dict[str, Any]:
    """Deterministic common-seed, seat-swapped parent evaluation."""
    actor.eval()
    games = []
    total_steps = len(parent_entries) * 2 * (episode_steps - 1)
    progress = Progress("self-play evaluation", total_steps)
    completed = 0
    for entry in parent_entries:
        model_loader(entry.checkpoint, opponent_model, device)
        opponent_model.eval()
        for learner_seat in (0, 1):
            environment = environment_type(seeds, episode_steps=episode_steps)
            actor_state = opponent_state = None
            status = None
            for _step in range(episode_steps - 1):
                learner = act_batch(
                    actor, environment.observe(learner_seat), actor_state,
                    device=device, rng=rng, deterministic=True,
                    price_function=price_function)
                opponent = act_batch(
                    opponent_model, environment.observe(1 - learner_seat), opponent_state,
                    device=device, rng=rng, deterministic=True,
                    price_function=price_function)
                actor_state, opponent_state = learner.state, opponent.state
                first, second = ((learner.actions, opponent.actions)
                                 if learner_seat == 0 else
                                 (opponent.actions, learner.actions))
                status = environment.step(*joint_engine_arrays(first, second))
                completed += 1
                progress.update(completed, detail=entry.name)
                if np.asarray(status["done"]).all():
                    break
            money = np.asarray(status["money"], dtype=np.float32)
            delta = money[:, learner_seat] - money[:, 1 - learner_seat]
            for seed, margin in zip(seeds, delta):
                games.append({
                    "opponent": entry.name, "learner_seat": learner_seat,
                    "seed": int(seed), "margin": float(margin),
                    "score": 1.0 if margin > 0 else 0.5 if margin == 0 else 0.0,
                })
    return {
        "score": float(np.mean([row["score"] for row in games])),
        "mean_money_margin": float(np.mean([row["margin"] for row in games])),
        "games": len(games),
        "records": games,
    }


__all__ = [
    "CentralCritic", "OpponentEntry", "OpponentPool", "Rollout",
    "central_features", "collect_rollout", "evaluate_against_parents",
    "ppo_update",
]
