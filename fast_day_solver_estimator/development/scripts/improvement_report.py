"""Compare search progress with pinned original estimators, by paired family."""
import argparse
import json
from collections import defaultdict
from pathlib import Path

import numpy as np


def bootstrap(rows, statistic):
    families = sorted({r["family"] for r in rows})
    by_family = {f: [r for r in rows if r["family"] == f] for f in families}
    random = np.random.default_rng(909)
    values = []
    for _ in range(4000):
        sample = [r for family in random.choice(families, len(families), replace=True) for r in by_family[family]]
        values.append(statistic(sample))
    return [float(x) for x in np.quantile(values, [.025, .975])]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("budget_study", type=Path)
    parser.add_argument("--scope", default="development proxy; unseen confirmation pending")
    args = parser.parse_args()
    rows = [json.loads(l) for l in (args.budget_study / "CASES.jsonl").open()]
    cases = {(r["method"], r["pool"], r["budget_cpu_seconds"]): r for r in rows}
    methods = sorted({r["method"] for r in rows if not r["method"].startswith("original_")})
    report = []
    for baseline in ["original_flat_10", "original_geometry"]:
        for method in methods:
            paired = []
            for row in rows:
                if row["method"] != method or row["budget_cpu_seconds"] != 60:
                    continue
                before = cases[(baseline, row["pool"], 60)]
                paired.append({"pool": row["pool"], "family": row["family"],
                               "before_time": before["time_to_oracle"], "after_time": row["time_to_oracle"],
                               "before_cost": before["cost"], "after_cost": row["cost"]})
            timed = [r for r in paired if r["before_time"] is not None and r["after_time"] is not None]
            found = [r for r in paired if r["before_cost"] is not None and r["after_cost"] is not None]
            def time_saved(data):
                return float(np.mean([r["before_time"] - r["after_time"] for r in data]))
            def cost_saved(data):
                return float(np.mean([r["before_cost"] - r["after_cost"] for r in data]))
            before_time = float(np.mean([r["before_time"] for r in timed]))
            after_time = float(np.mean([r["after_time"] for r in timed]))
            entry = {"baseline": baseline, "method": method, "pools": len(paired),
                     "time_to_best_reference_before": before_time, "time_to_best_reference_after": after_time,
                     "time_reduction_percent": 100 * (1 - after_time / before_time),
                     "mean_seconds_saved": time_saved(timed), "seconds_saved_family_bootstrap_95": bootstrap(timed, time_saved),
                     "paired_certified_pools_at_60s": len(found), "paired_mean_cost_saved_at_60s": cost_saved(found),
                     "cost_saved_family_bootstrap_95": bootstrap(found, cost_saved),
                     "before_no_certificate_at_60s": sum(r["before_cost"] is None for r in paired) / len(paired),
                     "after_no_certificate_at_60s": sum(r["after_cost"] is None for r in paired) / len(paired)}
            report.append(entry)
    output = {"status": args.scope,
              "comparison": "Same candidate pools and frozen V30 outcomes; original baselines receive zero scoring overhead, modern methods100 us/proposal.",
              "scope": "The labor component is isolated; this does not replay the entire original economic forecast and warm compiler.",
              "results": report}
    (args.budget_study / "IMPROVEMENT_VS_ORIGINAL.json").write_text(json.dumps(output, indent=2) + "\n")
    for row in report:
        if row["method"] in ["fitted_timing", "timing_extra"]:
            print(row["baseline"], row["method"], "time_reduction%", round(row["time_reduction_percent"], 2),
                  "seconds_saved95%", row["seconds_saved_family_bootstrap_95"])


if __name__ == "__main__":
    main()
