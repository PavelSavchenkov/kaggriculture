"""Derive cheap labor scores from held-family call-success curves."""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np

from baseline_study import cost, score


def curve_scores(workers, probabilities):
    order = np.argsort(workers)
    k, p = np.asarray(workers)[order], np.asarray(probabilities)[order]
    assert len(set(k)) == len(k) and np.all((p >= 0) & (p <= 1))
    cdf = np.maximum.accumulate(p)
    peak = cdf[-1]
    assert peak > 0
    mass = np.diff(np.r_[0, cdf / peak])
    prices = cost(np.arange(1, 41))
    result = {"curve_mean_workers": float(mass @ k),
              "curve_mean_cost": float(np.interp(mass @ cost(k), prices, np.arange(1, 41)))}
    for quantile in [.25, .5, .75, .9]:
        result[f"curve_quantile_{quantile:g}"] = float(k[np.searchsorted(cdf / peak, quantile)])
    result["curve_absolute_half"] = float(k[np.searchsorted(cdf, min(.5, peak))])
    return result, float(peak)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("forecasts", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--physical-ids", action="store_true")
    parser.add_argument("--query-method", default="calendar/boost")
    parser.add_argument("--scope", required=True)
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text()); assert not data.get("pending_cases")
    by_context = {r["id"]: r["contract_key"] for r in data["rows"]}
    contexts = {r["contract_key"]: r for r in data["rows"]}
    curves = defaultdict(dict)
    for row in map(json.loads, args.forecasts.read_text().splitlines()):
        if row["method"] != args.query_method:
            continue
        key = row["contract_key"] if "contract_key" in row else by_context[row["context_id"]]
        workers, value = row["workers"], row["predicted_success"]
        context = contexts[key]
        if workers < context["lower_bound"] or any(context.get(k, 0) > 0 for k in ["deadline_missing_quantity", "supply_missing", "seed_missing", "land_missing"]):
            continue
        if workers in curves[key]:
            assert abs(curves[key][workers] - value) < 1e-12
        curves[key][workers] = value
    predictions, missing = [], []
    for row in data["rows"]:
        key = row["contract_key"]
        if key not in curves:
            missing.append({"id": row["id"], "families": row["families"], "reference_workers": row["reference_workers"]})
            continue
        if len(row["families"]) != 1:
            missing.append({"id": row["id"], "families": row["families"], "reason": "shared_family"})
            continue
        scores, peak = curve_scores(list(curves[key]), list(curves[key].values()))
        identifier = row["obligation_key"][:20] if args.physical_ids else row["id"]
        for method, value in scores.items():
            predictions.append({"id": identifier, "contract_key": key, "family": row["families"][0], "method": method,
                "ranking_score": value, "raw_prediction": value, "reference_workers": row["reference_workers"],
                "peak_success_probability": peak, "predicted_hard": peak < .5})
    groups = defaultdict(list)
    for row in predictions:
        if row["reference_workers"] is not None:
            groups[row["method"]].append(row)
    summary = []
    for method, rows in sorted(groups.items()):
        y = np.array([r["reference_workers"] for r in rows]); p = np.array([r["ranking_score"] for r in rows])
        gap = cost(p) - cost(y)
        families = sorted({r["family"] for r in rows})
        family = {f: score(np.array([r["reference_workers"] for r in rows if r["family"] == f]),
                           np.array([r["ranking_score"] for r in rows if r["family"] == f])) for f in families}
        summary.append({"method": method, "labeled_contracts": len(rows), **score(y, p), "cash_bias": float(gap.mean()),
            "under_by_at_least_500_cash": float(np.mean(gap <= -500)), "over_by_at_least_500_cash": float(np.mean(gap >= 500)),
            "family_macro_worker_mae": float(np.mean([r["worker_mae"] for r in family.values()])), "families": family})
    report = {"scope": args.scope, "query_method": args.query_method,
        "construction": "Take the running maximum of individual query-success probabilities by increasing workforce, normalize by its peak, then derive worker/cash means and quantiles. These are heuristic scores; the probabilities of separate solver calls are not a proven joint minimum-workforce CDF.",
        "boundary": "No outcomes enter score construction. The target upper bound is read only for offline evaluation. Peak below 0.5 flags predicted difficulty; it never proves infeasibility.",
        "query_range": "Uses available held-family forecasts above the physical lower bound. Full-range query catalogs are required for consistent use.",
        "summary": summary, "unpredicted": missing,
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.forecasts, Path(__file__)]}}
    args.output.mkdir(exist_ok=False)
    (args.output / "PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in predictions))
    (args.output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps([{k: v for k, v in r.items() if k != "families"} for r in summary], indent=2))


if __name__ == "__main__":
    main()
