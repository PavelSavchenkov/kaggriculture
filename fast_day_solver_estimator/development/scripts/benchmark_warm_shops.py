"""Three frozen policies on predeclared new shop/environment fixtures."""
import argparse
import hashlib
import itertools
import json
import os
import subprocess
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(); parser.add_argument("--jobs", type=int, default=2); args = parser.parse_args()
    assert 1 <= args.jobs <= 2
    panel = EXP / "data/warm_shop_panel_v1"
    preparation = json.loads((panel / "PROTOCOL.json").read_text()); assert preparation["status"] == "prepared"
    for path, expected in preparation["sha256"].items(): assert sha(EXP / path) == expected, path
    cases = json.loads((panel / "CASES.json").read_text())
    orders = list(itertools.permutations(["original", "guided", "stage"]))
    for index, case in enumerate(sorted(cases, key=lambda r: sha_text("warm_shop_order_v1:" + r["id"]))):
        case["modes"] = list(orders[index % len(orders)])
    output = EXP / "runs/warm_shop_benchmark_v1"; output.mkdir(exist_ok=False); (output / "jobs").mkdir()
    protocol = {"utc": datetime.now(timezone.utc).isoformat(), "status": "running", "cases": cases, "jobs": args.jobs,
                "scope": preparation["scope"], "primary_candidate": "guided", "secondary_candidate": "stage", "control": "original",
                "order": "Six within-case permutations assigned cyclically in SHA order; no reruns or replacement fixtures.",
                "acceptance": preparation["acceptance"], "candidate_sha256": preparation["sha256"]}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")

    def run(case):
        root = output / "jobs" / case["id"]; root.mkdir(); records = []
        environment = dict(os.environ); environment["LABOR_WARM_SHOPS"] = case["shops"]
        for mode in case["modes"]:
            run_root = root / mode; name = str(run_root.relative_to(EXP / "runs"))
            with (root / (mode + ".log")).open("w") as stream:
                subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python", str(EXP / "scripts/run_warm_shop_compile.py"),
                                name, str(EXP / case["spec"]), "--seed", str(case["seed"]), "--shops", case["shops"], "--mode", mode],
                               cwd=EXP, stdout=stream, stderr=subprocess.STDOUT, check=True)
            result = json.loads((run_root / "RUN.json").read_text()); assert result["status"] == "completed"
            if result["returncode"] == 0:
                target = run_root / "INDEPENDENT.json"
                subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                                str(EXP / "build/verify_warm_shop_course"), str(run_root / "course"), str(case["seed"]), str(target)],
                               cwd=EXP, env=environment, check=True)
                replay = json.loads(target.read_text()); matched = json.loads((run_root / "course/MATCHED_RESULT.json").read_text())
                assert replay["real_transitions"] == 719 and all(replay[k] == matched[k] for k in ["cash", "rival_cash", "produced0", "produced1"])
                result["independent"] = replay
            records.append({"mode": mode, **result})
            (root / "RESULTS.json").write_text(json.dumps({**case, "runs": records}, indent=2) + "\n")
            print(case["id"], mode, "complete", flush=True)
        return {**case, "runs": records}

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(run, cases))
    (output / "RESULTS.json").write_text(json.dumps(results, indent=2) + "\n")
    protocol.update(status="completed", completed_utc=datetime.now(timezone.utc).isoformat())
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")


def sha_text(value):
    return hashlib.sha256(value.encode()).hexdigest()


if __name__ == "__main__":
    main()
