"""Signed cost-difference errors on grouped, held-family plan alternatives."""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np

from baseline_study import cost
from screening import screen_reason


EXP = Path(__file__).resolve().parents[1]


def cash(workers):
    return float(cost([workers])[0])


def metrics(rows):
    error = np.array([r["prediction"] - r["target"] for r in rows])
    truth = np.array([r["target"] for r in rows])
    predicted = np.array([r["prediction"] for r in rows])
    nonzero = np.abs(truth) > 1e-8
    return {"pairs": len(rows), "cash_mae": float(np.abs(error).mean()),
            "median_absolute_cash_error": float(np.median(np.abs(error))),
            "cash_bias": float(error.mean()), "p95_absolute_cash_error": float(np.quantile(np.abs(error), .95)),
            "under_by_10": float(np.mean(error < -10)), "over_by_10": float(np.mean(error > 10)),
            "under_by_100": float(np.mean(error < -100)), "over_by_100": float(np.mean(error > 100)),
            "under_by_500": float(np.mean(error < -500)), "over_by_500": float(np.mean(error > 500)),
            "nonzero_reference_pairs": int(nonzero.sum()),
            "sign_accuracy_nonzero": float(np.mean(np.sign(predicted[nonzero]) == np.sign(truth[nonzero]))) if nonzero.any() else None,
            "cost_increases_scored_as_savings": int(np.sum((truth > 0) & (predicted < -1e-8))),
            "savings_scored_as_nonimprovements": int(np.sum((truth < 0) & (predicted >= -1e-8))),
            "below_physical_interval": sum(r["prediction"] < r["physical_delta_lower"] - 1e-8 for r in rows),
            "above_physical_interval": sum(r["prediction"] > r["physical_delta_upper"] + 1e-8 for r in rows)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("study", type=Path)
    parser.add_argument("name")
    args = parser.parse_args()
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    data = json.loads(args.dataset.read_text())
    by_id = {r["id"]: r for r in data["rows"]}
    ix = {name: i for i, name in enumerate(data["feature_names"])}
    predictions = [json.loads(line) for line in (args.study / "PREDICTIONS.jsonl").read_text().splitlines()]
    pools = defaultdict(list)
    for row in predictions:
        if row["method"] not in ["bound", "route_open", "route_return", "task_distance"]:
            pools[(row["method"], row["pool"])].append(row)
    errors, censored = [], []
    for (method, pool), rows in pools.items():
        baseline = next(r for r in rows if r["variant"] == "original")
        base = by_id[baseline["id"]]
        for candidate in rows:
            if candidate["variant"] == "original":
                continue
            row = by_id[candidate["id"]]
            if row["short_sweep_workers"] is None or base["short_sweep_workers"] is None:
                censored.append({"method": method, "pool": pool, "candidate": row["id"],
                                 "candidate_screened": screen_reason(row) is not None,
                                 "candidate_screen_reason": screen_reason(row),
                                 "baseline_missing": base["short_sweep_workers"] is None,
                                 "candidate_missing": row["short_sweep_workers"] is None})
                continue
            baseline_workers = base["short_sweep_workers"]
            target = cash(row["short_sweep_workers"]) - cash(baseline_workers)
            delta_tasks = row["features"][ix["tasks"]] - base["features"][ix["tasks"]]
            if method.startswith("original_flat"):
                coefficient = float(method.rsplit("_", 1)[-1])
                values = {"cold": coefficient * delta_tasks}
            else:
                candidate_workers, base_workers = candidate["ranking_score"], baseline["ranking_score"]
                anchored = np.clip(max(row["lower_bound"], baseline_workers + candidate_workers - base_workers), 1, 40)
                values = {"cold": cash(candidate_workers) - cash(base_workers),
                          "warm_worker_anchor": cash(anchored) - cash(baseline_workers)}
            for mode, prediction in values.items():
                errors.append({"method": method, "mode": mode, "family": candidate["family"], "pool": pool,
                               "candidate": row["id"], "variant": row["variant"], "baseline": base["id"],
                               "prediction": prediction, "target": target, "error": prediction - target,
                               "baseline_workers": baseline_workers, "candidate_workers": row["short_sweep_workers"],
                               "baseline_cost": cash(baseline_workers), "delta_tasks": delta_tasks,
                               "baseline_tasks": base["features"][ix["tasks"]],
                               "candidate_tasks": row["features"][ix["tasks"]],
                               "physical_delta_lower": cash(row["lower_bound"]) - cash(base["reference_workers"]),
                               "physical_delta_upper": cash(row["reference_workers"]) - cash(base["lower_bound"])})
    groups = defaultdict(list)
    for row in errors:
        groups[(row["method"], row["mode"])].append(row)
    summary = []
    slices = []
    for (method, mode), rows in sorted(groups.items()):
        families = {f: [r for r in rows if r["family"] == f] for f in sorted({r["family"] for r in rows})}
        summary.append({"method": method, "mode": mode, **metrics(rows),
                        "family_macro_cash_mae": float(np.mean([metrics(v)["cash_mae"] for v in families.values()])),
                        "families": {f: metrics(v) for f, v in families.items()}})
        buckets = defaultdict(list)
        for row in rows:
            bill, size, delta = row["baseline_cost"], abs(row["delta_tasks"]), row["delta_tasks"]
            buckets[("baseline_cost", "0..19" if bill < 20 else "20..99" if bill < 100 else "100..499" if bill < 500 else "500+")].append(row)
            buckets[("absolute_task_change", "0" if size == 0 else "1..9" if size < 10 else "10..29" if size < 30 else "30+")].append(row)
            buckets[("work_direction", "remove" if delta < 0 else "add" if delta > 0 else "same_task_count")].append(row)
            buckets[("reference_cost_direction", "saving" if row["target"] < 0 else "increase" if row["target"] > 0 else "unchanged")].append(row)
        for (axis, value), selected in sorted(buckets.items()):
            slices.append({"method": method, "mode": mode, "axis": axis, "value": value,
                           "parent_pools": len({r["pool"] for r in selected}), **metrics(selected)})
    report = {"scope": "Held-family development plan differences. Unknown endpoints remain visible and are not assigned costs.",
              "target": "Difference between fixed short-compiler certificate costs, not a difference of proven minimum costs.",
              "warm_mode": "The original short-compiler workforce is a supplied certificate. Anchor the predicted worker change at it; this is not the historical source workforce under another hiring calendar.",
              "physical_interval": "Candidate lower cost minus baseline verified upper cost through candidate verified upper cost minus baseline lower cost.",
              "signed_error": "Negative error understates extra cost or overstates a saving. Positive error overstates extra cost or understates a saving. Reversing a pair is not independent expansion evidence.",
              "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.study / "PREDICTIONS.jsonl"]},
              "summary": summary, "censored_pairs_by_method": dict((m, sum(r["method"] == m for r in censored)) for m, _ in groups)}
    (output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")
    (output / "ERRORS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in errors))
    (output / "CENSORED.json").write_text(json.dumps(censored, indent=2) + "\n")
    (output / "SLICES.json").write_text(json.dumps(slices, indent=2) + "\n")
    for row in summary:
        print(row["method"], row["mode"], "pairs", row["pairs"], "mae", round(row["cash_mae"], 3),
              "bias", round(row["cash_bias"], 3), "under100", row["under_by_100"], "over100", row["over_by_100"])


if __name__ == "__main__":
    main()
