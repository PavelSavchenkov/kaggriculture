"""Apply fixed family-macro addition gates to complete saved pair errors."""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np


def compare(rows, candidate):
    pairs = defaultdict(dict)
    for row in rows:
        if row["mode"] == "cold" and row["method"] in ["original_flat_10", candidate]:
            key = row["pool"], row["candidate"]
            assert row["method"] not in pairs[key]
            pairs[key][row["method"]] = row
    assert all(set(v) == {"original_flat_10", candidate} for v in pairs.values())
    if not pairs: return {"candidate": candidate, "pairs": 0, "passes": False}
    groups = defaultdict(list)
    a_error, b_error = [], []
    for values in pairs.values():
        a, b = values["original_flat_10"], values[candidate]
        assert a["family"] == b["family"] and a["target"] == b["target"]
        a_error.append(a["error"]); b_error.append(b["error"])
        groups[a["family"]].append((abs(a["error"]), abs(b["error"])))
    family = np.array([np.mean(v, axis=0) for _, v in sorted(groups.items())])
    old, new = family.mean(axis=0)
    rng = np.random.default_rng(909906)
    sampled = family[rng.integers(len(groups), size=(10000, len(groups)))].mean(axis=1)
    interval = np.quantile(sampled[:, 0] - sampled[:, 1], [.025, .975])
    old_under, new_under = np.mean(np.array(a_error) <= -500), np.mean(np.array(b_error) <= -500)
    reduction = float(1 - new / old) if old > 0 else None
    gates = {"family_macro_mae_reduction_at_least_20_percent": reduction is not None and reduction >= .2,
             "positive_family_bootstrap_saved_cash_error": bool(interval[0] > 0),
             "no_more_underestimates_by_at_least_500": bool(new_under <= old_under)}
    return {"candidate": candidate, "baseline": "original_flat_10", "pairs": len(pairs), "families": len(groups),
            "baseline_family_macro_cash_mae": float(old), "candidate_family_macro_cash_mae": float(new),
            "family_macro_mae_reduction_fraction": reduction,
            "family_bootstrap_95_saved_cash_error": list(map(float, interval)),
            "baseline_bias": float(np.mean(a_error)), "candidate_bias": float(np.mean(b_error)),
            "baseline_under_by_at_least_500": float(old_under), "candidate_under_by_at_least_500": float(new_under),
            "baseline_over_by_at_least_500": float(np.mean(np.array(a_error) >= 500)),
            "candidate_over_by_at_least_500": float(np.mean(np.array(b_error) >= 500)),
            "gates": gates, "passes": all(gates.values())}


def main():
    parser = argparse.ArgumentParser(); parser.add_argument("report", type=Path)
    parser.add_argument("--scope", required=True); args = parser.parse_args()
    errors, source = args.report / "ERRORS.jsonl", args.report / "RESULTS.json"
    rows = list(map(json.loads, errors.read_text().splitlines())); metadata = json.loads(source.read_text())
    methods = ["cost_direct_extra", "timing_boost", "timing_extra"]
    available = {r["method"] for r in rows}; assert methods[0] in available
    result = {"scope": args.scope, "primary": methods[0], "target": metadata["target"],
              "censored_pairs": metadata["censored_pairs"], "unpredicted_pairs": metadata["unpredicted_pairs"],
              "thresholds": "Inclusive signed error <= -500 and >= 500. This differs at exact equality from older strict-threshold summary fields.",
              "family_weighting": "Unweighted mean of family MAEs; paired bootstrap resamples complete families with equal family weights.",
              "comparisons": [compare(rows, method) for method in methods if method in available],
              "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [errors, source, Path(__file__)]}}
    with (args.report / "MARGINAL_GATE.json").open("x") as stream:
        stream.write(json.dumps(result, indent=2, allow_nan=False) + "\n")
    print(json.dumps(result["comparisons"], indent=2))


if __name__ == "__main__":
    main()
