"""Check declared numerical component gates; keep pipeline claims separate."""
import argparse
import json
from pathlib import Path


def checks(row):
    return {
        "mean_margin_nonnegative": row["margin_gain"]["mean"] >= 0,
        "mean_utility_at_least_minus_quarter_point": row["utility_gain_pp"]["mean"] >= -0.25,
        "worst_decile_margin_loss_at_most_50":
            row["candidate"]["worst_decile_margin"] - row["reference"]["worst_decile_margin"] >= -50,
        "physical_states_match": row["both_match_original_physical_state"] == row["cases"],
        "no_added_faults": row["extra_own_faults"] == row["extra_rival_faults"] == 0,
        "no_production_stock_discard_changes": all(row[k] == 0 for k in (
            "produced_changes", "rival_produced_changes", "stock_changes", "rival_stock_changes",
            "discarded_changes", "rival_discarded_changes", "worker_days_changes")),
        "all_calendars_compiled": row["candidate"]["incomplete_calendars"] == row["reference"]["incomplete_calendars"] == 0,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("run", type=Path)
    args = parser.parse_args()
    comparison = json.loads((args.run / "VS_REFERENCE.json").read_text())
    own = json.loads((args.run / "SUMMARY.json").read_text())
    groups = {f"{kind}:{name}": checks(row) for kind, values in comparison["by"].items() for name, row in values.items()}
    global_checks = checks(comparison["all"]) | {
        "positive_margin_lower_bound": comparison["all"]["margin_gain"]["ci95"][0] > 0,
        "utility_lower_bound_at_least_minus_quarter_point": comparison["all"]["utility_gain_pp"]["ci95"][0] >= -0.25,
        "no_failed_jobs": own["failed_jobs"] == 0,
    }
    failures = {name: [key for key, passed in values.items() if not passed] for name, values in groups.items()}
    failures = {name: values for name, values in failures.items() if values}
    result = {
        "reference": comparison["reference_run"], "candidate": comparison["candidate_run"],
        "scope": "Declared numerical gates on this fixed live panel only. This does not establish unfamiliar source-family transfer or better plan selection.",
        "threshold_source": "PROTOCOL.md; utility thresholds converted from fractions to percentage points.",
        "global_checks": global_checks, "critical_groups": groups, "failed_groups": failures,
        "numerical_component_gates_pass": all(global_checks.values()) and not failures,
        "additional_policy_ms_over_paired_repair": own["all"]["history"]["mean_act_ms_per_game"] - own["all"]["repair"]["mean_act_ms_per_game"],
        "remaining_judgment": "Review prediction exceptions, complete compute cost and practical value. Fresh-seed status comes from the frozen run protocol.",
        "pipeline_promotion": False,
        "pipeline_limit": "Improved composition/plan selection and physically compatible opponent scenarios remain unproved.",
    }
    (args.run / "PROMOTION_AUDIT.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k != "critical_groups"}, indent=2))


if __name__ == "__main__":
    main()
