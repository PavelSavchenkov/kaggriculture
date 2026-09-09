"""Measure input-derived difficulty flags, preserving unknown reference cases."""
import argparse
import hashlib
import json
import math
from pathlib import Path

import numpy as np

from baseline_study import cost


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("curves", type=Path)
    parser.add_argument("estimates", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--point-method", default="cost_direct_extra")
    parser.add_argument("--point-field", default="ranking_score")
    parser.add_argument("--scope", required=True)
    parser.add_argument("--query-forecasts", type=Path)
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text()); assert not data.get("pending_cases")
    curves = {r["id"]: r for r in map(json.loads, args.curves.read_text().splitlines()) if r["method"] == "curve_quantile_0.5"}
    points = {r["id"]: r[args.point_field] for r in map(json.loads, args.estimates.read_text().splitlines()) if r["method"] == args.point_method}
    queries = {}
    if args.query_forecasts:
        for r in map(json.loads, args.query_forecasts.read_text().splitlines()):
            if r["method"] == "calendar/boost":
                key = r["contract_key"], r["workers"]
                if key in queries:
                    assert abs(queries[key] - r["predicted_success"]) < 1e-12
                queries[key] = r["predicted_success"]
    cases, missing = [], []
    for row in data["rows"]:
        identifier = row["id"]
        if identifier not in curves:
            missing.append(identifier); continue
        curve = curves[identifier]
        target = row["reference_workers"]
        point = points.get(identifier)
        error = float(cost([point])[0] - cost([target])[0]) if point is not None and target is not None else None
        probe = None
        if point is not None and queries:
            workers = min(40, max(1, math.ceil(point)))
            key = curve["contract_key"]
            counts = [k for contract, k in queries if contract == key]
            assert counts
            probe = 0.0 if workers < min(counts) else queries[key, workers]
        cases.append({"id": identifier, "families": row["families"], "peak_probability": curve["peak_success_probability"],
                      "probe_probability": probe, "reference_workers": target, "point_workers": point, "cash_error": error})
    result = []
    rules = [("peak", threshold) for threshold in [.25, .5, .75]]
    if queries:
        rules += [("probe", threshold) for threshold in [.2, .5]] + [("peak_or_probe", .2)]
    for rule, threshold in rules:
        def flag(row):
            if rule == "peak":
                return row["peak_probability"] < threshold
            if rule == "probe":
                return row["probe_probability"] is not None and row["probe_probability"] < threshold
            return row["peak_probability"] < .5 or (row["probe_probability"] is not None and row["probe_probability"] < threshold)
        flagged = [r for r in cases if flag(r)]
        known = [r for r in cases if r["cash_error"] is not None]
        for direction, condition in [("under_500", lambda e: e <= -500), ("over_500", lambda e: e >= 500), ("absolute_500", lambda e: abs(e) >= 500)]:
            errors = [r for r in known if condition(r["cash_error"])]
            caught = [r for r in errors if flag(r)]
            easy = [r for r in known if not flag(r)]
            result.append({"rule": rule, "threshold": threshold, "direction": direction, "eligible_contracts": len(cases),
                "flagged": len(flagged), "flagged_fraction": len(flagged) / len(cases),
                "unknown_contracts": sum(r["reference_workers"] is None for r in cases),
                "flagged_unknown_contracts": sum(r["reference_workers"] is None for r in flagged),
                "known_point_errors": len(known), "severe_errors": len(errors), "captured_severe_errors": len(caught),
                "severe_error_recall": len(caught) / len(errors) if errors else None,
                "unflagged_cash_mae": float(np.mean([abs(r["cash_error"]) for r in easy])) if easy else None,
                "flagged_known_cash_mae": float(np.mean([abs(r["cash_error"]) for r in known if flag(r)])) if any(flag(r) for r in known) else None})
    report = {"scope": args.scope, "definition": "Flag a case when the highest predicted three-second success probability across proposed worker counts is below the cutoff. This is model uncertainty/difficulty, never proof of infeasibility.",
        "targets": "Signed errors compare point predictions with best verified physical upper bills. UNKNOWN cases remain in the deferral denominator and have no fabricated error labels.",
        "point_method": args.point_method, "missing_curve_ids": missing, "summary": result,
        "probe": "One query at ceil(point workforce), rounded upward within 1..40. Counts below the necessary query bound have zero probability. Low probe probability diagnoses an optimistic operational cost estimate, distinct from low peak probability over the whole menu.",
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.curves, args.estimates, Path(__file__), *([args.query_forecasts] if args.query_forecasts else [])]}}
    args.output.mkdir(exist_ok=False)
    (args.output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")
    (args.output / "CASES.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in cases))
    print(json.dumps([r for r in result if r["direction"] == "under_500"], indent=2))


if __name__ == "__main__":
    main()
