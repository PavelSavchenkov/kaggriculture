"""C++ decision parity and timing on the complete exposed retry utility replay."""
import csv
import hashlib
import json
import subprocess
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

import numpy as np


EXP = Path(__file__).resolve().parents[1]


def main():
    output = EXP / "runs/long_retry_cpp_check_v1"; output.mkdir(exist_ok=False)
    source = EXP / "runs/long_retry_study_v1/PREDICTIONS.jsonl"
    expected_path = EXP / "runs/long_retry_utility_v1/DECISIONS.jsonl"
    study_path = EXP / "runs/long_retry_study_v1/RESULTS.json"
    groups = defaultdict(list)
    for row in map(json.loads, source.read_text().splitlines()):
        groups[row["method"], row["round"] + "/" + row["physical_key"]].append(row)
    folds = {r["family"]: r for r in json.loads(study_path.read_text())["folds"]}
    expected, manifest = {}, []
    for index, row in enumerate(map(json.loads, expected_path.read_text().splitlines())):
        identifier = str(index); expected[identifier] = row
        values = groups[row["method"], row["state_id"]]
        fold = folds[row["family"]]
        fields = [identifier, row["prior_upper_workers"], row["minimum_utility"], fold["training_mean_failed_cpu"], fold["training_mean_success_cpu"], len(values)]
        for value in values:
            fields += [value["workers"], value["probability"], int(value["success"]), value["long_cpu_seconds"]]
        manifest.append(" ".join(map(str, fields)))
        by_sample = {r["sample_id"]: r["workers"] for r in values}
        row["expected_choices"] = [by_sample[s] for s in row["selected_samples"]]
    (output / "INPUT.txt").write_text("\n".join(manifest) + "\n")
    runtime = EXP.parents[2] / "day_solver/with_runtime.sh"
    binary = EXP / "build/replay_long_retry_cpp"
    command = [str(runtime), str(binary), str(output / "INPUT.txt"), str(output / "OUTPUT.csv"), "5"]
    run = subprocess.run(command, cwd=EXP, text=True, capture_output=True)
    (output / "stdout.txt").write_text(run.stdout); (output / "stderr.txt").write_text(run.stderr)
    assert run.returncode == 0, run.stderr
    timings, seen, maximum_cpu_difference = [], set(), 0.
    for row in csv.DictReader((output / "OUTPUT.csv").open()):
        key = row["id"], int(row["pass"])
        assert key not in seen; seen.add(key)
        target = expected[row["id"]]
        assert int(row["final_workers"]) == target["final_workers"]
        assert float(row["final_bill"]) == target["final_bill"]
        assert [int(w) for w in row["choices"].split(":") if w] == target["expected_choices"]
        difference = abs(float(row["backend_cpu"]) - target["cpu_seconds"])
        assert difference < 1e-10
        maximum_cpu_difference = max(maximum_cpu_difference, difference)
        timings.append(float(row["policy_cpu_us"]))
    assert seen == {(key, p) for key in expected for p in range(5)}
    invalid = {"unknown_incumbent": "x 41 .01 30 8 0\n", "duplicate": "x 4 .01 30 8 2 3 .5 0 30 3 .5 0 30\n",
               "invalid_probability": "x 4 .01 30 8 1 3 1.1 0 30\n", "non_improving_query": "x 4 .01 30 8 1 4 .5 0 30\n"}
    for name, content in invalid.items():
        path = output / (name + ".txt"); path.write_text(content)
        check = subprocess.run([str(runtime), str(binary), str(path), str(output / (name + ".csv")), "1"], cwd=EXP, text=True, capture_output=True)
        assert check.returncode == 2 and "invalid" in check.stderr
        (output / (name + ".stderr")).write_text(check.stderr)
    report = {"utc": datetime.now(timezone.utc).isoformat(), "status": "passed", "states": len(expected),
              "decision_replays": len(seen), "complete_sequence_and_bill_mismatches": 0,
              "maximum_backend_cpu_difference": maximum_cpu_difference, "invalid_state_rejections": len(invalid),
              "policy_cpu_us": {"mean": float(np.mean(timings)), "p50": float(np.quantile(timings, .5)), "p95": float(np.quantile(timings, .95)), "p99": float(np.quantile(timings, .99))},
              "timing_scope": "Policy construction and all adaptive choices/observations on already scored options; includes clock overhead. Physical feature extraction and existing model inference are excluded here and must be charged separately.",
              "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [source, expected_path, study_path, Path(__file__), binary, EXP / "include/long_retry_policy.hpp", EXP / "source/replay_long_retry_cpp.cpp"]}}
    (output / "CHECK.json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
