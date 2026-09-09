"""Finish unstarted frozen pairs while retaining a recorded native failure."""
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
    output = EXP / "runs/warm_stage_seed_benchmark_v3"
    protocol_path = output / "PROTOCOL.json"
    protocol = json.loads(protocol_path.read_text()); assert protocol["status"] == "running"
    for path, expected in protocol["candidate_sha256"].items():
        assert sha(EXP / path) == expected, path
    existing = []
    for path in sorted((output / "jobs").glob("*/*/RUN.json")):
        row = json.loads(path.read_text()); assert row["status"] == "completed", path
        existing.append({"path": str(path.relative_to(EXP)), "sha256": sha(path), "returncode": row["returncode"]})
    unexpected = [r for r in existing if r["returncode"] not in [0, 3, 4]]
    assert len(unexpected) == 1 and unexpected[0]["returncode"] == 134
    addendum = {"utc": datetime.now(timezone.utc).isoformat(), "reason": "The original benchmark launcher aborted on native backend return 134 (partial assignment outside exact task hour domain). Its completed child record is retained as a failed course with its full recorded CPU.",
                "scope": "Driver continuation only. Keep all completed calls and run each unstarted method/case once, in the original within-pair order. No candidate/source/model/threshold change, no course-prefix resume, and no replacement seed or failure rerun.",
                "existing_runs": existing, "native_failures": unexpected, "script_sha256": sha(Path(__file__))}
    (output / "CONTINUATION.json").write_text(json.dumps(addendum, indent=2) + "\n")

    def run(case):
        root = output / "jobs" / case["id"]; root.mkdir(exist_ok=True)
        records = []
        for mode in case["modes"]:
            run_root = root / mode; run_path = run_root / "RUN.json"
            launcher_code = None
            if not run_path.exists():
                assert not run_root.exists(), "unexplained partial directory"
                name = str(run_root.relative_to(EXP / "runs"))
                launcher = "run_warm_stage.py" if mode == "guided" else "run_warm_compile.py"
                command = ["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python", str(EXP / "scripts" / launcher),
                           name, str(EXP / case["spec"]), "--seconds", "3", "--seed", str(case["seed"])]
                if mode == "original": command.append("--early")
                with (root / (mode + ".log")).open("w") as stream:
                    launcher_code = subprocess.run(command, cwd=EXP, stdout=stream, stderr=subprocess.STDOUT).returncode
            result = json.loads(run_path.read_text()); assert result["status"] == "completed", run_path
            binary = EXP / "build" / ("warm_compile_stage" if mode == "guided" else "warm_compile_early")
            assert result["binary_sha256"] == sha(binary)
            if launcher_code not in [None, 0]:
                assert result["returncode"] not in [0, 3, 4], "launcher failed without a recorded native failure"
            if result["returncode"] == 0:
                target = run_root / "INDEPENDENT.json"
                if not target.exists():
                    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                                    str(EXP / "build/verify_warm_course"), str(run_root / "course"), str(case["seed"]), str(target)], cwd=EXP, check=True)
                replay = json.loads(target.read_text()); matched = json.loads((run_root / "course/MATCHED_RESULT.json").read_text())
                assert replay["real_transitions"] == 719 and all(replay[k] == matched[k] for k in ["cash", "rival_cash", "produced0", "produced1"])
                result["independent"] = replay
            records.append({"mode": mode, **result})
            (root / "RESULTS.json").write_text(json.dumps({**case, "runs": records}, indent=2) + "\n")
        return {**case, "runs": records}

    with ThreadPoolExecutor(max_workers=2) as pool:
        results = list(pool.map(run, protocol["cases"]))
    for row in existing:
        assert sha(EXP / row["path"]) == row["sha256"], "completed child record changed"
    (output / "RESULTS.json").write_text(json.dumps(results, indent=2) + "\n")
    protocol.update(status="completed", completed_utc=datetime.now(timezone.utc).isoformat(), continuation="CONTINUATION.json")
    protocol_path.write_text(json.dumps(protocol, indent=2) + "\n")
    print("Completed every frozen pair; retained all native failures.")


if __name__ == "__main__":
    main()
