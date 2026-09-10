"""Keep selector losses and fallback attribution separate from fixed-course effects."""
import argparse
import collections
import json
from pathlib import Path

from compare_portfolio_variants import describe, paired, read_variant


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    protocol = json.loads((args.directory / "PROTOCOL.json").read_text())
    control = read_variant(args.directory / "current_cash", protocol)
    candidate = read_variant(args.directory / "historical_cash", protocol)
    attribution = collections.Counter()
    comparisons = {name: [] for name in ["chosen", "demand", "old_model"]}
    for key in sorted(control):
        if key[3] != 0:
            continue
        prefix = key[:3]
        old = {b: control[(*prefix, b)] for b in range(3)}
        new = {b: candidate[(*prefix, b)] for b in range(3)}
        old_rank = max(old, key=lambda b: old[b]["predicted_margin"])
        new_rank = max(new, key=lambda b: new[b]["predicted_margin"])
        eligible = [b for b, row in old.items() if row["complete"] and
                    not row["own_forecast_failures"] and not row["rival_forecast_failures"]]
        changed = old[0]["chosen"] != new[0]["chosen"]
        attribution["decisions"] += 1
        attribution["numeric_ranking_changed"] += old_rank != new_rank
        attribution["choice_changed"] += changed
        attribution["changed_after_no_eligible_fallback"] += changed and not eligible
        attribution["historical_choice_equals_old_numeric_argmax"] += new[0]["chosen"] == old_rank
        for comparator in comparisons:
            a = new[new[0]["chosen"]]
            b = old[old[0][comparator]]
            row = paired(key, b, a)
            row.update(control_branch=old[0][comparator], candidate_branch=new[0]["chosen"])
            comparisons[comparator].append(row)
    (args.directory / "RANK_ATTRIBUTION.json").write_text(json.dumps(dict(attribution), indent=2) + "\n")
    report = {}
    for comparator, rows in comparisons.items():
        groups = {}
        for opponent in protocol["opponents"]:
            groups[opponent] = describe([r for r in rows if r["opponent"] == opponent])
        for seat in range(2):
            groups[f"seat_{seat}"] = describe([r for r in rows if r["seat"] == seat])
        report[comparator] = {"all": describe(rows), "groups": groups}
        (args.directory / f"SELECTED_VS_{comparator.upper()}.json").write_text(json.dumps(rows, indent=2) + "\n")
        negative = sorted([r for r in rows if r["margin"] < 0], key=lambda r: r["margin"])
        (args.directory / f"SELECTED_LOSSES_VS_{comparator.upper()}.json").write_text(json.dumps(negative, indent=2) + "\n")
    (args.directory / "SELECTION_AUDIT.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"attribution": dict(attribution), "comparators": {
        k: {"all": v["all"], "negative_mean_opponents": [o for o in protocol["opponents"]
            if v["groups"][o]["margin"]["mean"] < 0]} for k, v in report.items()}}, indent=2))


if __name__ == "__main__":
    main()
