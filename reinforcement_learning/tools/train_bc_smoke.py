#!/usr/bin/env python3
"""Small end-to-end behavior-cloning run over parity-canonicalized replays.

This is a pipeline gate, not a competitive model. It trains unit operations, the
first market-line operation, and terminal W/D/L on independent sampled states.
Sequence training, conditional item/quantity losses, and cluster adapters belong
to the full M2 trainer.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import time

import numpy as np
import torch
from torch import nn

from kaggriculture_rl import KaggricultureActor, Progress, ReplayCursor


def collect(paths: list[Path], stride: int, label: str):
    observations: dict[str, list[np.ndarray]] = {}
    unit_ops: list[np.ndarray] = []
    market_ops: list[int] = []
    outcomes: list[int] = []
    progress = Progress(label, len(paths))
    for index, path in enumerate(paths, 1):
        cursor = ReplayCursor(str(path))
        if not cursor.verify_joint_canonicalization():
            raise RuntimeError(f"joint canonicalization parity failed: {path}")
        while cursor.step + 1 < cursor.n_steps:
            if cursor.step % stride == 0:
                for seat in (0, 1):
                    obs = cursor.observe(seat)
                    for name, value in obs.items():
                        observations.setdefault(name, []).append(value[0])
                    action = cursor.joint_effective_action(seat)
                    unit_ops.append(action["unit_op"].copy())
                    market_ops.append(int(action["order_op"][0]))
                    outcomes.append(cursor.target(seat) + 1)  # loss/draw/win -> 0/1/2
            cursor.advance()
        progress.update(index, detail=path.name)
    packed = {name: np.stack(values) for name, values in observations.items()}
    labels = {
        "unit_op": np.stack(unit_ops),
        "market_op": np.asarray(market_ops, dtype=np.int64),
        "wdl": np.asarray(outcomes, dtype=np.int64),
    }
    return packed, labels


def tensorize(data, device):
    return {key: torch.from_numpy(value).to(device) for key, value in data.items()}


def metrics(model, obs, labels, label, batch_size=128):
    totals = {"loss": 0.0, "unit_correct": 0, "unit_count": 0,
              "market_correct": 0, "wdl_correct": 0, "samples": len(labels["wdl"])}
    model.eval()
    n_batches = (totals["samples"] + batch_size - 1) // batch_size
    progress = Progress(label, n_batches)
    with torch.inference_mode():
        for batch_index, start in enumerate(
                range(0, totals["samples"], batch_size), 1):
            ix = slice(start, start + batch_size)
            batch = {key: value[ix] for key, value in obs.items()}
            out = model(batch)
            active = batch["active_units"][:, 0].bool()
            unit_logits = out["unit_op_logits"][active]
            unit_target = labels["unit_op"][ix][active]
            market_target = labels["market_op"][ix]
            wdl_target = labels["wdl"][ix]
            loss = (nn.functional.cross_entropy(unit_logits, unit_target)
                    + nn.functional.cross_entropy(out["market_op_logits"], market_target)
                    + 0.25 * nn.functional.cross_entropy(out["wdl_logits"], wdl_target))
            n = len(wdl_target)
            totals["loss"] += float(loss) * n
            totals["unit_correct"] += int((unit_logits.argmax(-1) == unit_target).sum())
            totals["unit_count"] += int(active.sum())
            totals["market_correct"] += int(
                (out["market_op_logits"].argmax(-1) == market_target).sum())
            totals["wdl_correct"] += int((out["wdl_logits"].argmax(-1) == wdl_target).sum())
            progress.update(batch_index)
    return {
        "loss": totals["loss"] / totals["samples"],
        "unit_accuracy": totals["unit_correct"] / totals["unit_count"],
        "market_accuracy": totals["market_correct"] / totals["samples"],
        "wdl_accuracy": totals["wdl_correct"] / totals["samples"],
        "samples": totals["samples"],
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--replays", type=Path, default=Path("corpus/data"))
    parser.add_argument("--episodes", type=int, default=12)
    parser.add_argument("--stride", type=int, default=24)
    parser.add_argument("--steps", type=int, default=100)
    parser.add_argument("--batch-size", type=int, default=64)
    parser.add_argument("--seed", type=int, default=20260909)
    parser.add_argument("--out", type=Path,
                        default=Path("reinforcement_learning/artifacts/bc_smoke.pt"))
    args = parser.parse_args()

    if args.episodes < 4 or args.stride < 1 or args.steps < 1 or args.batch_size < 1:
        raise SystemExit("episodes >= 4 and positive stride, steps, and batch size required")

    torch.manual_seed(args.seed)
    np.random.seed(args.seed)
    paths = sorted(args.replays.glob("*.kagz"))[:args.episodes]
    if len(paths) < 4:
        raise SystemExit("need at least four replay episodes")
    split = max(1, int(0.8 * len(paths)))
    train_paths, validation_paths = paths[:split], paths[split:]
    started = time.time()
    train_obs_np, train_labels_np = collect(train_paths, args.stride, "load train replays")
    val_obs_np, val_labels_np = collect(
        validation_paths, args.stride, "load validation replays")

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    train_obs = tensorize(train_obs_np, device)
    train_labels = tensorize(train_labels_np, device)
    val_obs = tensorize(val_obs_np, device)
    val_labels = tensorize(val_labels_np, device)
    model = KaggricultureActor().to(device)
    optimizer = torch.optim.AdamW(model.parameters(), lr=3e-4, weight_decay=1e-4)
    initial = metrics(model, val_obs, val_labels, "initial validation")

    model.train()
    samples = len(train_labels["wdl"])
    progress = Progress("optimize smoke model", args.steps)
    for step in range(args.steps):
        indices = torch.randint(samples, (args.batch_size,), device=device)
        obs = {key: value[indices] for key, value in train_obs.items()}
        active = obs["active_units"][:, 0].bool()
        with torch.autocast(device_type=device.type, dtype=torch.bfloat16,
                            enabled=device.type == "cuda"):
            out = model(obs)
            unit_logits = out["unit_op_logits"][active]
            unit_target = train_labels["unit_op"][indices][active]
            loss = (
                nn.functional.cross_entropy(unit_logits, unit_target)
                + nn.functional.cross_entropy(
                    out["market_op_logits"], train_labels["market_op"][indices])
                + 0.25 * nn.functional.cross_entropy(
                    out["wdl_logits"], train_labels["wdl"][indices])
            )
        optimizer.zero_grad(set_to_none=True)
        loss.backward()
        nn.utils.clip_grad_norm_(model.parameters(), 1.0)
        optimizer.step()
        completed = step + 1
        report_now = completed == 1 or completed % max(1, args.steps // 20) == 0
        progress.update(completed,
                        detail=f"loss={float(loss.detach()):.4f}" if report_now else None,
                        force=report_now)

    final = metrics(model, val_obs, val_labels, "final validation")
    args.out.parent.mkdir(parents=True, exist_ok=True)
    torch.save({"model": model.state_dict(), "seed": args.seed,
                "train_replays": [p.name for p in train_paths],
                "validation_replays": [p.name for p in validation_paths]}, args.out)
    report = {
        "purpose": "pipeline smoke test; not a competitive checkpoint",
        "device": str(device),
        "parameters": sum(p.numel() for p in model.parameters()),
        "episodes": len(paths),
        "stride": args.stride,
        "optimization_steps": args.steps,
        "initial_validation": initial,
        "final_validation": final,
        "elapsed_seconds": time.time() - started,
        "checkpoint": str(args.out),
    }
    report_path = args.out.with_suffix(".json")
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    print(f"[save artifacts] checkpoint={args.out} report={report_path}", flush=True)
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
