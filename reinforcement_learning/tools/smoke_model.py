#!/usr/bin/env python3
"""Build one engine observation and run the actor on CPU or CUDA."""

from __future__ import annotations

import time

import torch

from kaggriculture_rl import KaggricultureActor, Progress, VectorEnv


def main() -> None:
    if VectorEnv is None:
        raise SystemExit("build the CMake extension first")
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"[setup] device={device} environments=32", flush=True)
    env = VectorEnv(list(range(32)))
    raw = env.observe(0)
    obs = {name: torch.from_numpy(value).to(device) for name, value in raw.items()}
    model = KaggricultureActor().to(device).eval()
    with torch.inference_mode():
        warmup = Progress("warmup inference", 3)
        for index in range(1, 4):
            output = model(obs)
            warmup.update(index)
        if device.type == "cuda":
            torch.cuda.synchronize()
        started = time.perf_counter()
        benchmark = Progress("benchmark inference", 20)
        for index in range(1, 21):
            output = model(obs, output["state"])
            benchmark.update(index)
        if device.type == "cuda":
            torch.cuda.synchronize()
    parameters = sum(p.numel() for p in model.parameters())
    print(f"device={device} parameters={parameters:,} batch=32 "
          f"mean_forward_ms={(time.perf_counter() - started) * 50:.3f}")
    print("unit logits", tuple(output["unit_op_logits"].shape),
          "wdl", tuple(output["wdl_logits"].shape))


if __name__ == "__main__":
    main()
