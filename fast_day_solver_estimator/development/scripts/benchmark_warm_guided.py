"""Paired complete warm-compiler runs with fixed courses and independent replay."""
import argparse
import hashlib
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("--repeats", type=int, default=2)
    parser.add_argument("--jobs", type=int, default=1)
    args = parser.parse_args()
    output = EXP / "runs" / args.name; output.mkdir(exist_ok=False)
    specs = sorted((EXP / "data/warm_courses_v1/specs").glob("*.txt"))
    assert len(specs) == 7
    cases = []
    for repeat in range(args.repeats):
        for spec in specs:
            key = f"r{repeat}_{spec.stem}"
            first = int(hashlib.sha256(spec.stem.encode()).hexdigest()[:8], 16) % 2
            modes = ["original", "guided"] if (first + repeat) % 2 == 0 else ["guided", "original"]
            cases.append({"id": key, "repeat": repeat, "spec": str(spec.relative_to(EXP)), "modes": modes})
    paths = [EXP / "build/warm_compile_early", EXP / "build/warm_compile_guided", EXP / "build/verify_warm_course",
             EXP / "source/warm_compile_guided.cpp", EXP / "scripts/run_warm_compile.py", EXP / "scripts/run_warm_guided.py",
             EXP / "models/context_candidate_dev_v2/context_model.hpp", EXP / "scripts/analyze_warm_guided.py", Path(__file__), *specs]
    paths.extend(EXP / "include" / name for name in ["context_query_features.hpp", "planning_context.hpp", "planning_features.hpp", "features.hpp", "bounds.hpp", "supply_bounds.hpp", "release_bounds.hpp"])
    protocol = {"utc": datetime.now(timezone.utc).isoformat(), "status": "running", "cases": cases, "jobs": args.jobs,
                "scope": "Exposed fixed-calendar course-compilation benchmark, not unseen online-agent search.",
                "inputs": "All seven imported course specifications, fixed original seed/shops/source/rival, no resume prefixes, three seconds per cold fallback, unchanged warm repair.",
                "comparison": "Original early-input compiler versus route reuse followed by probability-below-0.02 deferral. Deferred queries are retried in original order if all other trials fail. Two repeats reverse each course's method order.",
                "primary": "Mean complete child CPU across all requested courses, including failures. Wall time, failure rate, hire cost, final cash and signed per-course differences are reported separately.",
                "acceptance": "Require at least 20 percent mean CPU saving with a positive course-cluster confidence interval, no extra failed courses, and no worse mean verified hiring bill. Report individual economic and cost regressions; never turn an earlier failure into a speed gain.",
                "verification": "Every successful course is independently replayed over all 719 real transitions. Every learned deferral is logged with its pure query probability; it is not an infeasibility claim.",
                "sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    (output / "jobs").mkdir()

    def run(case):
        root = output / "jobs" / case["id"]; root.mkdir()
        records = []
        for mode in case["modes"]:
            name = str((root / mode).relative_to(EXP / "runs"))
            launcher = "run_warm_guided.py" if mode == "guided" else "run_warm_compile.py"
            command = ["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python", str(EXP / "scripts" / launcher),
                       name, str(EXP / case["spec"]), "--seconds", "3"]
            if mode == "original": command.append("--early")
            with (root / (mode + ".log")).open("w") as log:
                subprocess.run(command, cwd=EXP, stdout=log, stderr=subprocess.STDOUT, check=True)
            run_root = root / mode
            result = json.loads((run_root / "RUN.json").read_text())
            if result["returncode"] == 0:
                verification = run_root / "INDEPENDENT.json"
                subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                                str(EXP / "build/verify_warm_course"), str(run_root / "course"), "1201301738", str(verification)], check=True, cwd=EXP)
                replay = json.loads(verification.read_text())
                reported = json.loads((run_root / "course/MATCHED_RESULT.json").read_text())
                assert all(replay[k] == reported[k] for k in ["cash", "rival_cash", "produced0", "produced1"])
                result["independent"] = replay
            records.append({"mode": mode, **result})
            (root / "RESULTS.json").write_text(json.dumps({**case, "runs": records}, indent=2) + "\n")
            print(case["id"], mode, "returncode", result["returncode"], "cpu", round(result["cpu_seconds"], 3), flush=True)
        return {**case, "runs": records}

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(run, cases))
    (output / "RESULTS.json").write_text(json.dumps(results, indent=2) + "\n")
    protocol.update(status="completed", completed_utc=datetime.now(timezone.utc).isoformat())
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")


if __name__ == "__main__":
    main()
