"""Collect all available warm compiler calls while advancing with the first certificate."""
import argparse
import hashlib
import json
import resource
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("spec", type=Path)
    parser.add_argument("--seconds", type=float, default=3)
    parser.add_argument("--seed", type=int, default=1201301738)
    parser.add_argument("--resume", type=Path)
    args = parser.parse_args()
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    binary = EXP / "build/warm_compile_catalog"
    command = ["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
               str(binary), str(output / "course"), str(args.seed), str(args.spec.resolve()), str(args.seconds)]
    if args.resume:
        command.append(str(args.resume.resolve()))
    protocol = {"utc": datetime.now(timezone.utc).isoformat(), "status": "running", "command": command,
        "scope": "Offline complete available workforce-call catalog. Continue each course using its first full-game-verified certificate. Additional calls restore the same dawn/source/rival state. This is reference work, not a speed benchmark.",
        "timing_scope": "Complete child process CPU and wall time, including the common conda/runtime launcher, contract construction, route reuse, four-worker fixed repair, cold fallback, full-game endpoint checks and artifact output.",
        "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
        "spec_sha256": hashlib.sha256(args.spec.read_bytes()).hexdigest()}
    protocol["resume_prefix"] = str(args.resume) if args.resume else None
    if args.resume:
        protocol["resume_hashes"] = {str(p.relative_to(args.resume)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(args.resume.glob("days/*/*")) if p.name in ["problem.json", "actions.txt"]}
    (output / "RUN.json").write_text(json.dumps(protocol, indent=2) + "\n")
    before = resource.getrusage(resource.RUSAGE_CHILDREN); start = time.perf_counter()
    with (output / "compile.log").open("w") as log:
        result = subprocess.run(command, cwd=EXP, stdout=log, stderr=subprocess.STDOUT)
    after = resource.getrusage(resource.RUSAGE_CHILDREN)
    protocol.update(status="completed", completed_utc=datetime.now(timezone.utc).isoformat(), returncode=result.returncode,
                    wall_seconds=time.perf_counter() - start,
                    cpu_seconds=after.ru_utime + after.ru_stime - before.ru_utime - before.ru_stime)
    status = output / "course/STATUS.json"
    if status.is_file():
        protocol["compiler_status"] = json.loads(status.read_text())
    (output / "RUN.json").write_text(json.dumps(protocol, indent=2) + "\n")
    assert result.returncode in [0, 3, 4], protocol
    print(json.dumps(protocol, indent=2))


if __name__ == "__main__":
    main()
