"""Time one complete compiler invocation under an explicit shop fixture."""
import argparse
import hashlib
import json
import os
import resource
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name"); parser.add_argument("spec", type=Path)
    parser.add_argument("--seed", type=int, required=True); parser.add_argument("--shops", required=True)
    parser.add_argument("--mode", choices=["original", "guided", "stage"], required=True)
    args = parser.parse_args()
    assert len(args.shops) == 8 and all(c in "01234567" for c in args.shops)
    output = EXP / "runs" / args.name; output.mkdir(exist_ok=False)
    binary = EXP / "build" / ("warm_shop_compile_" + args.mode)
    environment = dict(os.environ); environment["LABOR_WARM_SHOPS"] = args.shops
    command = ["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
               str(binary), str(output / "course"), str(args.seed), str(args.spec.resolve()), "3.0"]
    protocol = {"utc": datetime.now(timezone.utc).isoformat(), "status": "running", "mode": args.mode, "seed": args.seed,
                "shops": args.shops, "command": command, "scope": "Same compiler body and source/rival agents; explicit revealed shop fixture. Native failures remain failed outcomes and are not rerun.",
                "timing_scope": "Complete child process CPU and wall including common launcher, source construction, reuse, repair, cold fallback, in-compiler endpoint checks and artifact output. Independent post-run verification is outside timing.",
                "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(), "spec_sha256": hashlib.sha256(args.spec.read_bytes()).hexdigest()}
    (output / "RUN.json").write_text(json.dumps(protocol, indent=2) + "\n")
    before = resource.getrusage(resource.RUSAGE_CHILDREN); start = time.perf_counter()
    with (output / "compile.log").open("w") as stream:
        result = subprocess.run(command, cwd=EXP, env=environment, stdout=stream, stderr=subprocess.STDOUT)
    after = resource.getrusage(resource.RUSAGE_CHILDREN)
    protocol.update(status="completed", completed_utc=datetime.now(timezone.utc).isoformat(), returncode=result.returncode,
                    native_failure=result.returncode not in [0, 3, 4], wall_seconds=time.perf_counter() - start,
                    cpu_seconds=after.ru_utime + after.ru_stime - before.ru_utime - before.ru_stime)
    status = output / "course/STATUS.json"
    if status.is_file(): protocol["compiler_status"] = json.loads(status.read_text())
    (output / "RUN.json").write_text(json.dumps(protocol, indent=2) + "\n")
    print(args.mode, args.seed, args.shops, result.returncode, round(protocol["cpu_seconds"], 3), flush=True)


if __name__ == "__main__":
    main()
