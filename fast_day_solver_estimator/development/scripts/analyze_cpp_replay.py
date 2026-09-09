"""Report measured scoring-plus-compilation progress, tails and paired controls."""
import argparse
import csv
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np

from baseline_study import cost
from improvement_report import bootstrap


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("replay", type=Path)
    parser.add_argument("dataset", type=Path)
    parser.add_argument("--scope", required=True)
    args = parser.parse_args()
    target = args.replay / "MEASURED_RESULTS.json"
    assert not target.exists()
    data = json.loads(args.dataset.read_text())
    assert not data["pending_cases"]
    definitions = json.loads((args.replay / "DEFINITIONS.json").read_text())
    tests = {r["test"]: r for r in csv.DictReader((args.replay / "cpp/TESTS.csv").open())}
    calls, pools = defaultdict(list), defaultdict(list)
    for r in csv.DictReader((args.replay / "cpp/CALLS.csv").open()):
        calls[r["test"]].append(r)
    for row in data["rows"]:
        pools[row["pool"]].append(row)
    rows = []
    for definition in definitions:
        test, pool = definition["test"], definition["pool"]
        record = tests[test]
        sequence = calls[test]
        known = [r["short_sweep_workers"] for r in pools[pool] if r["short_sweep_workers"] is not None]
        best = min(known) if known else None
        reached = next((r for r in sequence if r["success"] == "1" and int(r["best_workers"]) == best), None)
        assert reached is not None or best is None
        first_model = next((float(r["elapsed_cpu_seconds"]) - float(r["backend_cpu_seconds"]) for r in sequence if int(r["forecasts"]) > 0), None)
        for budget in [1, 3, 10, 30, 60, 120, 240]:
            completed = [r for r in sequence if float(r["elapsed_cpu_seconds"]) <= budget and r["success"] == "1"]
            certified = int(completed[-1]["best_workers"]) if completed else None
            price = float(cost([certified])[0]) if certified is not None else None
            oracle = float(cost([best])[0]) if best is not None else None
            rows.append({"pool": pool, "family": definition["family"], "method": definition["policy"], "budget_cpu_seconds": budget,
                "cost": price, "oracle_cost": oracle, "regret": price - oracle if price is not None else None,
                "time_to_oracle": float(reached["elapsed_cpu_seconds"]) if reached else None,
                "queries_to_oracle": int(reached["step"]) + 1 if reached else None,
                "scoring_cpu_seconds": float(record["scoring_cpu_seconds"]), "decision_cpu_seconds": float(record["decision_cpu_seconds"]),
                "full_backend_cpu_seconds": float(record["backend_cpu_seconds"]), "full_total_cpu_seconds": float(record["total_cpu_seconds"]),
                "forecasts": int(record["forecasts"]), "first_model_cpu_seconds": first_model,
                "model_used_within_budget": first_model is not None and first_model <= budget,
                "model_used_before_best": first_model is not None and reached is not None and first_model <= float(reached["elapsed_cpu_seconds"])})
    groups = defaultdict(list)
    for row in rows:
        groups[row["method"], row["budget_cpu_seconds"]].append(row)
    summary = []
    for (method, budget), group in sorted(groups.items()):
        valid = [r for r in group if r["cost"] is not None]
        timed = [r for r in group if r["time_to_oracle"] is not None]
        times = np.array([r["time_to_oracle"] for r in timed])
        summary.append({"method": method, "budget_cpu_seconds": budget, "pools": len(group), "reference_certified_pools": len(timed),
            "no_certificate_rate": 1 - len(valid) / len(group), "mean_conditional_cost": float(np.mean([r["cost"] for r in valid])) if valid else None,
            "mean_conditional_regret": float(np.mean([r["regret"] for r in valid])) if valid else None,
            "best_cost_recall_all_pools": sum(r["regret"] == 0 for r in valid) / len(group),
            "mean_time_to_best": float(times.mean()) if len(times) else None,
            "p95_time_to_best": float(np.quantile(times, .95)) if len(times) else None,
            "max_time_to_best": float(times.max()) if len(times) else None,
            "mean_scoring_cpu_us_per_proposal": float(np.mean([r["scoring_cpu_seconds"] / 5 * 1e6 for r in group])),
            "mean_full_decision_cpu_ms": float(np.mean([r["decision_cpu_seconds"] * 1e3 for r in group])),
            "model_use_fraction_within_budget": float(np.mean([r["model_used_within_budget"] for r in group])),
            "model_use_fraction_before_best": float(np.mean([r["model_used_before_best"] for r in group]))})
    lookup = {(r["method"], r["pool"]): r for r in rows if r["budget_cpu_seconds"] == 30}
    pairs = [(before, after) for before in ["original_flat_10/sequential", "original_geometry/sequential", "original_geometry/around"]
             for after in ["cost_direct_extra/around", "boost/hybrid_nominal"]]
    pairs += [("cost_direct_extra/around", after) for after in ["boost/hybrid_nominal", "boost/hybrid_cpu", "logistic/hybrid_cpu", "boost/query_nominal"]]
    comparisons = []
    for before, after in pairs:
        paired = []
        for pool in pools:
            a, b = lookup[before, pool], lookup[after, pool]
            paired.append({"pool": pool, "family": a["family"], "before_time": a["time_to_oracle"], "after_time": b["time_to_oracle"],
                "seconds_saved": a["time_to_oracle"] - b["time_to_oracle"] if a["time_to_oracle"] is not None else None,
                "before_cost_at_30s": a["cost"], "after_cost_at_30s": b["cost"]})
        timed = [r for r in paired if r["seconds_saved"] is not None]
        def saved(sample):
            return float(np.mean([r["seconds_saved"] for r in sample]))
        before_mean = float(np.mean([r["before_time"] for r in timed])) if timed else None
        comparisons.append({"before": before, "after": after, "pools": len(paired), "timed_pools": len(timed),
            "before_mean_seconds": before_mean, "after_mean_seconds": before_mean - saved(timed) if timed else None,
            "relative_time_reduction_percent": 100 * saved(timed) / before_mean if timed else None,
            "seconds_saved_family_bootstrap_95": bootstrap(timed, saved) if timed else None,
            "before_no_certificate_at_30s": sum(r["before_cost_at_30s"] is None for r in paired),
            "after_no_certificate_at_30s": sum(r["after_cost_at_30s"] is None for r in paired),
            "largest_regressions": sorted(timed, key=lambda r: r["seconds_saved"])[:5],
            "largest_speedups": sorted(timed, key=lambda r: -r["seconds_saved"])[:5]})
    report = {"scope": args.scope, "timing": "Measured C++ scoring and lazy query decisions plus recorded cold-compiler CPU. Parsing/output and the original warm course compiler are outside this replay. Only completed calls yield certificates at a budget.",
              "empty_reference_pools": sorted(pool for pool, group in pools.items() if all(r["short_sweep_workers"] is None for r in group)),
              "summary": summary, "comparisons": comparisons,
              "dataset_sha256": hashlib.sha256(args.dataset.read_bytes()).hexdigest()}
    target.write_text(json.dumps(report, indent=2) + "\n")
    (args.replay / "MEASURED_CASES.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in rows))
    for row in summary:
        if row["budget_cpu_seconds"] == 30:
            print(row["method"], "time", row["mean_time_to_best"], "cost", row["mean_conditional_cost"], "no_certificate", row["no_certificate_rate"])


if __name__ == "__main__":
    main()
