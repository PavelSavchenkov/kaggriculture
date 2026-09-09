"""Evaluate the frozen warm deferral policy on predeclared new seeds."""
import argparse
import hashlib
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    panel = EXP / "data/warm_seed_panel_v2"
    preparation = json.loads((panel / "PROTOCOL.json").read_text()); assert preparation["status"] == "prepared"
    for name, expected in preparation["sha256"].items(): assert sha(EXP / name) == expected, name
    cases = json.loads((panel / "CASES.json").read_text())
    for case in cases:
        first = int(hashlib.sha256(("warm_seed_order_v2:" + case["id"]).encode()).hexdigest()[:8], 16) % 2
        case.update(repeat=0, modes=["original", "guided"] if first == 0 else ["guided", "original"])
    output = EXP / "runs" / args.name; output.mkdir(exist_ok=False); (output / "jobs").mkdir()
    paths = [Path(__file__), panel / "PROTOCOL.json", panel / "CASES.json", panel / "EXCLUDED.json",
             EXP / "scripts/run_warm_compile.py", EXP / "scripts/run_warm_guided.py", EXP / "scripts/analyze_warm_guided.py",
             EXP / "build/verify_warm_course"]
    protocol = {"utc": datetime.now(timezone.utc).isoformat(), "status": "running", "cases": cases, "jobs": args.jobs,
        "scope": "Prospective environment-seed transfer for the unchanged warm deferral candidate, same exposed source/rival and fixed shops. Not unseen-agent or shop-sequence generalization.",
        "selection": "Every eligible case from the fixed eight-seed panel. Exactly one run per method and case; no replacement seeds, prefixes, refit or threshold change.",
        "primary": "Complete child CPU including source construction, reuse, repair, cold fallback, full-game endpoint replay, artifact writes and launch overhead. All failures remain in denominator.",
        "acceptance": "Keep the strict development gate: at least 20 percent mean CPU saving, positive seed-cluster confidence interval, no lost certificates and no higher mean common-success hire bill. Also report seed/course clustered quality uncertainty without using it to erase a failed strict gate.",
        "verification": "Independently replay all 719 transitions of every completed course. Compare cash, rival cash and all produced goods with compiler output.",
        "candidate_sha256": preparation["sha256"], "sha256": {str(p.relative_to(EXP)): sha(p) for p in paths}}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")

    def run(case):
        root = output / "jobs" / case["id"]; root.mkdir(); records = []
        for mode in case["modes"]:
            name = str((root / mode).relative_to(EXP / "runs"))
            launcher = "run_warm_guided.py" if mode == "guided" else "run_warm_compile.py"
            command = ["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python", str(EXP / "scripts" / launcher),
                       name, str(EXP / case["spec"]), "--seconds", "3", "--seed", str(case["seed"])]
            if mode == "original": command.append("--early")
            with (root / (mode + ".log")).open("w") as stream:
                subprocess.run(command, cwd=EXP, stdout=stream, stderr=subprocess.STDOUT, check=True)
            run_root = root / mode; result = json.loads((run_root / "RUN.json").read_text())
            if result["returncode"] == 0:
                target = run_root / "INDEPENDENT.json"
                subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                                str(EXP / "build/verify_warm_course"), str(run_root / "course"), str(case["seed"]), str(target)], cwd=EXP, check=True)
                replay = json.loads(target.read_text()); expected = json.loads((run_root / "course/MATCHED_RESULT.json").read_text())
                assert all(replay[k] == expected[k] for k in ["cash", "rival_cash", "produced0", "produced1"])
                result["independent"] = replay
            records.append({"mode": mode, **result})
            (root / "RESULTS.json").write_text(json.dumps({**case, "runs": records}, indent=2) + "\n")
            print(case["id"], mode, "returncode", result["returncode"], "cpu", round(result["cpu_seconds"], 3), flush=True)
        return {**case, "runs": records}

    with ThreadPoolExecutor(max_workers=args.jobs) as pool: results = list(pool.map(run, cases))
    (output / "RESULTS.json").write_text(json.dumps(results, indent=2) + "\n")
    protocol.update(status="completed", completed_utc=datetime.now(timezone.utc).isoformat())
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")


if __name__ == "__main__":
    main()
