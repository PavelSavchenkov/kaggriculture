"""Freeze family-exclusion predictions and compact model candidates."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

import joblib
import numpy as np
from threadpoolctl import threadpool_limits

from baseline_study import score
from models import candidates


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("name")
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text())
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    shutil.copyfile(args.dataset, output / "DATASET.json")
    for name in ["models.py", "model_study.py", "baseline_study.py"]:
        shutil.copyfile(EXP / "scripts" / name, output / name)
    rows = [r for r in data["rows"] if r["reference_workers"] is not None and len(r["families"]) == 1]
    x = np.array([r["features"] for r in rows]); y = np.array([r["reference_workers"] for r in rows])
    groups = np.array([r["families"][0] if r["families"] != ["historical"]
                       else "root_solver" if any(s.startswith("benchmarks/") for s in r["sources"]) else "composition_archive" for r in rows])
    lower = np.array([r["lower_bound"] for r in rows]); names = data["feature_names"]
    if len(rows) < 40:
        raise ValueError("too few complete labeled cases")
    reports, predictions = [], []
    for group in sorted(set(groups)):
        train, test = groups != group, groups == group
        if sum(train) < 20 or sum(test) < 10:
            continue
        models = candidates(names)
        values = {}
        with threadpool_limits(limits=1):
            for name, model in models.items():
                model.fit(x[train], y[train])
                values[name] = model.predict(x[test])
        values["blend_timing_ridge"] = .5 * values["timing_extra"] + .5 * values["ridge_floor"]
        report = {"held_family": str(group), "training_cases": int(sum(train)), "test_cases": int(sum(test)), "results": {}}
        for name, raw in values.items():
            # Raw values stay visible so domain clipping cannot hide instability.
            prediction = np.clip(np.maximum(lower[test], raw), 1, 40)
            report["results"][name] = {**score(y[test], prediction),
                                       "raw_min": float(np.min(raw)), "raw_max": float(np.max(raw))}
            for index, value, before_clamp in zip(np.flatnonzero(test), prediction, raw):
                predictions.append({"id": rows[index]["id"], "family": str(group), "method": name,
                                    "reference_workers": int(y[index]), "lower_bound": int(lower[index]),
                                    "prediction": float(value), "raw_prediction": float(before_clamp)})
        reports.append(report)
        print(group, int(sum(test)), {k: round(v["worker_mae"], 3) for k, v in report["results"].items()}, flush=True)
    with threadpool_limits(limits=1):
        fitted = {name: model.fit(x, y) for name, model in candidates(names).items()}
    joblib.dump({"feature_names": names, "models": fitted}, output / "models.joblib")
    result = {"dataset_sha256": hashlib.sha256((output / "DATASET.json").read_bytes()).hexdigest(), "cases": len(rows),
              "target": data["target"], "scope": "Exposed development family-exclusion study, not untouched test evidence.", "folds": reports}
    (output / "RESULTS.json").write_text(json.dumps(result, indent=2) + "\n")
    (output / "PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in predictions))


if __name__ == "__main__":
    main()
