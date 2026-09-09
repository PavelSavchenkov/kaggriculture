"""Run frozen offline reference queries with bounded process concurrency."""
import argparse
import hashlib
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]
REPO = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("run")
    parser.add_argument("--seconds", type=float, default=3)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--round", default="initial_3s")
    parser.add_argument("--binary", choices=["reference", "reference_terminal", "reference_terminal_deadlines"], default="reference")
    args = parser.parse_args()
    root = EXP / "runs" / args.run
    output = root / args.round
    output.mkdir(exist_ok=False)
    cases = json.loads((root / "CASES.json").read_text())
    active_hours = 24 if args.binary == "reference" else 23
    if any(c.get("active_hours", 24) != active_hours for c in cases):
        raise ValueError("reference binary does not match the explicit contract horizon")
    manifests = []
    for worker in range(args.jobs):
        lines = []
        for case in cases[worker::args.jobs]:
            for query in reversed(case["queries"]):
                lines.append(f"{case['id']}_w{query['workers']:02} {EXP / query['path']}\n")
        path = output / f"worker{worker}.txt"
        path.write_text("".join(lines))
        manifests.append(path)
    protocol = {"utc": datetime.now(timezone.utc).isoformat(), "query_seconds": args.seconds,
                "jobs": args.jobs, "root_solver": "V30 unchanged", "status": "running",
                "active_hours": active_hours,
                "terminal_work_deadlines": args.binary == "reference_terminal_deadlines",
                "binary_sha256": hashlib.sha256((EXP / "build" / args.binary).read_bytes()).hexdigest(),
                "cases_sha256": hashlib.sha256((root / "CASES.json").read_bytes()).hexdigest()}
    (output / "STATUS.json").write_text(json.dumps(protocol, indent=2) + "\n")

    def run(index):
        cmd = ["conda", "run", "--no-capture-output", "-n", "kaggriculture", "nice", "-n", "10",
               str(REPO / "day_solver/with_runtime.sh"), str(EXP / "build" / args.binary), str(manifests[index]),
               str(output / f"worker{index}"), str(args.seconds)]
        with (output / f"worker{index}.log").open("w") as log:
            subprocess.run(cmd, cwd=REPO, stdout=log, stderr=subprocess.STDOUT, check=True)
        return {"worker": index, "command": cmd}

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        commands = list(pool.map(run, range(args.jobs)))
    protocol.update(status="completed", completed_utc=datetime.now(timezone.utc).isoformat(), commands=commands)
    (output / "STATUS.json").write_text(json.dumps(protocol, indent=2) + "\n")
    print(f"Completed {len(cases)} reference cases")


if __name__ == "__main__":
    main()
