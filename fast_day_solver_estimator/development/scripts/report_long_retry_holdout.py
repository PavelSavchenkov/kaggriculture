"""Complete prospective known-certificate retry gate with actual C++ choices."""
import csv
import hashlib
import json
import subprocess
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

import numpy as np

from replay_long_retry import bill, compare, summarize


EXP = Path(__file__).resolve().parents[1]
FAILED_CPU = 30.104672395368077
SUCCESSFUL_CPU = 7.652708758064516


def main():
    reference = EXP / "runs/holdout_b_long_retry_reference_v1"
    protocol_path = reference / "PROTOCOL.json"
    protocol = json.loads(protocol_path.read_text())
    status_path = reference / "long_retry_30s/STATUS.json"
    status = json.loads(status_path.read_text()); assert status["status"] == "completed"
    assert status["binary_sha256"] == protocol["reference_binary_sha256"]
    assert datetime.fromisoformat(protocol["utc"]) < datetime.fromisoformat(status["utc"])
    for path, expected in json.loads((EXP / "snapshots/long_retry_v1/FREEZE.json").read_text())["input_sha256"].items():
        assert hashlib.sha256((EXP / path).read_bytes()).hexdigest() == expected, path
    options_path = reference / "OPTIONS.jsonl"
    options = list(map(json.loads, options_path.read_text().splitlines()))
    outcomes = {}
    result_paths = sorted((reference / "long_retry_30s").glob("worker*/results.jsonl"))
    for path in result_paths:
        for line in path.read_text().splitlines(keepends=True):
            assert line.endswith("\n"), "incomplete result line in a completed sweep"
            row = json.loads(line); assert row["id"] not in outcomes
            outcomes[row["id"]] = row
    assert set(outcomes) == {r["query_id"] for r in options} and len(options) == protocol["queries"]
    profile_path = EXP / "runs/holdout_b_context_check_v3/CPP.csv"
    profile = {}
    for row in csv.DictReader(profile_path.open()):
        key = row["id"], int(row["workers"])
        assert key not in profile
        profile[key] = row
    groups = defaultdict(list)
    for option in options:
        result = outcomes[option["query_id"]]
        assert result["budget_seconds"] == 30 and result["workers"] == option["workers"]
        assert result["status"] in ["FEASIBLE", "UNKNOWN"]
        assert hashlib.sha256((EXP / option["query_path"]).read_bytes()).hexdigest() == option["query_input_sha256"]
        timing = profile[option["context_id"], option["workers"]]
        assert abs(float(timing["boost"]) - option["probability"]) < 1e-8
        groups[option["physical_key"]].append({**option, "success": result["status"] == "FEASIBLE", "backend_cpu": result["cpu_seconds"],
                                                "feature_cpu_us": float(timing["feature_cpu_us"]), "query_cpu_us": float(timing["query_cpu_us"])})
    assert len(groups) == protocol["cases"] and groups, "no eligible retry states"
    output = EXP / "runs/holdout_b_long_retry_evaluation_v1"; output.mkdir(exist_ok=False)
    methods = {"candidate_001": .01, "predictive_no_stop": 0.}
    expected, manifest = {}, []
    for index, (key, values) in enumerate(sorted(groups.items())):
        assert 1 <= len(values) <= 3 and len({r["workers"] for r in values}) == len(values)
        assert len({r["family"] for r in values}) == len({r["prior_upper_workers"] for r in values}) == len({r["context_id"] for r in values}) == 1
        for method, threshold in methods.items():
            identifier = str(index) + "_" + method; expected[identifier] = key, method
            fields = [identifier, values[0]["prior_upper_workers"], threshold, FAILED_CPU, SUCCESSFUL_CPU, len(values)]
            for row in values: fields += [row["workers"], row["probability"], int(row["success"]), row["backend_cpu"]]
            manifest.append(" ".join(map(str, fields)))
    (output / "INPUT.txt").write_text("\n".join(manifest) + "\n")
    binary = EXP / "build/replay_long_retry_cpp"
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                    str(binary), str(output / "INPUT.txt"), str(output / "CPP.csv"), "10"], cwd=EXP, check=True)
    cpp = defaultdict(list)
    for row in csv.DictReader((output / "CPP.csv").open()):
        assert row["id"] in expected
        cpp[row["id"]].append(row)
    assert set(cpp) == set(expected)
    records = []

    def record(values, method, choices, policy_cpu_us):
        prior = values[0]["prior_upper_workers"]
        selected = {r["workers"]: r for r in values}
        assert len(set(choices)) == len(choices) and set(choices) <= set(selected)
        current = prior
        for workers in choices:
            assert workers < current, "selected a dominated query"
            if selected[workers]["success"]: current = workers
        best = min([prior] + [r["workers"] for r in values if r["success"]])
        feature = max(r["feature_cpu_us"] for r in values)
        assert max(r["feature_cpu_us"] for r in values) - min(r["feature_cpu_us"] for r in values) < 1e-6
        score_cpu = (feature + sum(r["query_cpu_us"] for r in values)) / 1e6 if method != "nearest_all" else 0.
        backend = sum(selected[w]["backend_cpu"] for w in choices)
        initial_bill = bill(prior)
        workforce_band = next(f"{lo}-{hi}" for lo, hi in [(1, 4), (5, 8), (9, 12), (13, 20), (21, 40)] if lo <= prior <= hi)
        bill_band = "0-100" if initial_bill <= 100 else "101-1000" if initial_bill <= 1000 else "1001-10000" if initial_bill <= 10000 else "above10000"
        return {"state_id": values[0]["physical_key"], "physical_key": values[0]["physical_key"], "family": values[0]["family"],
                "panel": values[0]["panel"], "method": method, "prior_upper_workers": prior, "initial_bill": bill(prior),
                "initial_workforce_band": workforce_band, "initial_bill_band": bill_band,
                "best_recorded_bill": bill(best), "final_workers": current, "final_bill": bill(current), "calls": len(choices),
                "selected_workers": choices, "successful_calls": sum(selected[w]["success"] for w in choices),
                "backend_cpu_seconds": backend, "scoring_cpu_seconds": score_cpu, "policy_cpu_us": policy_cpu_us,
                "cpu_seconds": backend + score_cpu + policy_cpu_us / 1e6,
                "untried_successes": sum(r["success"] and r["workers"] not in choices for r in values),
                "untried_successes_better_than_final": sum(r["success"] and r["workers"] < current and r["workers"] not in choices for r in values)}

    for identifier, values in cpp.items():
        assert len(values) == 10 and {int(r["pass"]) for r in values} == set(range(10))
        assert len({(r["final_workers"], r["final_bill"], r["backend_cpu"], r["choices"]) for r in values}) == 1
        key, method = expected[identifier]
        choices = [int(w) for w in values[0]["choices"].split(":") if w]
        row = record(groups[key], method, choices, float(np.mean([float(r["policy_cpu_us"]) for r in values])))
        assert row["final_workers"] == int(values[0]["final_workers"]) and row["final_bill"] == float(values[0]["final_bill"])
        assert abs(row["backend_cpu_seconds"] - float(values[0]["backend_cpu"])) < 1e-10
        if method == "predictive_no_stop": assert row["final_bill"] == row["best_recorded_bill"]
        records.append(row)
    for values in groups.values():
        row = record(values, "nearest_all", sorted((r["workers"] for r in values), reverse=True), 0.)
        assert row["final_bill"] == row["best_recorded_bill"]
        records.append(row)
    summary, comparisons = [], []
    for method in [*methods, "nearest_all"]:
        selected = [r for r in records if r["method"] == method]
        for axis in ["all", "family", "panel", "initial_workforce_band", "initial_bill_band"]:
            values = ["all"] if axis == "all" else sorted({r[axis] for r in selected})
            for value in values:
                subset = selected if axis == "all" else [r for r in selected if r[axis] == value]
                summary.append({"method": method, "axis": axis, "value": value, **summarize(subset)})
        if method == "candidate_001":
            for control in ["predictive_no_stop", "nearest_all"]:
                baseline = {r["state_id"]: r for r in records if r["method"] == control}
                result = compare(selected, baseline)
                saving = 1 - sum(r["cpu_seconds"] for r in selected) / sum(r["cpu_seconds"] for r in baseline.values())
                gates = {"mean_cpu_saving_at_least_20_percent": saving >= .2,
                         "positive_family_cluster_saved_cpu_interval": result["family_bootstrap_95_cpu_saved"][0] > 0,
                         "no_higher_final_bill_in_any_state": result["common_bills_higher"] == 0}
                comparisons.append({"control": control, "cpu_saving_fraction": saving, **result, "gates": gates, "passes_gate": all(gates.values())})
    candidate = {r["physical_key"]: r for r in records if r["method"] == "candidate_001"}
    call_errors = []
    for key, values in groups.items():
        for row in values:
            p = row["probability"]; predicted_cpu = (1 - p) * FAILED_CPU + p * SUCCESSFUL_CPU
            call_errors.append({**{k: row[k] for k in ["query_id", "physical_key", "workers", "prior_upper_workers", "family", "panel", "probability", "success", "backend_cpu"]},
                                "signed_probability_error": p - int(row["success"]), "predicted_long_cpu": predicted_cpu,
                                "signed_cpu_error": predicted_cpu - row["backend_cpu"],
                                "selected_by_candidate": row["workers"] in candidate[key]["selected_workers"],
                                "successful_and_better_than_candidate_final": row["success"] and row["workers"] < candidate[key]["final_workers"]})
    calibration = {"scope": "Diagnostic application of the frozen three-second probability as a conditional thirty-second retry proxy; not a newly calibrated long-horizon model.",
                   "calls": len(call_errors), "brier": float(np.mean([r["signed_probability_error"] ** 2 for r in call_errors])),
                   "probability_bias": float(np.mean([r["signed_probability_error"] for r in call_errors])),
                   "cpu_mae": float(np.mean([abs(r["signed_cpu_error"]) for r in call_errors])),
                   "cpu_bias": float(np.mean([r["signed_cpu_error"] for r in call_errors]))}
    regressions = [r for r in records if r["method"] == "candidate_001" and r["final_bill"] > r["best_recorded_bill"]]
    report = {"utc": datetime.now(timezone.utc).isoformat(), "scope": "Prospective frozen optional refinement after a known short-sweep certificate. Only the fixed offered retry population is evaluated. Not a complete compiler benchmark or a feasibility claim.",
              "timing": "Recorded selected backend CPU plus per-input C++ feature and three-model query CPU (conservative for the one-model candidate), plus ten-replay mean C++ selection CPU. Nearest control gets zero selection overhead, conservatively favoring that control. JSON/parsing, outer compilation and independent reference verification are outside this replay.",
              "states": len(groups), "reference_calls": len(options), "reference_recoveries": sum(r["success"] for values in groups.values() for r in values),
              "summary": summary, "comparisons": comparisons, "conditional_proxy_calibration": calibration,
              "regressions": len(regressions), "passes_primary_gate": bool(comparisons) and all(r["passes_gate"] for r in comparisons),
              "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [protocol_path, status_path, options_path, profile_path, binary, Path(__file__), EXP / "scripts/replay_long_retry.py", *result_paths]}}
    (output / "RESULTS.json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    (output / "DECISIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in records))
    (output / "CALL_ERRORS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in call_errors))
    (output / "REGRESSIONS.json").write_text(json.dumps(regressions, indent=2) + "\n")
    print(json.dumps({"states": report["states"], "comparisons": comparisons, "passes_primary_gate": report["passes_primary_gate"]}, indent=2))


if __name__ == "__main__":
    main()
