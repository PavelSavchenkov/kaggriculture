"""Check exported context models and time full C++ feature/query computation."""
import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

import joblib
import numpy as np
import pandas as pd
from threadpoolctl import threadpool_limits

from context_query_models import design


EXP = Path(__file__).resolve().parents[1]


def timing(values):
    return {"mean": float(np.mean(values)), "p50": float(np.quantile(values, .5)),
            "p95": float(np.quantile(values, .95)), "p99": float(np.quantile(values, .99)), "max": float(np.max(values))}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("study", type=Path)
    parser.add_argument("name")
    parser.add_argument("--datasets", nargs="+", type=Path, required=True)
    parser.add_argument("--manifests", nargs="+", type=Path, required=True)
    parser.add_argument("--repeats", type=int, default=3)
    args = parser.parse_args()
    output = EXP / "runs" / args.name; output.mkdir(exist_ok=False)
    sys.path.insert(0, str(args.study.resolve()))
    models = joblib.load(args.study / "models.joblib")["models"]["calendar"]
    records, names = {}, None
    for path in args.datasets:
        data = json.loads(path.read_text())
        assert names is None or names == data["feature_names"]
        names = data["feature_names"]
        for row in data["rows"]:
            if row["id"] in records:
                assert np.array_equal(np.float32(row["features"]), np.float32(records[row["id"]]["features"]))
                assert row["hire_slots"] == records[row["id"]]["hire_slots"]
            records[row["id"]] = row
    manifest_rows = {}
    for path in args.manifests:
        for line in path.read_text().splitlines():
            fields = line.split(); key, problem, hours, count = fields[:4]
            slots = list(zip(map(int, fields[4::2]), map(int, fields[5::2])))
            assert int(hours) == records[key]["active_hours"] and len(slots) == int(count)
            assert [list(p) for p in slots] == records[key]["hire_slots"]
            manifest_rows.setdefault(key, " ".join(fields[:3] + ["0"] + fields[3:]) + "\n")
    assert set(manifest_rows) == set(records)
    manifest = output / "INPUT.txt"; manifest.write_text("".join(manifest_rows.values()))
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                    str(EXP / "build/predict_context"), str(manifest), str(output / "CPP.csv"), str(args.repeats)], cwd=EXP, check=True)
    ids, counts, features = [], [], []
    for key, row in records.items():
        for workers in range(1, len(row["hire_slots"]) + 2):
            ids.append(key); counts.append(workers); features.append(row["features"])
    x = design(np.array(features), np.array(counts), names)
    expected = pd.DataFrame({"id": ids, "workers": counts})
    with threadpool_limits(limits=1):
        for name, model in models.items(): expected[name + "_expected"] = model.predict(x)
    actual = pd.read_csv(output / "CPP.csv")
    assert len(actual) == len(expected) * args.repeats
    assert not actual.duplicated(["id", "pass", "workers"]).any()
    assert set(actual["pass"]) == set(range(args.repeats))
    comparison = actual.merge(expected, on=["id", "workers"], validate="many_to_one")
    assert len(comparison) == len(actual)
    errors = {}
    for name in models:
        error = np.abs(comparison[name] - comparison[name + "_expected"])
        threshold = 1e-7 * np.maximum(1, np.abs(comparison[name + "_expected"]))
        errors[name] = float(error.max())
        assert np.all(error <= threshold), (name, errors[name])
    feature = actual.drop_duplicates(["id", "pass"])["feature_cpu_us"]
    report = {"status": "C++ parity passed; no fresh generalization or search gain is claimed by this check.",
        "contexts": len(records), "query_checks": len(actual), "max_absolute_error": errors,
        "feature_cpu_us": timing(feature), "three_models_plus_query_features_cpu_us": timing(actual["query_cpu_us"]),
        "timing": "Thread CPU clock; full menu validation, physical/context features and necessary bounds timed per proposal, query capacities and all three models timed per call. JSON parsing and CSV output excluded.",
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [*args.datasets, *args.manifests,
            args.study / "models.joblib", EXP / "models/context_candidate_dev_v2/context_model.hpp", EXP / "build/predict_context", Path(__file__)]}}
    (output / "CHECK.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "input_sha256"}, indent=2))


if __name__ == "__main__":
    main()
