"""Apply the frozen ordinary cold-search gate to a completed C++ replay."""
import argparse
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(); parser.add_argument("report", type=Path)
    parser.add_argument("--scope", required=True); args = parser.parse_args()
    path, parity_path = args.report / "MEASURED_RESULTS.json", args.report / "REPORT.json"
    data = json.loads(path.read_text()); parity = json.loads(parity_path.read_text())
    before, after = "original_geometry/around", "cost_direct_extra/around"
    summaries = {r["method"]: r for r in data["summary"] if r["budget_cpu_seconds"] == 30}
    comparison = next(r for r in data["comparisons"] if r["before"] == before and r["after"] == after)
    a, b = summaries[before], summaries[after]
    reduction, interval = comparison["relative_time_reduction_percent"], comparison["seconds_saved_family_bootstrap_95"]
    gates = {"mean_time_reduction_at_least_20_percent": reduction is not None and reduction >= 20,
             "positive_family_interval_for_saved_time": interval is not None and interval[0] > 0,
             "no_worse_30s_certificate_rate": b["no_certificate_rate"] <= a["no_certificate_rate"],
             "no_worse_30s_mean_verified_bill": b["mean_conditional_cost"] is not None and a["mean_conditional_cost"] is not None and b["mean_conditional_cost"] <= a["mean_conditional_cost"],
             "exact_frozen_cpp_query_choices": parity["exact_frozen_query_choices"]}
    result = {"scope": args.scope, "baseline": before, "candidate": after, "comparison": comparison,
              "original_30s": a, "candidate_30s": b, "gates": gates, "passes": all(gates.values()),
              "limit": "Time to reference-best is retrospective; fixed-budget bill and certificate rate are separate. This does not accept the complete warm compiler or general marginal estimator.",
              "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [path, parity_path, Path(__file__)]}}
    with (args.report / "COLD_GATE.json").open("x") as stream: stream.write(json.dumps(result, indent=2, allow_nan=False) + "\n")
    print(json.dumps({"gates": gates, "passes": result["passes"]}, indent=2))


if __name__ == "__main__":
    main()
