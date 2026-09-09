"""Compare original marginal formulas and frozen models on warm course bills."""
import argparse
import csv
import json
import sys
from collections import defaultdict
from pathlib import Path

import joblib
import numpy as np
from threadpoolctl import threadpool_limits

from baseline_study import cost


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("study", type=Path)
    parser.add_argument("snapshot", type=Path)
    args = parser.parse_args()
    output = args.study / "RESULTS.json"
    assert not output.exists()
    sys.path.insert(0, str((args.snapshot / "cost_study").resolve()))
    fit = joblib.load(args.snapshot / "cost_study/models.joblib")
    features = list(csv.DictReader((args.study / "FEATURES.csv").open()))
    ids = [r["id"] for r in features]
    x = np.array([[float(r[name]) for name in fit["feature_names"]] for r in features])
    with threadpool_limits(limits=1):
        values = {name: model.predict(x) for name, model in fit["models"].items()}
    original = {r["id"]: float(r["workers"]) for r in csv.DictReader((args.study / "ORIGINAL.csv").open())}
    values["original_geometry"] = np.array([original[key] for key in ids])
    predictions = {name: dict(zip(ids, prediction)) for name, prediction in values.items()}
    data = json.loads((args.study / "PAIRS.json").read_text())
    results = []
    for pair in data["pairs"]:
        before, after = pair["baseline_observed_workers"], pair["candidate_observed_workers"]
        truth = float(cost([after])[0] - cost([before])[0])
        for coefficient in [0, 10, 25]:
            estimate = coefficient * (pair["candidate_tasks"] - pair["baseline_tasks"])
            results.append({**pair, "method": f"original_flat_{coefficient}", "truth": truth, "estimate": estimate, "error": estimate - truth})
        for name, scores in predictions.items():
            a, b = scores[pair["baseline"]], scores[pair["candidate"]]
            for mode in ["cold", "baseline_worker_anchor"]:
                estimate = float(cost([b])[0] - cost([a])[0]) if mode == "cold" else float(cost([before + b - a])[0] - cost([before])[0])
                results.append({**pair, "method": name + "/" + mode, "truth": truth, "estimate": estimate, "error": estimate - truth})
    groups = defaultdict(list)
    for row in results:
        groups[row["course"], row["method"]].append(row)
    courses = []
    for (course, method), rows in sorted(groups.items()):
        courses.append({"course": course, "method": method, "ordinary_days": len(rows),
            "observed_marginal_bill": sum(r["truth"] for r in rows), "predicted_marginal_bill": sum(r["estimate"] for r in rows),
            "signed_total_error": sum(r["error"] for r in rows), "daily_mae": float(np.mean([abs(r["error"]) for r in rows])),
            "underestimated_days": sum(r["error"] < -1e-8 for r in rows), "overestimated_days": sum(r["error"] > 1e-8 for r in rows)})
    report = {"scope": data["scope"], "unsupported": data["unsupported"], "courses": courses,
              "target_limit": "Observed valid compiled bills under inherited hire calendars, not workforce minima. This diagnostic does not establish calibrated estimates for that calendar, a full-season estimate or a search speedup."}
    output.write_text(json.dumps(report, indent=2) + "\n")
    (args.study / "ERRORS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in results))
    for row in courses:
        if row["method"] in ["original_flat_10", "original_geometry/cold", "cost_direct_extra/cold", "cost_direct_extra/baseline_worker_anchor"]:
            print(row["course"], row["method"], round(row["observed_marginal_bill"], 2), round(row["predicted_marginal_bill"], 2), round(row["signed_total_error"], 2))


if __name__ == "__main__":
    main()
