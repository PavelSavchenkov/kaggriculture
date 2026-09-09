"""Signed calendar accuracy, censoring and fixed family-macro acceptance gates."""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np

from baseline_study import cost


def summarize(rows):
    known = [r for r in rows if r["target"] is not None and r["prediction"] is not None]
    result = {"contracts": len(rows), "labeled": sum(r["target"] is not None for r in rows), "scored": len(known),
              "unknown": sum(r["target"] is None for r in rows), "rejected": sum(r["prediction"] is None for r in rows)}
    if not known: return result
    error = np.array([r["prediction"] - r["target"] for r in known])
    finite = [r for r in known if 1 <= r["prediction"] <= 40]
    gap = np.array([float(cost([r["prediction"]])[0] - cost([r["target"]])[0]) for r in finite])
    family_errors = defaultdict(list)
    for r in known:
        for family in r["families"]: family_errors[family].append(abs(r["prediction"] - r["target"]))
    result.update(worker_mae=float(np.abs(error).mean()), worker_bias=float(error.mean()),
                  family_macro_worker_mae=float(np.mean([np.mean(v) for v in family_errors.values()])),
                  under_by_at_least_2_workers=float(np.mean(error <= -2)), over_by_at_least_2_workers=float(np.mean(error >= 2)),
                  p95_absolute_worker_error=float(np.quantile(np.abs(error), .95)),
                  workforce_overflow_markers=len(known) - len(finite), finite_cash_predictions=len(finite))
    if len(gap):
        result.update(cash_mae=float(np.abs(gap).mean()), cash_bias=float(gap.mean()),
                      under_by_at_least_500_cash=float(np.mean(gap <= -500)), over_by_at_least_500_cash=float(np.mean(gap >= 500)))
    return result


def compare(rows, baseline, candidate):
    by_id = defaultdict(dict)
    for row in rows:
        if row["method"] in [baseline, candidate]: by_id[row["id"]][row["method"]] = row
    pairs = [(v[baseline], v[candidate]) for v in by_id.values() if set(v) == {baseline, candidate}
             and v[baseline]["target"] is not None and v[baseline]["prediction"] is not None and v[candidate]["prediction"] is not None]
    if not pairs: return {"baseline": baseline, "candidate": candidate, "pairs": 0, "passes": False}
    groups = defaultdict(list)
    for a, b in pairs:
        assert a["target"] == b["target"] and a["families"] == b["families"]
        for f in a["families"]: groups[f].append((abs(a["prediction"] - a["target"]), abs(b["prediction"] - b["target"])))
    family = np.array([np.mean(values, axis=0) for values in groups.values()])
    base, new = family.mean(axis=0)
    rng = np.random.default_rng(909903)
    samples = family[rng.integers(len(family), size=(10000, len(family)))].mean(axis=1)
    interval = np.quantile(samples[:, 0] - samples[:, 1], [.025, .975])
    base_under = np.mean([a["prediction"] - a["target"] <= -2 for a, _ in pairs])
    new_under = np.mean([b["prediction"] - b["target"] <= -2 for _, b in pairs])
    reduction = 1 - new / base if base > 0 else None
    gates = {"family_macro_mae_reduction_at_least_20_percent": reduction is not None and reduction >= .2,
             "positive_family_bootstrap_saved_error": interval[0] > 0,
             "no_more_two_worker_underestimates": new_under <= base_under}
    return {"baseline": baseline, "candidate": candidate, "pairs": len(pairs), "families": len(groups),
            "baseline_family_macro_worker_mae": float(base), "candidate_family_macro_worker_mae": float(new),
            "mae_reduction_fraction": float(reduction) if reduction is not None else None,
            "family_bootstrap_95_saved_worker_error": list(map(float, interval)),
            "baseline_under_by_at_least_2": float(base_under), "candidate_under_by_at_least_2": float(new_under),
            "gates": {k: bool(v) for k, v in gates.items()}, "passes": bool(all(gates.values()))}


def main():
    parser = argparse.ArgumentParser()
    for name in ["dataset", "predictions", "novelty", "output"]: parser.add_argument(name, type=Path)
    parser.add_argument("--controls", nargs="+", type=Path, required=True)
    parser.add_argument("--scope", required=True)
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text()); assert not data.get("pending_cases")
    cases = {r["contract_key"]: r for r in data["rows"]}; assert len(cases) == len(data["rows"])
    forecasts = {}
    for path in [args.predictions, *args.controls]:
        for row in map(json.loads, path.read_text().splitlines()):
            if row["contract_key"] not in cases: continue
            key = row["contract_key"], row["method"]
            assert key not in forecasts; forecasts[key] = row
    novelty = {r["contract_key"]: r["novel"] for r in map(json.loads, args.novelty.read_text().splitlines()) if "contract_key" in r}
    assert cases.keys() <= novelty.keys()
    records = []
    for (key, method), prediction in forecasts.items():
        case = cases[key]; point = prediction["workers"] if "workers" in prediction else prediction["ranking_score"]
        if method == "adaptive_planning_v3":
            rejected = case["lower_bound"] > len(case["hire_slots"]) + 1 or any(case[k] > 0 for k in ["deadline_missing_quantity", "supply_missing", "seed_missing", "land_missing"])
            assert bool(prediction["rejected"]) == rejected
            if rejected: assert point is None and case["reference_workers"] is None
        records.append({"id": case["id"], "contract_key": key, "method": method, "families": case["families"],
                        "active_hours": case["active_hours"], "profile": case["profile"], "novel": novelty[key],
                        "lower_bound": case["lower_bound"], "target": case["reference_workers"], "prediction": point,
                        "error": point - case["reference_workers"] if point is not None and case["reference_workers"] is not None else None})
    assert {r["contract_key"] for r in records if r["method"] == "adaptive_planning_v3"} == cases.keys()
    for method in ["original_geometry", "original_geometry_calendar_floor"]:
        assert {r["contract_key"] for r in records if r["method"] == method} == cases.keys(), method
    summaries, gates = [], []
    for scope in ["full", "novel"]:
        selected = [r for r in records if scope == "full" or r["novel"]]
        groups = defaultdict(list)
        for r in selected:
            groups[r["method"], "all", "all"].append(r)
            for axis in ["active_hours", "profile"]: groups[r["method"], axis, str(r[axis])].append(r)
            groups[r["method"], "horizon_profile", f"{r['active_hours']}/{r['profile']}"].append(r)
            for family in r["families"]: groups[r["method"], "family", family].append(r)
        summaries.extend({"scope": scope, "method": method, "axis": axis, "value": value, **summarize(rows)}
                         for (method, axis, value), rows in sorted(groups.items()))
        for hours in [None, 23, 24]:
            rows = [r for r in selected if hours is None or r["active_hours"] == hours]
            gates.append({"scope": scope, "active_hours": hours, **compare(rows, "original_geometry_calendar_floor", "adaptive_planning_v3")})
    result = {"scope": args.scope, "contracts": len(cases), "novel_contracts": sum(novelty[key] for key in cases),
              "target": "Cheapest verified upper under each exact menu and horizon; unknowns are censored, not infeasible. Physical minima remain unproven.",
              "cash_overflow": "Workforce 41 in an old capacity-transfer control means outside the supported menu. It contributes to worker error but has no finite cash prediction; report its count separately.",
              "primary": "Novel pooled calendar gate; horizon-specific results remain mandatory even if the pooled gate passes.",
              "summary": summaries, "gates": gates,
              "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.predictions, args.novelty, *args.controls, Path(__file__)]}}
    args.output.mkdir(exist_ok=False)
    (args.output / "RESULTS.json").write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
    (args.output / "ERRORS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in records))
    print(json.dumps(gates, indent=2))


if __name__ == "__main__":
    main()
