#!/usr/bin/env python3
"""Export recurrent BC checkpoints as dependency-light LocalLB agent directories."""

from __future__ import annotations

import argparse
from dataclasses import asdict
import hashlib
import json
from pathlib import Path
import re
import shutil
import time

import numpy as np
import torch

from kaggriculture_rl import (KaggricultureActor, ModelConfig, Progress,
                              ReplayCursor, VectorEnv, market_price)
from kaggriculture_rl.action_codec import decode_action
from kaggriculture_rl.portable_runtime import PortableActor


ROOT = Path(__file__).resolve().parents[2]
RUNTIME_SOURCE = ROOT / "reinforcement_learning/python/kaggriculture_rl/portable_runtime.py"
MAIN_SOURCE = ROOT / "reinforcement_learning/deployment/main.py"
CODEC_SOURCE = ROOT / "reinforcement_learning/python/kaggriculture_rl/action_codec.py"


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            value.update(block)
    return value.hexdigest()


def agent_id(cluster: str, prefix: str) -> str:
    base = re.sub(r"[^a-z0-9]+", "-", cluster.lower()).strip("-") or "agent"
    suffix = hashlib.sha1(cluster.encode()).hexdigest()[:8]
    room = max(1, 40 - len(prefix) - len(suffix) - 2)
    return f"{prefix}-{base[:room]}-{suffix}"


def checkpoint_rows(source: Path):
    report_path = source / "training_report.json"
    report = json.loads(report_path.read_text())
    for cluster, row in report["clusters"].items():
        checkpoint = Path(row["checkpoint"])
        if not checkpoint.is_absolute():
            checkpoint = ROOT / checkpoint
        yield cluster, checkpoint, row


def numerical_parity(model: KaggricultureActor, portable: PortableActor,
                     tolerance: float) -> dict[str, float]:
    environment = VectorEnv([1701])
    observations = environment.observe(0)
    torch_observations = {key: torch.from_numpy(value) for key, value in observations.items()}
    with torch.inference_mode():
        expected = model(torch_observations)
    actual = portable.forward(observations)
    keys = ("unit_op_logits", "unit_item_logits", "unit_quantity_logits",
            "market_op_logits", "market_item_logits", "market_quantity_logits")
    errors = {key: float(np.max(np.abs(expected[key].numpy() - actual[key]))) for key in keys}
    errors["hourly_state"] = float(np.max(np.abs(expected["state"].hourly.numpy()
                                                    - actual["state"][0])))
    errors["daily_state"] = float(np.max(np.abs(expected["state"].daily.numpy()
                                                   - actual["state"][1])))
    if max(errors.values()) > tolerance:
        raise RuntimeError(f"portable numerical parity failed: {errors}")
    return errors


def action_parity(model: KaggricultureActor, portable: PortableActor) -> int:
    """Verify the exported runtime selects the same complete structured action."""
    checked = 0
    torch_states, portable_states = [None, None], [None, None]

    def compare(one, seat, label):
        nonlocal checked
        torch_one = {key: torch.from_numpy(value) for key, value in one.items()}
        with torch.inference_mode():
            expected_raw = model(torch_one, torch_states[seat])
        torch_states[seat] = expected_raw["state"]
        expected = {key: value.detach().numpy() for key, value in expected_raw.items()
                    if isinstance(value, torch.Tensor)}

        def torch_market(hidden, operation, item):
            with torch.inference_mode():
                result = model.market_step(torch.from_numpy(hidden),
                                           torch.tensor([operation]),
                                           torch.tensor([item]))
            return {key: value.detach().numpy() for key, value in result.items()}

        expected["market_step"] = torch_market
        actual = portable.forward(one, portable_states[seat])
        portable_states[seat] = actual["state"]
        actual["market_step"] = portable.market_step
        if decode_action(expected, one, market_price) != decode_action(actual, one, market_price):
            raise RuntimeError(f"structured action parity failed at {label} seat={seat}")
        checked += 1

    environment = VectorEnv([1703, 1704, 1705, 1706])
    for seat in (0, 1):
        observations = environment.observe(seat)
        for batch in range(4):
            compare({key: value[batch:batch + 1] for key, value in observations.items()},
                    seat, f"fresh-{batch}")

    replay_paths = sorted((ROOT / "corpus/data").glob("*.kagz"))
    if replay_paths:
        cursor = ReplayCursor(str(replay_paths[0]))
        torch_states, portable_states = [None, None], [None, None]
        while cursor.step + 1 < cursor.n_steps:
            if cursor.step % 24 == 0:
                for seat in (0, 1):
                    compare(cursor.observe(seat), seat, f"replay-{cursor.step}")
            cursor.advance()
    return checked


def export_one(cluster: str, checkpoint_path: Path, destination: Path,
               overwrite: bool, tolerance: float):
    if destination.exists():
        if not overwrite:
            raise FileExistsError(f"destination exists: {destination}")
        shutil.rmtree(destination)
    destination.mkdir(parents=True)
    checkpoint = torch.load(checkpoint_path, map_location="cpu", weights_only=False)
    config = ModelConfig(**checkpoint["model_config"])
    model = KaggricultureActor(config).eval()
    model.load_state_dict(checkpoint["model"])
    arrays = {key: value.detach().cpu().numpy().astype(np.float32, copy=False)
              for key, value in model.state_dict().items()}
    arrays["__config_json"] = np.asarray(json.dumps(asdict(config), sort_keys=True))
    weights_path = destination / "weights.npz"
    np.savez(weights_path, **arrays)
    shutil.copy2(RUNTIME_SOURCE, destination / "portable_runtime.py")
    shutil.copy2(MAIN_SOURCE, destination / "main.py")
    shutil.copy2(CODEC_SOURCE, destination / "action_codec.py")
    (destination / "AGENT.toml").write_text(
        f'display_name = "RL seed: {cluster}"\nauthor = "pavel"\n')
    portable = PortableActor(arrays, asdict(config))
    errors = numerical_parity(model, portable, tolerance)
    parity_actions = action_parity(model, portable)
    started = time.perf_counter()
    portable.forward(VectorEnv([1702]).observe(0))
    latency = (time.perf_counter() - started) * 1000.0
    files = {path.name: {"bytes": path.stat().st_size, "sha256": digest(path)}
             for path in destination.iterdir() if path.is_file()}
    manifest = {
        "format": 1, "agent_id": destination.name, "cluster": cluster,
        "source_checkpoint": str(checkpoint_path),
        "source_checkpoint_sha256": digest(checkpoint_path),
        "completed_updates": checkpoint.get("completed_updates"),
        "model_config": asdict(config), "numerical_max_abs_error": errors,
        "structured_actions_checked": parity_actions,
        "single_forward_cpu_ms": latency, "files": files,
    }
    manifest_path = destination / "export_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    manifest["files"][manifest_path.name] = {
        "bytes": manifest_path.stat().st_size, "sha256": digest(manifest_path)}
    return manifest


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path,
                        default=Path("reinforcement_learning/artifacts/seeds-pilot"))
    parser.add_argument("--output-root", type=Path, required=True)
    parser.add_argument("--agent-prefix", default="rl-seed")
    parser.add_argument("--overwrite", action="store_true")
    parser.add_argument("--parity-tolerance", type=float, default=2e-4)
    args = parser.parse_args()
    rows = list(checkpoint_rows(args.input))
    if not rows:
        raise SystemExit("training report contains no seed checkpoints")
    if not re.fullmatch(r"[a-z0-9][a-z0-9_-]{1,20}", args.agent_prefix):
        raise SystemExit("--agent-prefix must be 2-21 lowercase id characters")
    args.output_root.mkdir(parents=True, exist_ok=True)
    progress = Progress("export seed agents", len(rows))
    summary = {"source": str(args.input), "agents": {}}
    for index, (cluster, checkpoint, _) in enumerate(rows, 1):
        destination = args.output_root / agent_id(cluster, args.agent_prefix)
        report = export_one(cluster, checkpoint, destination,
                            args.overwrite, args.parity_tolerance)
        summary["agents"][destination.name] = report
        progress.update(index, detail=destination.name)
    output = args.output_root / "seed_export_report.json"
    output.write_text(json.dumps(summary, indent=2) + "\n")
    print(f"[export report] {output}", flush=True)


if __name__ == "__main__":
    main()
