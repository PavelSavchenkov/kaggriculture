"""Audit report arithmetic and reproduce an existing complete calendar report."""
import hashlib
import json
import subprocess
from pathlib import Path

from report_calendar_queries import paired_brier
from report_marginal_gate_v3 import compare as compare_marginal
from report_planning_calendars import summarize as summarize_cost
from report_planning_risk import summarize as summarize_risk


EXP = Path(__file__).resolve().parents[1]


def main():
    output = EXP / "runs/planning_report_reader_check_v4"; output.mkdir(exist_ok=False)
    rows = [{"id": str(i), "target": target, "prediction": prediction, "families": [family]}
            for i, (target, prediction, family) in enumerate([(5, 3, "a"), (5, 7, "a"), (10, 11, "b"), (40, 41, "b"), (None, 5, "a"), (None, None, "b")])]
    accuracy = summarize_cost(rows)
    assert accuracy["contracts"] == 6 and accuracy["labeled"] == 4 and accuracy["unknown"] == 2
    assert accuracy["scored"] == 4 and accuracy["rejected"] == 1 and accuracy["finite_cash_predictions"] == 3
    assert accuracy["workforce_overflow_markers"] == 1 and accuracy["worker_mae"] == 1.5
    assert accuracy["under_by_at_least_2_workers"] == .25 and accuracy["over_by_at_least_2_workers"] == .25
    marginal = []
    for family, errors in [("a", [(-500, -500)]), ("b", [(100, 50), (100, 50), (100, 50)])]:
        for i, (old, new) in enumerate(errors):
            for method, error in [("original_flat_10", old), ("candidate", new)]:
                marginal.append({"family": family, "pool": family, "candidate": str(i), "method": method, "mode": "cold", "target": 0, "error": error})
    marginal_result = compare_marginal(marginal, "candidate")
    assert marginal_result["baseline_family_macro_cash_mae"] == 300
    assert marginal_result["candidate_family_macro_cash_mae"] == 275
    assert marginal_result["baseline_under_by_at_least_500"] == .25
    assert not marginal_result["passes"]
    risk_rows = [{"families": ["a" if i < 2 else "b"], "cash_error": error, "reference_workers": None if error is None else 10,
                  "rejected": i == 3, "probe_available": i != 3, "flag": flag}
                 for i, (error, flag) in enumerate([(-500, True), (-501, False), (500, True), (None, False)])]
    risk = summarize_risk(risk_rows, "flag", "under_500")
    assert risk["contracts"] == 4 and risk["unknown_contracts"] == 1 and risk["analytically_rejected"] == 1
    assert risk["severe_errors"] == 2 and risk["captured_severe_errors"] == 1 and risk["flagged_fraction"] == .5
    assert risk["severe_error_recall"] == .5 and not risk["passes_point_capture_gate"]
    assert paired_brier([], "a", "b")["calls"] == 0
    dataset_path = EXP / "runs/calendar_complete_initial_v2/DATASET.json"
    data = json.loads(dataset_path.read_text()); cases = {r["contract_key"]: r for r in data["rows"]}
    prediction_path = EXP / "runs/context_calendar_augmented_v2/PREDICTIONS.jsonl"
    predictions = []
    for row in map(json.loads, prediction_path.read_text().splitlines()):
        case = cases[row["contract_key"]]
        predictions.append({"query_id": f"{case['id']}_w{row['workers']:02}", "method": row["method"], "workers": row["workers"],
                            "predicted_success": row["predicted_success"], "predicted_cpu_seconds": row["predicted_cpu_seconds"]})
    (output / "PREDICTIONS.jsonl").write_text("".join(json.dumps(r) + "\n" for r in predictions))
    (output / "NOVELTY.jsonl").write_text("".join(json.dumps({"contract_key": k, "novel": False}) + "\n" for k in cases))
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python", str(EXP / "scripts/report_calendar_queries.py"),
                    str(dataset_path), str(output / "PREDICTIONS.jsonl"), str(output / "NOVELTY.jsonl"), str(output / "query_report"),
                    "--scope", "Reader regression only: reproduce the existing completed development family-exclusion report; no new data or model."], cwd=EXP, check=True)
    report = json.loads((output / "query_report/RESULTS.json").read_text())
    reproduced = {r["method"]: r for r in report["summary"] if r["scope"] == "full" and r["axis"] == "all"}
    source = json.loads((EXP / "runs/context_calendar_augmented_v2/RESULTS.json").read_text())
    expected = {r["method"]: r for r in source["summary"] if r["axis"] == "all"}
    for method, row in reproduced.items():
        assert row["calls"] == 4442
        assert abs(row["brier"] - expected[method]["brier"]) < 1e-12
    result = {"scope": "Arithmetic boundary checks and exact reproduction of completed development query Brier metrics.",
              "accuracy": accuracy, "marginal": marginal_result, "risk": risk, "query_brier_parity": True,
              "methods": sorted(reproduced), "calls_per_method": 4442,
              "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [dataset_path, prediction_path, Path(__file__)]}}
    (output / "CHECK.json").write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
    print(json.dumps({k: result[k] for k in ["scope", "query_brier_parity", "methods", "calls_per_method"]}, indent=2))


if __name__ == "__main__":
    main()
