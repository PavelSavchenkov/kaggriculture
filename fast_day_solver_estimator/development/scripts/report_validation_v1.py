"""Freeze the first candidate's acceptance decision without changing its fit."""
import csv
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

import numpy as np


EXP = Path(__file__).resolve().parents[1]


def main():
    output = EXP / "runs/validation_v1_report"
    output.mkdir(exist_ok=False)
    source_path = EXP / "runs/validation_novel_evaluation_v1/RESULTS.json"
    layout_path = EXP / "runs/validation_layout_novel_study_v1/RESULTS.json"
    budget_path = EXP / "runs/validation_novel_search_budget_v1/IMPROVEMENT_VS_ORIGINAL.json"
    source = json.loads(source_path.read_text()); layout = json.loads(layout_path.read_text()); budget = json.loads(budget_path.read_text())
    methods = {r["method"]: r for r in source["summary"]}
    primary = methods["timing_extra"]; baseline = methods["fitted_tasks"]
    families = sorted(primary["families"])
    before = np.array([baseline["families"][f]["worker_mae"] for f in families])
    after = np.array([primary["families"][f]["worker_mae"] for f in families])
    random = np.random.default_rng(909)
    sampled = random.integers(0, len(families), (4000, len(families)))
    reductions = 100 * (1 - after[sampled].mean(axis=1) / before[sampled].mean(axis=1))
    choices = {r["method"]: r for r in layout["summary"] if r["mode"] == "cold" and r["candidate_budget"] == 1}
    geometric = ["original_geometry", "fitted_geometry", "route_open", "route_return"]
    comparisons = [{"baseline": name, "conditional_cost_before": choices[name]["mean_verified_cost"],
                    "conditional_cost_after": choices["timing_extra"]["mean_verified_cost"],
                    "cost_reduction_percent": 100 * (1 - choices["timing_extra"]["mean_verified_cost"] / choices[name]["mean_verified_cost"]),
                    "failure_rate_before": choices[name]["no_certificate_rate"], "failure_rate_after": choices["timing_extra"]["no_certificate_rate"]}
                   for name in geometric]
    latency = {}
    for name in ["validation", "validation_layout", "synthetic"]:
        rows = list(csv.DictReader((EXP / "models/candidate_v1" / (name + "_predictions_first.csv")).open()))
        values = np.array([float(r["combined_us"]) for r in rows])
        latency[name] = {"calls": len(values), "mean_us": float(values.mean()), "p50_us": float(np.median(values)),
                         "p95_us": float(np.quantile(values, .95)), "p99_us": float(np.quantile(values, .99))}
    gates = {"source_macro_mae_15_percent": primary["family_macro_worker_mae"] <= .85 * baseline["family_macro_worker_mae"],
             "source_severe_underestimate_tolerance": primary["under_by_at_least_2_workers"] <= baseline["under_by_at_least_2_workers"] + .02,
             "layout_cost_5_percent_against_strongest_geometric": all(r["cost_reduction_percent"] >= 5 for r in comparisons),
             "layout_failure_tolerance": all(r["failure_rate_after"] <= r["failure_rate_before"] + .02 for r in comparisons),
             "validation_100us_p95_tier": all(latency[name]["p95_us"] < 100 for name in ["validation", "validation_layout"])}
    blocking = {k: v for k, v in gates.items() if k != "validation_100us_p95_tier"}
    report = {"utc": datetime.now(timezone.utc).isoformat(), "candidate": "candidate_v1", "primary_method": "timing_extra", "accepted": all(blocking.values()),
              "decision": "Do not accept v1 as the final search estimator: its first-wave stronger-baseline layout-cost gate failed.",
              "novel_source_contracts": source["contracts"], "source_upper_bound_labels": source["labeled"], "unresolved_source_contracts": source["unresolved"],
              "novel_layout_pools": layout["complete_pools"], "source_macro_worker_mae_before": baseline["family_macro_worker_mae"],
              "source_macro_worker_mae_after": primary["family_macro_worker_mae"],
              "source_macro_mae_reduction_percent": 100 * (1 - after.mean() / before.mean()),
              "source_macro_mae_reduction_family_bootstrap_95": [float(x) for x in np.quantile(reductions, [.025, .975])],
              "source_original_geometry_macro_mae": methods["original_geometry"]["family_macro_worker_mae"],
              "source_underestimate_by_at_least_two_workers_before": baseline["under_by_at_least_2_workers"],
              "source_underestimate_by_at_least_two_workers_after": primary["under_by_at_least_2_workers"],
              "layout_comparisons": comparisons, "gates": gates, "latency": latency,
              "search_comparisons": [r for r in budget["results"] if r["method"] in ["timing_extra", "fitted_timing", "ridge_floor"]],
              "limits": "Reference costs are verified upper bounds. Conditional layout costs have different failure sets; paired CPU-budget comparisons and failure rates are also supplied. Timing excludes physical-contract construction and the original full economic forecast/warm compiler. Secondary frozen methods are not substituted for the original primary.",
              "future_use": "This validation wave may now be development data for a later candidate. Holdout_a and holdout_b remain undownloaded and unanalyzed.",
              "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [source_path, layout_path, budget_path]}}
    (output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: report[k] for k in ["accepted", "source_macro_mae_reduction_percent", "source_macro_mae_reduction_family_bootstrap_95", "gates", "latency"]}, indent=2))


if __name__ == "__main__":
    main()
