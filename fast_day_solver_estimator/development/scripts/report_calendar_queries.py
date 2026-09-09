"""Evaluate frozen calendar call forecasts, retaining horizon and family slices."""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np

from evaluate_query_forecasts import metrics


def paired_brier(rows, baseline, candidate):
    calls = defaultdict(dict)
    for row in rows:
        if row["method"] in [baseline, candidate]: calls[row["query_id"]][row["method"]] = row
    pairs = [(v[baseline], v[candidate]) for v in calls.values() if set(v) == {baseline, candidate}]
    if not pairs: return {"baseline": baseline, "candidate": candidate, "calls": 0}
    groups = defaultdict(list)
    for a, b in pairs:
        assert a["success"] == b["success"] and a["families"] == b["families"]
        errors = ((a["predicted_success"] - a["success"]) ** 2, (b["predicted_success"] - b["success"]) ** 2)
        for family in a["families"]: groups[family].append(errors)
    family_errors = np.array([np.mean(v, axis=0) for _, v in sorted(groups.items())])
    rng = np.random.default_rng(909904)
    samples = family_errors[rng.integers(len(groups), size=(10000, len(groups)))].mean(axis=1)
    return {"baseline": baseline, "candidate": candidate, "calls": len(pairs), "families": len(groups),
            "baseline_family_macro_brier": float(family_errors[:, 0].mean()),
            "candidate_family_macro_brier": float(family_errors[:, 1].mean()),
            "family_bootstrap_95_saved_brier": list(map(float, np.quantile(samples[:, 0] - samples[:, 1], [.025, .975])))}


def main():
    parser = argparse.ArgumentParser()
    for name in ["dataset", "predictions", "novelty", "output"]: parser.add_argument(name, type=Path)
    parser.add_argument("--controls", type=Path, nargs="+", default=[])
    parser.add_argument("--scope", required=True)
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text()); assert not data["pending_cases"]
    cases = {r["id"]: r for r in data["rows"]}; assert len(cases) == len(data["rows"])
    novelty = {r["contract_key"]: r["novel"] for r in map(json.loads, args.novelty.read_text().splitlines()) if "contract_key" in r}
    assert all(r["contract_key"] in novelty for r in cases.values())
    forecasts = defaultdict(dict)
    for path in [args.predictions, *args.controls]:
        for row in map(json.loads, path.read_text().splitlines()):
            if row["query_id"].rsplit("_w", 1)[0] not in cases: continue
            method = "calendar/" + row["method"] if row["method"] in ["boost", "logistic"] else row["method"]
            assert row["query_id"] not in forecasts[method]
            assert np.isfinite(row["predicted_success"]) and 0 <= row["predicted_success"] <= 1
            assert np.isfinite(row["predicted_cpu_seconds"]) and row["predicted_cpu_seconds"] >= 0
            forecasts[method][row["query_id"]] = row
    assert "calendar/boost" in forecasts
    records, screens = [], []
    for case in cases.values():
        missing = any(case[k] > 0 for k in ["deadline_missing_quantity", "supply_missing", "seed_missing", "land_missing"])
        for query in case["query_outcomes"]:
            assert query["status"] in ["FEASIBLE", "UNKNOWN"] and query["budget_seconds"] == 3
            if missing or query["workers"] < case["lower_bound"] or query["workers"] > len(case["hire_slots"]) + 1:
                assert query["status"] == "UNKNOWN"
                screens.append({"query_id": query["id"], "contract": case["id"], "active_hours": case["active_hours"],
                                "profile": case["profile"], "novel": novelty[case["contract_key"]],
                                "reason": "missing_supply_or_deadline" if missing else "outside_necessary_workforce_range"})
                continue
            for method, values in forecasts.items():
                prediction = values[query["id"]]
                assert prediction["workers"] == query["workers"]
                records.append({"query_id": query["id"], "contract": case["id"], "families": case["families"],
                                "active_hours": case["active_hours"], "profile": case["profile"],
                                "novel": novelty[case["contract_key"]], "workers": query["workers"], "method": method,
                                "success": query["status"] == "FEASIBLE", "cpu_seconds": query["cpu_seconds"],
                                "predicted_success": prediction["predicted_success"],
                                "predicted_cpu_seconds": prediction["predicted_cpu_seconds"]})
    summary, comparisons = [], []
    for scope in ["full", "novel"]:
        rows = [r for r in records if scope == "full" or r["novel"]]
        groups = defaultdict(list)
        for row in rows:
            method = row["method"]
            groups[method, "all", "all"].append(row)
            for axis in ["active_hours", "profile"]: groups[method, axis, str(row[axis])].append(row)
            groups[method, "horizon_profile", f"{row['active_hours']}/{row['profile']}"].append(row)
            for lo, hi in [(1, 4), (5, 8), (9, 12), (13, 16), (17, 24), (25, 32), (33, 40)]:
                if lo <= row["workers"] <= hi:
                    groups[method, "workers", f"{lo}-{hi}"].append(row)
                    groups[method, "horizon_workers", f"{row['active_hours']}/{lo}-{hi}"].append(row)
            decile = min(9, int(row["predicted_success"] * 10))
            groups[method, "probability_bin", f"{decile / 10:.1f}-{(decile + 1) / 10:.1f}"].append(row)
            for family in row["families"]: groups[method, "family", family].append(row)
        summary.extend({"scope": scope, "method": method, "axis": axis, "value": value, **metrics(values)}
                       for (method, axis, value), values in sorted(groups.items()))
        for hours in [None, 23, 24]:
            subset = [r for r in rows if hours is None or r["active_hours"] == hours]
            for control in ["physical_horizon/boost", "old_ignores_calendar/boost"]:
                comparisons.append({"scope": scope, "active_hours": hours, **paired_brier(subset, control, "calendar/boost")})
    result = {"scope": args.scope, "contracts": len(cases), "screened_calls": len(screens),
              "target": "Actual bounded three-second call success. UNKNOWN is a failed bounded call, not proof of physical infeasibility.",
              "screen": "All methods use the same necessary input/release/workforce screen; screened failed calls are retained separately.",
              "cpu_limit": "CPU forecasts exponentiate mean log CPU and must not be called expected CPU.",
              "summary": summary, "paired_family_comparisons": comparisons,
              "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.predictions, args.novelty, *args.controls, Path(__file__)]}}
    args.output.mkdir(exist_ok=False)
    (args.output / "RESULTS.json").write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
    (args.output / "PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in records))
    (args.output / "SCREENED.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in screens))
    print(json.dumps([r for r in summary if r["axis"] == "all"], indent=2))


if __name__ == "__main__":
    main()
