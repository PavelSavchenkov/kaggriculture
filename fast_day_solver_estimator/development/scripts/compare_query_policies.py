"""Paired family uncertainty and directional cases for adaptive query policies."""
import argparse
import json
from pathlib import Path

import numpy as np

from improvement_report import bootstrap


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("study", type=Path)
    args = parser.parse_args()
    rows = [json.loads(line) for line in (args.study / "CASES.jsonl").read_text().splitlines()]
    lookup = {(r["method"], r["pool"]): r for r in rows if r["budget_cpu_seconds"] == 30}
    comparisons = []
    for before in ["original_flat_10/sequential", "original_geometry/sequential", "original_geometry/around", "cost_direct_extra/around"]:
        for after in ["boost/query_cpu", "boost/hybrid_cpu", "boost/hybrid_nominal", "logistic/hybrid_cpu"]:
            paired = []
            for (method, pool), a in lookup.items():
                if method != before:
                    continue
                b = lookup[after, pool]
                paired.append({"pool": pool, "family": a["family"], "before_time": a["time_to_oracle"], "after_time": b["time_to_oracle"],
                               "seconds_saved": a["time_to_oracle"] - b["time_to_oracle"], "before_cost": a["cost"], "after_cost": b["cost"],
                               "oracle_cost": a["oracle_cost"]})
            def saved(sample):
                return float(np.mean([r["seconds_saved"] for r in sample]))
            original_time = float(np.mean([r["before_time"] for r in paired]))
            value = {"before": before, "after": after, "pools": len(paired), "families": len({r["family"] for r in paired}),
                     "before_mean_seconds": original_time, "after_mean_seconds": original_time - saved(paired),
                     "mean_seconds_saved": saved(paired), "relative_time_reduction_percent": 100 * saved(paired) / original_time,
                     "seconds_saved_family_bootstrap_95": bootstrap(paired, saved),
                     "faster_pools": sum(r["seconds_saved"] > .001 for r in paired),
                     "slower_pools": sum(r["seconds_saved"] < -.001 for r in paired),
                     "before_no_certificate_at_30s": sum(r["before_cost"] is None for r in paired),
                     "after_no_certificate_at_30s": sum(r["after_cost"] is None for r in paired),
                     "largest_speedups": sorted(paired, key=lambda r: -r["seconds_saved"])[:5],
                     "largest_regressions": sorted(paired, key=lambda r: r["seconds_saved"])[:5]}
            comparisons.append(value)
            print(before, after, round(value["relative_time_reduction_percent"], 2), value["seconds_saved_family_bootstrap_95"])
    report = {"scope": "Development method selection over eight exposed families. CIs do not correct for selecting models/policies on these same development results.",
              "timing": "Cached actual compiler CPU plus provisional scoring charges; no full original warm-pipeline claim.", "comparisons": comparisons}
    target = args.study / "PAIRED_COMPARISONS.json"
    assert not target.exists()
    target.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
