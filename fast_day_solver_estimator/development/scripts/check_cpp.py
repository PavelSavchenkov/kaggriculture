"""Verify C++ prediction parity and report mixed-input end-to-end latency."""
import argparse
import csv
import hashlib
import json
import sys
from pathlib import Path

import joblib
import numpy as np


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("study", type=Path)
    parser.add_argument("features", type=Path)
    parser.add_argument("predictions", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    sys.path.insert(0, str(args.study.resolve()))
    artifact = joblib.load(args.study / "models.joblib")
    features = {r["id"]: r for r in csv.DictReader(args.features.open())}
    rows = list(csv.DictReader(args.predictions.open()))
    first = [r for r in rows if r["pass"] == "0"]
    x = np.array([[float(features[r["id"]][k]) for k in artifact["feature_names"]] for r in first])
    errors = {}
    for cpp, name in [("formula", "fitted_timing"), ("ridge", "ridge_floor"), ("trees", "timing_extra")]:
        expected = artifact["models"][name].predict(x)
        actual = np.array([float(r[cpp]) for r in first])
        errors[name] = float(np.max(np.abs(expected - actual)))
        if errors[name] > 1e-5:
            raise ValueError(f"C++ parity failed for{name}: {errors[name]}")
    first_by_id = {r["id"]: r for r in first}
    for r in rows:
        assert all(r[k] == first_by_id[r["id"]][k] for k in ["lower_bound", "impossible", "formula", "ridge", "trees"])
    report = {"cases": len(first), "predictions": len(rows), "maximum_absolute_worker_difference": errors,
              "input_order": "All inputs loaded once, shuffled separately each pass; no input parsing or output writes inside timing.",
              "timing_scope": "Features, bound and all three exported models per call. CPU shared with ongoing independent reference jobs.",
              "timings_us": {k: {"p50": float(np.quantile([float(r[k]) for r in rows], .5)),
                                  "p95": float(np.quantile([float(r[k]) for r in rows], .95)),
                                  "p99": float(np.quantile([float(r[k]) for r in rows], .99))}
                             for k in ["feature_us", "model_us", "combined_us"]},
              "prediction_file_sha256": hashlib.sha256(args.predictions.read_bytes()).hexdigest()}
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))


if __name__ == "__main__":
    main()
