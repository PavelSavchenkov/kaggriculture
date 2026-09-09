"""Apply saved model parameters to feature-only manifests before label analysis."""
import argparse
import csv
import hashlib
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

import joblib
import numpy as np
from threadpoolctl import threadpool_limits


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("study", type=Path)
    parser.add_argument("index", type=Path)
    parser.add_argument("features", type=Path)
    parser.add_argument("bounds", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--cpp", type=Path)
    args = parser.parse_args()
    args.output.mkdir(exist_ok=False)
    # Pickled custom classes must resolve against the code frozen with the fit.
    sys.path.insert(0, str(args.study.resolve()))
    fitted = joblib.load(args.study / "models.joblib")
    raw = list(csv.DictReader(args.features.open()))
    bounds = {r["id"]: int(r["lower_bound"]) for r in csv.DictReader(args.bounds.open())}
    ids = [r["id"] for r in raw]
    x = np.array([[float(r[k]) for k in fitted["feature_names"]] for r in raw])
    with threadpool_limits(limits=1):
        values = {name: model.predict(x) for name, model in fitted["models"].items()}
    cpp_error = {}
    if args.cpp:
        cpp = {r["id"]: r for r in csv.DictReader(args.cpp.open())}
        for name, column in [("fitted_timing", "formula"), ("ridge_floor", "ridge"), ("timing_extra", "trees")]:
            error = max(abs(value - float(cpp[key][column])) for key, value in zip(ids, values[name]))
            assert error < 1e-5, (name, error)
            cpp_error[name] = error
    index = [json.loads(line) for line in args.index.read_text().splitlines()]
    physical = {r["source_sha256"][:20]: r["physical_key"] for r in index}
    records = []
    for name, predictions in values.items():
        unique = {}
        for key, value in zip(ids, predictions):
            row = {"id": physical[key][:20], "method": name, "raw_prediction": float(value),
                   "ranking_score": float(np.clip(max(bounds[key], value), 1, 40))}
            if row["id"] in unique:
                assert abs(unique[row["id"]]["ranking_score"] - row["ranking_score"]) < 1e-8
            unique[row["id"]] = row
        records.extend(unique.values())
    (args.output / "PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in records))
    manifest = {"utc": datetime.now(timezone.utc).isoformat(), "input_physical_contracts": len(set(physical.values())),
                "methods": list(values), "cpp_max_error": cpp_error,
                "label_access": "Only feature/bound/index files and fixed model parameters; no new reference outcomes.",
                "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in [args.study / "models.joblib", args.study / "models.py", args.index, args.features, args.bounds]
                                 + ([args.cpp] if args.cpp else [])}}
    (args.output / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
