"""Compare exported cost/query models, including extreme workforce arguments."""
import argparse
import csv
import hashlib
import json
import sys
from pathlib import Path

import joblib
import numpy as np
from threadpoolctl import threadpool_limits


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("cost_study", type=Path)
    parser.add_argument("query_study", type=Path)
    parser.add_argument("features", type=Path)
    parser.add_argument("predictions", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    sys.path[:0] = [str(args.cost_study.resolve()), str(args.query_study.resolve())]
    from query_models import design
    cost_artifact = joblib.load(args.cost_study / "models.joblib")
    query_artifact = joblib.load(args.query_study / "models.joblib")
    features = {r["id"]: r for r in csv.DictReader(args.features.open())}
    rows = list(csv.DictReader(args.predictions.open()))
    first = [r for r in rows if r["pass"] == "0"]
    names = cost_artifact["feature_names"]
    x = np.array([[float(features[r["id"]][name]) for name in names] for r in first])
    k = np.array([int(r["workers"]) for r in first]); lower = np.array([int(r["lower_bound"]) for r in first])
    q = design(x, k, lower, names)
    with threadpool_limits(limits=1):
        expected = {"cost": cost_artifact["models"]["cost_direct_extra"].model.predict(x)}
        expected.update({name: query_artifact["models"][name].predict(q) for name in ["logistic", "boost", "cpu"]})
    errors = {}
    for name, values in expected.items():
        actual = np.array([float(r[name]) for r in first])
        error = np.abs(actual - values)
        errors[name] = {"max_absolute": float(error.max()), "max_relative": float(np.max(error / np.maximum(1e-8, np.abs(values))))}
        assert np.all(error <= 1e-7 * np.maximum(1, np.abs(values))), (name, errors[name])
    by_id = {(r["id"], r["workers"]): r for r in first}
    for row in rows:
        original = by_id[row["id"], row["workers"]]
        assert all(row[name] == original[name] for name in ["lower_bound", "cost", "logistic", "boost", "cpu"])
    def summary(values):
        return {"mean": float(np.mean(values)), **{f"p{p}": float(np.quantile(values, p / 100)) for p in [50, 95, 99]}}
    feature_rows = [r for r in rows if r["workers"] == "1"]
    report = {"unique_inputs": len({r["id"] for r in first}), "query_predictions": len(rows), "parity": errors,
        "timing_us": {"features_and_bound": summary([float(r["feature_us"]) for r in feature_rows]),
                      "cost_model": summary([float(r["cost_us"]) for r in feature_rows]),
                      "one_query_all_three_models": summary([float(r["query_us"]) for r in rows]),
                      "cheap_path_total": summary([float(r["feature_us"]) + float(r["cost_us"]) for r in feature_rows])},
        "scope": "Mixed input order each pass; feature extraction once then all 40 proposed workforce counts. Query timing includes feature transforms, logistic, boosted probability and log-CPU model. Parsing/output excluded; offline reference jobs share the host.",
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.features, args.predictions]}}
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
