import argparse
import collections
import json
import statistics
from pathlib import Path

import numpy as np


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    protocol = json.loads((args.directory / "PROTOCOL.json").read_text())
    groups = collections.defaultdict(dict)
    for opponent in protocol["opponents"]:
        for line in (args.directory / f"{opponent}.jsonl").open():
            row = json.loads(line)
            groups[(opponent, row["seed"], row["seat"])][row["branch"]] = row
    rows = []
    coverage = collections.Counter()
    timings = []
    for (opponent, seed, seat), branches in groups.items():
        assert set(branches) == {0, 1, 2}
        source = branches[0]
        eligible = [c for c, r in branches.items() if r["complete"] and not r["own_forecast_failures"] and not r["rival_forecast_failures"]]
        best = max(branches, key=lambda c: branches[c]["margin"])
        # Diagnostic only: the pre-registered selector maximizes predicted margin.
        cash_chosen = max(eligible, key=lambda c: branches[c]["predicted_cash"], default=source["demand"])
        row = {"opponent": opponent, "seed": seed, "seat": seat, "best": best}
        for mode in ["chosen", "demand", "old_model", "cash_chosen"]:
            branch = cash_chosen if mode == "cash_chosen" else source[mode]
            value = branches[branch]
            row[mode] = {"branch": branch, "margin": value["margin"],
                         "utility": float(value["margin"] > 0) + 0.5 * (value["margin"] == 0),
                         "regret": branches[best]["margin"] - value["margin"]}
        rows.append(row)
        coverage["choices"] += 1
        coverage["choice_differs_demand"] += source["chosen"] != source["demand"]
        coverage["choice_differs_old_model"] += source["chosen"] != source["old_model"]
        coverage["no_eligible_branch"] += not eligible
        coverage["all_three_eligible"] += len(eligible) == 3
        for r in branches.values():
            coverage["compiled"] += r["complete"]
            coverage["branches_with_own_forecast_failure"] += r["own_forecast_failures"] > 0
            coverage["branches_with_rival_forecast_failure"] += r["rival_forecast_failures"] > 0
            timings.append(r["seconds"])
    assert len(rows) == len(protocol["opponents"]) * protocol["seeds_per_opponent"] * 2
    summary, uncertainty = [], []
    for opponent in ["all", *protocol["opponents"]]:
        selected = [r for r in rows if opponent == "all" or r["opponent"] == opponent]
        report = {"opponent": opponent, "cases": len(selected), "modes": {}}
        for mode in ["chosen", "demand", "old_model", "cash_chosen"]:
            report["modes"][mode] = {key: statistics.mean(r[mode][key] for r in selected) for key in ["margin", "utility", "regret"]}
            report["modes"][mode]["branch_counts"] = dict(collections.Counter(r[mode]["branch"] for r in selected))
        summary.append(report)
        for comparator in ["demand", "old_model"]:
            by_seed = collections.defaultdict(list)
            for r in selected:
                by_seed[r["seed"]].append([r["chosen"]["margin"] - r[comparator]["margin"],
                                           100 * (r["chosen"]["utility"] - r[comparator]["utility"])])
            values = np.array([np.mean(by_seed[s], axis=0) for s in sorted(by_seed)])
            rng = np.random.default_rng(202609092344)
            samples = values[rng.integers(0, len(values), size=(10000, len(values)))].mean(axis=1)
            bounds = np.quantile(samples, [0.025, 0.975], axis=0)
            uncertainty.append({"opponent": opponent, "comparator": comparator, "seed_clusters": len(values),
                                "mean_margin_gain": float(values[:, 0].mean()), "margin_ci95": bounds[:, 0].tolist(),
                                "mean_utility_gain_pp": float(values[:, 1].mean()), "utility_ci95_pp": bounds[:, 1].tolist()})
    for name, value in [("CHOICES", rows), ("SUMMARY", summary), ("STATISTICS", uncertainty), ("COVERAGE", dict(coverage))]:
        (args.directory / f"{name}.json").write_text(json.dumps(value, indent=2) + "\n")
    runtime = {"mean_course_seconds": statistics.mean(timings), "p95_course_seconds": float(np.quantile(timings, 0.95)),
               "max_course_seconds": max(timings), "scope": "Compile one course and value it in eight scenarios; excludes loading the scenario library and actual-game evaluation."}
    (args.directory / "TIMING.json").write_text(json.dumps(runtime, indent=2) + "\n")
    print(json.dumps({"all": summary[0], "uncertainty": uncertainty[:2], "coverage": dict(coverage), "timing": runtime}, indent=2))


if __name__ == "__main__":
    main()
