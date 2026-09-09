"""Freeze cost and query forecasts using inputs only, before reference calls."""
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


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("snapshot", type=Path)
    parser.add_argument("index", type=Path)
    parser.add_argument("features", type=Path)
    parser.add_argument("bounds", type=Path)
    parser.add_argument("cpp", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.mkdir(exist_ok=False)
    sys.path[:0] = [str((args.snapshot / d).resolve()) for d in ["cost_study", "query_study"]]
    from query_models import design
    cost_fit = joblib.load(args.snapshot / "cost_study/models.joblib")
    query_fit = joblib.load(args.snapshot / "query_study/models.joblib")
    index = [json.loads(line) for line in args.index.read_text().splitlines()]
    physical = {r["source_sha256"][:20]: r["physical_key"][:20] for r in index}
    features = list(csv.DictReader(args.features.open()))
    ids = [r["id"] for r in features]
    assert set(ids) == set(physical)
    names = cost_fit["feature_names"]
    x = np.array([[float(row[name]) for name in names] for row in features])
    old_bounds = {r["id"]: r for r in csv.DictReader(args.bounds.open())}
    cpp_rows = [r for r in csv.DictReader(args.cpp.open()) if r["pass"] == "0"]
    cpp = {(r["id"], int(r["workers"])): r for r in cpp_rows}
    assert len(cpp) == len(ids) * 40
    lower = np.array([int(cpp[key, 1]["lower_bound"]) for key in ids])
    bound_rows = {}
    for key, value in zip(ids, lower):
        c, old = cpp[key, 1], old_bounds[key]
        assert int(value) >= int(old["lower_bound"])
        assert bool(int(c["output_missing"])) == (float(old["deadline_missing_quantity"]) > 0)
        row = {"lower_bound": int(value), "deadline_missing_quantity": float(old["deadline_missing_quantity"]),
               "input_missing_quantity": int(c["input_missing"])}
        assert physical[key] not in bound_rows or bound_rows[physical[key]] == row
        bound_rows[physical[key]] = row
    with (args.output / "BOUNDS.csv").open("w") as stream:
        writer = csv.DictWriter(stream, fieldnames=["id", "lower_bound", "deadline_missing_quantity", "input_missing_quantity"])
        writer.writeheader()
        writer.writerows(dict(id=key, **bound_rows[physical[key]]) for key in ids)
    (args.output / "PHYSICAL_BOUNDS.json").write_text(json.dumps(bound_rows, indent=2) + "\n")
    parity, records = {}, []
    with threadpool_limits(limits=1):
        values = {name: model.predict(x) for name, model in cost_fit["models"].items()}
        raw_cost = cost_fit["models"]["cost_direct_extra"].model.predict(x)
    parity["cost"] = float(max(abs(value - float(cpp[key, 1]["cost"])) for key, value in zip(ids, raw_cost)))
    assert parity["cost"] < 1e-7 * max(1, np.max(np.abs(raw_cost)))
    for name, predictions in values.items():
        unique = {}
        for i, (key, prediction) in enumerate(zip(ids, predictions)):
            row = {"id": physical[key], "method": name, "raw_prediction": float(prediction),
                   "ranking_score": float(np.clip(max(lower[i], prediction), 1, 40))}
            if row["id"] in unique:
                assert abs(unique[row["id"]]["raw_prediction"] - row["raw_prediction"]) < 1e-8
            unique[row["id"]] = row
        records.extend(unique.values())
    (args.output / "PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in records))
    queries = {}
    for workers in range(1, 41):
        q = design(x, np.full(len(x), workers), lower, names)
        with threadpool_limits(limits=1):
            predictions = {name: query_fit["models"][name].predict(q) for name in ["logistic", "boost", "cpu"]}
        for name, predicted in predictions.items():
            actual = np.array([float(cpp[key, workers][name]) for key in ids])
            error = np.abs(predicted - actual)
            assert np.all(error <= 1e-7 * np.maximum(1, np.abs(predicted))), (name, error.max())
            parity[name] = max(parity.get(name, 0), float(error.max()))
        for i, key in enumerate(ids):
            for name in ["logistic", "boost"]:
                row = {"id": physical[key], "query_id": f"{physical[key]}_w{workers:02}", "workers": workers,
                       "method": name, "predicted_success": float(predictions[name][i]),
                       "predicted_cpu_seconds": float(predictions["cpu"][i])}
                unique = (row["query_id"], name)
                assert unique not in queries or queries[unique] == row
                queries[unique] = row
    (args.output / "QUERY_PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in queries.values()))
    files = [args.index, args.features, args.bounds, args.cpp, args.snapshot / "FREEZE.json"]
    report = {"utc": datetime.now(timezone.utc).isoformat(), "physical_contracts": len(bound_rows),
              "cost_predictions": len(records), "query_predictions": len(queries), "cpp_max_absolute_error": parity,
              "label_access": "Feature, bound, index and fixed model files only. No new reference outcomes.",
              "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in files},
              "output_sha256": {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in args.output.iterdir()}}
    (args.output / "MANIFEST.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
