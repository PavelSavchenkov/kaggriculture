"""Record the fixed second-wave gate without changing model selection rules."""
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    root = EXP / "runs/holdout_a_gate_v2"; root.mkdir(exist_ok=False)
    freeze_path = EXP / "snapshots/search_wave_v2/FREEZE.json"
    freeze = json.loads(freeze_path.read_text())
    for name, item in freeze["files"].items():
        path = freeze_path.parent / name
        assert hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"], name
    path = EXP / "runs/holdout_a_layout_novel_evaluation_v2/MEASURED_RESULTS.json"
    data = json.loads(path.read_text())
    parity_path = path.parent / "REPORT.json"; parity = json.loads(parity_path.read_text())
    summaries = {r["method"]: r for r in data["summary"] if r["budget_cpu_seconds"] == 30}
    pairs = {(r["before"], r["after"]): r for r in data["comparisons"]}
    before, after = "original_geometry/around", "cost_direct_extra/around"
    comparison = pairs[before, after]; a, b = summaries[before], summaries[after]
    gates = {"mean_time_reduction_at_least_20_percent": comparison["relative_time_reduction_percent"] >= 20,
        "positive_family_interval_for_saved_time": comparison["seconds_saved_family_bootstrap_95"][0] > 0,
        "no_worse_30s_certificate_rate": b["no_certificate_rate"] <= a["no_certificate_rate"],
        "no_worse_30s_mean_verified_bill": b["mean_conditional_cost"] <= a["mean_conditional_cost"],
        "exact_frozen_cpp_query_choices": parity["exact_frozen_query_choices"]}
    optional = pairs[after, "boost/hybrid_nominal"]
    alternative = summaries["boost/hybrid_nominal"]
    optional_gates = {"time_gain_at_least_5_percent": optional["relative_time_reduction_percent"] >= 5,
        "positive_saved_time_interval": optional["seconds_saved_family_bootstrap_95"][0] > 0,
        "fixed_budget_quality_improves": alternative["no_certificate_rate"] < b["no_certificate_rate"] or alternative["mean_conditional_cost"] < b["mean_conditional_cost"],
        "time_regression_at_most_5_percent": optional["relative_time_reduction_percent"] >= -5}
    result = {"utc": datetime.now(timezone.utc).isoformat(),
        "scope": "Ordinary 24-phase, earliest-hire-menu cold layout search with equal-work/value candidates. This does not accept a general marginal estimator, terminal/calendar transfer or the original warm course pipeline.",
        "primary": after, "baseline": before, "novel_layout_pools": b["pools"], "primary_gates": gates,
        "primary_passes": all(gates.values()), "comparison": comparison,
        "optional_primary": "boost/hybrid_nominal", "optional_checks": optional_gates,
        "optional_passes": False,
        "optional_reason": "13.04 percent slower than direct-cost nearby ordering, with the same 30-second quality; fails both predeclared routes to adoption.",
        "secondary_hybrid_cpu": pairs[after, "boost/hybrid_cpu"],
        "secondary_reason": "1.30 percent mean gain with an interval crossing zero and no 30-second quality gain; insufficient evidence to adopt.",
        "adopted_scoped_component": "search_v2_cost_only_h24_cold" if all(gates.values()) else None,
        "whole_pipeline_accepted": False,
        "timing_limit": "Time to reference-best cost is retrospective. The caller cannot observe that stopping time. Fixed-budget quality is reported separately; no complete original warm-pipeline speedup is claimed.",
        "frozen_files_verified": len(freeze["files"]),
        "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [freeze_path, path, parity_path, Path(__file__)]}}
    (root / "GATE.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: result[k] for k in ["primary_gates", "primary_passes", "optional_passes", "adopted_scoped_component", "whole_pipeline_accepted"]}, indent=2))


if __name__ == "__main__":
    main()
