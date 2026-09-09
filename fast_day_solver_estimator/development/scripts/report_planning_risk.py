"""Measure the exported C++ difficulty flags, including unknown inputs."""
import argparse
import hashlib
import json
import math
from pathlib import Path

import numpy as np

from baseline_study import cost


def wilson(successes, total):
    if not total: return None
    z = 1.959963984540054
    p = successes / total; denominator = 1 + z * z / total
    center = (p + z * z / (2 * total)) / denominator
    width = z * math.sqrt(p * (1 - p) / total + z * z / (4 * total * total)) / denominator
    return [max(0., center - width), min(1., center + width)]


def summarize(rows, flag_name, direction):
    flag = lambda r: r[flag_name]
    condition = {"under_500": lambda e: e <= -500, "over_500": lambda e: e >= 500,
                 "absolute_500": lambda e: abs(e) >= 500}[direction]
    flagged = [r for r in rows if flag(r)]
    known = [r for r in rows if r["cash_error"] is not None]
    severe = [r for r in known if condition(r["cash_error"])]
    caught = [r for r in severe if flag(r)]
    groups = sorted({f for r in rows for f in r["families"]})
    family = np.array([[sum(flag(r) for r in rows if f in r["families"]), sum(f in r["families"] for r in rows),
                        sum(f in r["families"] for r in caught), sum(f in r["families"] for r in severe)] for f in groups])
    intervals = None
    if len(groups):
        rng = np.random.default_rng(909905)
        sampled = family[rng.integers(len(groups), size=(10000, len(groups)))].sum(axis=1)
        with_errors = sampled[:, 3] > 0
        intervals = {"flagged_fraction": list(map(float, np.quantile(sampled[:, 0] / sampled[:, 1], [.025, .975]))),
                     "severe_recall": list(map(float, np.quantile(sampled[with_errors, 2] / sampled[with_errors, 3], [.025, .975]))) if with_errors.any() else None,
                     "bootstrap_samples_without_severe_errors": int((~with_errors).sum())}
    recall = len(caught) / len(severe) if severe else None
    fraction = len(flagged) / len(rows) if rows else None
    return {"rule": flag_name, "direction": direction, "contracts": len(rows), "known_point_errors": len(known),
            "unknown_contracts": sum(r["reference_workers"] is None for r in rows),
            "analytically_rejected": sum(r["rejected"] for r in rows),
            "probe_available": sum(r["probe_available"] for r in rows), "flagged": len(flagged), "flagged_fraction": fraction,
            "flagged_unknown": sum(r["reference_workers"] is None for r in flagged),
            "severe_errors": len(severe), "captured_severe_errors": len(caught), "severe_error_recall": recall,
            "recall_wilson_95_descriptive": wilson(len(caught), len(severe)),
            "flagged_fraction_wilson_95_descriptive": wilson(len(flagged), len(rows)),
            "families": len(groups), "family_bootstrap_95": intervals,
            "unflagged_cash_mae": float(np.mean([abs(r["cash_error"]) for r in known if not flag(r)])) if any(not flag(r) for r in known) else None,
            "passes_point_capture_gate": recall is not None and recall >= .8 and fraction <= .3}


def main():
    parser = argparse.ArgumentParser()
    for name in ["dataset", "predictions", "output"]: parser.add_argument(name, type=Path)
    parser.add_argument("--scope", required=True)
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text()); assert not data["pending_cases"]
    by_id, by_context = {}, {}
    for row in map(json.loads, args.predictions.read_text().splitlines()):
        assert row["method"] == "adaptive_planning_v3" and row["contract_key"] not in by_context
        by_context[row["contract_key"]] = row
        for alias in set([row["id"], *row["source_ids"]]):
            assert alias not in by_id
            by_id[alias] = row
    records = []
    for case in data["rows"]:
        prediction = by_context[case["contract_key"]] if "contract_key" in case else by_id[case["id"]]
        point, target = prediction["workers"], case["reference_workers"]
        assert not prediction["rejected"] or target is None
        assert point is None or 1 <= point <= 40
        error = float(cost([point])[0] - cost([target])[0]) if point is not None and target is not None else None
        records.append({"id": case["id"], "contract_key": prediction["contract_key"], "families": case["families"],
                        "active_hours": prediction["active_hours"], "profile": prediction["profile"],
                        "uses_direct_cost": prediction["uses_direct_cost"], "point_workers": point, "reference_workers": target,
                        "cash_error": error, "rejected": prediction["rejected"], "low_peak": prediction["low_peak"],
                        "weak_probe": prediction["weak_probe"], "peak_or_probe": prediction["low_peak"] or prediction["weak_probe"],
                        "probe_available": prediction["probe_probability"] is not None,
                        "complete_curve": prediction["complete_curve"], "cpu_us": prediction["cpu_us"]})
    summary = []
    slices = [("all", "all", records)]
    for axis in ["active_hours", "profile", "uses_direct_cost"]:
        slices += [(axis, str(value), [r for r in records if r[axis] == value]) for value in sorted({r[axis] for r in records})]
    for axis, value, rows in slices:
        for flag in ["low_peak", "weak_probe", "peak_or_probe"]:
            for direction in ["under_500", "over_500", "absolute_500"]:
                summary.append({"axis": axis, "value": value, **summarize(rows, flag, direction)})
    result = {"scope": args.scope, "target": "Per-contract prediction error against verified upper bills, not marginal-pair recall or proven minimum cost.",
              "flags": "Use the unchanged exported C++ booleans. Probe exists only on ordinary direct-cost inputs; other calendars have peak-only diagnosis. Analytical rejection is separate and remains in the denominator.",
              "uncertainty": "Wilson intervals are descriptive independent-case intervals. Family bootstrap records clustering and reports draws with no severe labels separately.",
              "summary": summary, "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.predictions, Path(__file__)]}}
    args.output.mkdir(exist_ok=False)
    (args.output / "RESULTS.json").write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
    (args.output / "CASES.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in records))
    print(json.dumps([r for r in summary if r["axis"] == "all" and r["direction"] == "under_500"], indent=2))


if __name__ == "__main__":
    main()
