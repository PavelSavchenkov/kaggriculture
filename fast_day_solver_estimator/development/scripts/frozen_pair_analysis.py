"""Signed marginal errors for frozen forecasts, preserving equivalent variants."""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np

from marginal_analysis import cash, metrics
from screening import screen_reason


EXP = Path(__file__).resolve().parents[1]


def paired_gain(rows, baseline, candidate):
    groups = defaultdict(dict)
    for row in rows:
        key = row["pool"], row["candidate"]
        if (row["method"], row["mode"]) in [baseline, candidate]:
            groups[key][row["method"], row["mode"]] = row
    pairs = [(v[baseline], v[candidate]) for v in groups.values() if baseline in v and candidate in v]
    if not pairs:
        return None
    families = sorted({a["family"] for a, _ in pairs})
    differences = {family: [abs(a["error"]) - abs(b["error"]) for a, b in pairs if a["family"] == family] for family in families}
    base_mae = np.mean([abs(a["error"]) for a, _ in pairs])
    candidate_mae = np.mean([abs(b["error"]) for _, b in pairs])
    rng = np.random.default_rng(909603)
    samples = []
    for _ in range(5000):
        picked = rng.choice(families, len(families), replace=True)
        samples.append(np.mean([value for family in picked for value in differences[family]]))
    return {"baseline": list(baseline), "candidate": list(candidate), "pairs": len(pairs), "families": len(families),
            "baseline_cash_mae": float(base_mae), "candidate_cash_mae": float(candidate_mae),
            "cash_mae_reduction_fraction": float(1 - candidate_mae / base_mae) if base_mae else None,
            "mean_absolute_error_saved": float(base_mae - candidate_mae),
            "family_bootstrap_95_saved_cash": list(map(float, np.quantile(samples, [.025, .975])))}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("index", type=Path)
    parser.add_argument("predictions", type=Path)
    parser.add_argument("name")
    parser.add_argument("--scope", required=True)
    parser.add_argument("--prediction-field", choices=["ranking_score", "prediction"], default="ranking_score")
    parser.add_argument("--allow-missing-predictions", action="store_true")
    parser.add_argument("--target-field", choices=["short_sweep_workers", "reference_workers"], default="short_sweep_workers")
    args = parser.parse_args()
    output = EXP / "runs" / args.name; output.mkdir(exist_ok=False)
    data = json.loads(args.dataset.read_text())
    assert not data["pending_cases"], "do not analyze a partial reference sweep"
    by_key = {r["physical_key"]: r for r in data["rows"]}
    ix = {name: i for i, name in enumerate(data["feature_names"])}
    forecasts = defaultdict(dict)
    for row in map(json.loads, args.predictions.read_text().splitlines()):
        assert row["id"] not in forecasts[row["method"]]
        forecasts[row["method"]][row["id"]] = row[args.prediction_field]
    pools = defaultdict(list)
    for row in map(json.loads, args.index.read_text().splitlines()):
        if row["physical_key"] in by_key:
            pools[row["pool"]].append(row)
    errors, censored, membership, unpredicted = [], [], [], []
    for pool, members in pools.items():
        base_keys = {r["physical_key"] for r in members if r["variant"] == "original"}
        assert len(base_keys) == 1
        base = by_key[next(iter(base_keys))]
        variants = defaultdict(list)
        for member in members:
            if member["variant"] != "original": variants[member["physical_key"]].append(member)
        for key, aliases in variants.items():
            row = by_key[key]
            family = sorted({r["source_family"] for r in aliases})
            assert len(family) == 1
            identity = {"family": family[0], "pool": pool, "candidate": row["id"], "baseline": base["id"],
                        "variant": aliases[0]["variant"], "variant_aliases": sorted({r["variant"] for r in aliases}),
                        "raw_memberships": len(aliases), "added_assets": sorted({r.get("added_assets", 0) for r in aliases})}
            membership.append(identity)
            delta_tasks = row["features"][ix["tasks"]] - base["features"][ix["tasks"]]
            base_workers, workers = base[args.target_field], row[args.target_field]
            if workers is None or base_workers is None:
                censored.append({**identity, "candidate_screen_reason": screen_reason(row), "baseline_screen_reason": screen_reason(base),
                                 "baseline_missing": base_workers is None, "candidate_missing": workers is None,
                                 "physical_delta_lower": cash(row["lower_bound"]) - cash(base["reference_workers"]) if base["reference_workers"] is not None and row["lower_bound"] <= 40 else None,
                                 "physical_delta_upper": cash(row["reference_workers"]) - cash(base["lower_bound"]) if row["reference_workers"] is not None and base["lower_bound"] <= 40 else None})
                continue
            target = cash(workers) - cash(base_workers)
            missing = [method for method, values in forecasts.items() if row["id"] not in values or base["id"] not in values]
            if missing:
                assert args.allow_missing_predictions, (pool, row["id"], missing)
                unpredicted.append({**identity, "missing_methods": missing, "target": target})
                continue
            predictions = {}
            for method, values in forecasts.items():
                assert row["id"] in values and base["id"] in values
                predictions[method] = values[row["id"]], values[base["id"]]
            if row.get("original_geometry_workers") is not None and base.get("original_geometry_workers") is not None:
                predictions["original_geometry"] = row["original_geometry_workers"], base["original_geometry_workers"]
            cash_predictions = {(f"original_flat_{coefficient}", "cold"): coefficient * delta_tasks for coefficient in [0, 10, 25]}
            for method, (candidate_workers, predicted_base) in predictions.items():
                anchored = np.clip(max(row["lower_bound"], base_workers + candidate_workers - predicted_base), 1, 40)
                cash_predictions[method, "cold"] = cash(candidate_workers) - cash(predicted_base)
                cash_predictions[method, "warm_worker_anchor"] = cash(anchored) - cash(base_workers)
            for (method, mode), prediction in cash_predictions.items():
                errors.append({**identity, "method": method, "mode": mode, "prediction": float(prediction), "target": target, "error": float(prediction - target),
                               "baseline_workers": base_workers, "candidate_workers": workers, "baseline_cost": cash(base_workers), "delta_tasks": delta_tasks,
                               "baseline_tasks": base["features"][ix["tasks"]], "candidate_tasks": row["features"][ix["tasks"]],
                               "physical_delta_lower": cash(row["lower_bound"]) - cash(base["reference_workers"]),
                               "physical_delta_upper": cash(row["reference_workers"]) - cash(base["lower_bound"])})
    groups = defaultdict(list)
    for row in errors: groups[row["method"], row["mode"]].append(row)
    summaries, slices = [], []
    for (method, mode), rows in sorted(groups.items()):
        families = {f: metrics([r for r in rows if r["family"] == f]) for f in sorted({r["family"] for r in rows})}
        summaries.append({"method": method, "mode": mode, **metrics(rows), "families": families,
                          "family_macro_cash_mae": float(np.mean([r["cash_mae"] for r in families.values()]))})
        for threshold in [100, 500, 1000]:
            for axis in ["baseline_cost", "target"]:
                selected = [r for r in rows if r[axis] >= threshold]
                if selected: slices.append({"method": method, "mode": mode, "axis": axis, "at_least": threshold,
                                             "parents": len({r["pool"] for r in selected}), **metrics(selected)})
    comparisons = [paired_gain(errors, ("original_flat_10", "cold"), group) for group in sorted(groups) if group != ("original_flat_10", "cold")]
    target_description = "Difference between initial 3-second compiler upper bills" if args.target_field == "short_sweep_workers" else "Difference between best available verified upper bills"
    report = {"scope": args.scope, "target": target_description + ", not proven minimum marginal cost.", "target_field": args.target_field,
              "counting": "One distinct physical candidate per parent; all raw equivalent variant names retained. Families are resampled as complete groups for paired confidence intervals.",
              "warm_context": "Baseline workforce is the supplied verified certificate from the selected target under the same menu; candidate answers are never predictor inputs.",
              "signed_error": "Negative understates extra labor cost or overstates a saving. Positive overstates extra labor or understates a saving.",
              "pairs": len(membership), "raw_pair_memberships": sum(r["raw_memberships"] for r in membership), "censored_pairs": len(censored), "unpredicted_pairs": len(unpredicted),
              "summary": summaries, "original_flat10_comparisons": comparisons,
              "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.index, args.predictions, Path(__file__)]}}
    (output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")
    (output / "ERRORS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in errors))
    for name, rows in [("CENSORED", censored), ("MEMBERSHIP", membership), ("SLICES", slices), ("UNPREDICTED", unpredicted)]:
        (output / (name + ".json")).write_text(json.dumps(rows, indent=2) + "\n")
    shown = [r for r in summaries if r["mode"] == "cold" and r["method"] in ["cost_direct_extra", "timing_extra", "timing_boost", "original_flat_10", "original_geometry"]]
    print(json.dumps({"pairs": len(membership), "censored": len(censored), "summary": [{k: r[k] for k in ["method", "mode", "cash_mae", "cash_bias", "under_by_500", "over_by_500"]} for r in shown]}, indent=2))


if __name__ == "__main__":
    main()
