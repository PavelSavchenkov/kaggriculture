"""Track signed out-of-family errors, physical slices and hard examples."""
import argparse
import json
from collections import defaultdict
from pathlib import Path

import numpy as np

from baseline_study import cost, score


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("study", type=Path)
    args = parser.parse_args()
    output = args.study / "errors"
    output.mkdir(exist_ok=False)
    data = json.loads((args.study / "DATASET.json").read_text())
    cases = {r["id"]: r for r in data["rows"]}
    names = data["feature_names"]
    predictions = [json.loads(line) for line in (args.study / "PREDICTIONS.jsonl").read_text().splitlines()]
    groups, detailed = defaultdict(list), []
    for p in predictions:
        case = cases[p["id"]]
        f = dict(zip(names, case["features"]))
        crops = sum(f[f"species_{i}"] for i in range(5))
        animals = sum(f[f"species_{i}"] for i in [9, 10, 11])
        mix = "mixed" if crops and animals else "crops" if crops else "animals" if animals else "construction_or_empty"
        bands = ["all", "family:" + p["family"], "mix:" + mix,
                 "tasks:" + ("0-50" if f["tasks"] <= 50 else "51-100" if f["tasks"] <= 100 else "101-150" if f["tasks"] <= 150 else "151+"),
                 "workers:" + ("1-5" if p["reference_workers"] <= 5 else "6-10" if p["reference_workers"] <= 10 else "11-14" if p["reference_workers"] <= 14 else "15+"),
                 "distance:" + ("far_7+" if f["distance_max"] >= 7 else "near"),
                 "deadline:" + ("early_deficit" if f["deadline_deficit_1"] > 0 else "no_early_deficit"),
                 "purchase:" + ("after_noon" if f["purchase_last_hour"] >= 12 else "earlier_or_none")]
        if f["op_8"] + f["op_13"] + f["op_14"] + f["op_7"] > 0:
            bands.append("construction_or_planting")
        for species in [0, 1, 2, 3, 4, 9, 10, 11]:
            if f[f"species_{species}"]:
                bands.append(f"species:{species}")
        cost_error = float(cost([p["prediction"]])[0] - cost([p["reference_workers"]])[0])
        row = {**p, "worker_error": p["prediction"] - p["reference_workers"], "cost_error": cost_error,
               "reference_gap": p["reference_workers"] - p["lower_bound"], "slices": bands,
               "short_sweep_workers": case["short_sweep_workers"], "source_workers": case["source_workers"],
               "tasks": f["tasks"], "active_tiles": f["active_tiles"], "maximum_distance": f["distance_max"],
               "early_deficit": f["deadline_deficit_1"], "sources": case["sources"]}
        detailed.append(row)
        for band in bands:
            groups[(p["method"], band)].append(row)
    summaries = []
    for (method, band), rows in sorted(groups.items()):
        truth = np.array([r["reference_workers"] for r in rows]); prediction = np.array([r["prediction"] for r in rows])
        error = prediction - truth
        summaries.append({"method": method, "slice": band, "cases": len(rows), **score(truth, prediction),
                          "over_by_1_worker": float(np.mean(error >= 1)), "under_by_1_worker": float(np.mean(error <= -1)),
                          "p90_absolute_error": float(np.quantile(np.abs(error), .9)),
                          "cost_bias": float(np.mean([r["cost_error"] for r in rows]))})
    (output / "SLICES.json").write_text(json.dumps(summaries, indent=2) + "\n")
    worst = {}
    for method in sorted({r["method"] for r in detailed}):
        rows = [r for r in detailed if r["method"] == method]
        worst[method] = {"underestimates": sorted(rows, key=lambda r: r["worker_error"])[:20],
                         "overestimates": sorted(rows, key=lambda r: -r["worker_error"])[:20],
                         "largest_cash_errors": sorted(rows, key=lambda r: -abs(r["cost_error"]))[:20]}
    (output / "WORST_CASES.json").write_text(json.dumps(worst, indent=2) + "\n")
    (output / "ERRORS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in detailed))
    for r in summaries:
        if r["slice"] == "all":
            print(r["method"], "MAE", round(r["worker_mae"], 3), "bias", round(r["worker_bias"], 3),
                  "under>=1", round(r["under_by_1_worker"], 3), "over>=1", round(r["over_by_1_worker"], 3))


if __name__ == "__main__":
    main()
