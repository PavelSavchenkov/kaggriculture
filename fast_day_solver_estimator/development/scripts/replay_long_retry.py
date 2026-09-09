"""Budgeted development decisions after an observed three-second failure."""
import hashlib
import json
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

import numpy as np

from baseline_study import cost


EXP = Path(__file__).resolve().parents[1]
PRICES = cost(np.arange(1, 41))


def bill(workers):
    assert 1 <= workers <= 40
    return float(PRICES[workers - 1])


def choose(remaining, incumbent, method, cutoff):
    eligible = [r for r in remaining if incumbent is None or r["workers"] < incumbent]
    if method == "nearest_first":
        return max(eligible, key=lambda r: r["workers"], default=None)
    if method == "cheapest_first":
        return min(eligible, key=lambda r: r["workers"], default=None)
    eligible = [r for r in eligible if r["predictions"][method]["probability"] >= cutoff]

    def value(row):
        prediction = row["predictions"][method]
        gain = bill(incumbent) - bill(row["workers"]) if incumbent is not None else 1.
        return prediction["probability"] * gain / prediction["predicted_cpu_seconds"], row["workers"]

    return max(eligible, key=value, default=None)


def replay(rows, method, cap, cutoff):
    # This is the complete selection interface: outcomes, timing labels, family
    # and query provenance cannot enter choose(). State updates reveal only the
    # result of a selected call. Probabilities remain fixed at initial state.
    remaining = [{k: r[k] for k in ["sample_id", "workers", "predictions"]} for r in rows]
    hidden = {r["sample_id"]: r for r in rows}
    incumbent = rows[0]["prior_upper_workers"]
    attempts, cpu, recovered = [], 0., 0
    for _ in range(cap):
        selected = choose(remaining, incumbent, method, cutoff)
        if selected is None:
            break
        remaining.remove(selected)
        result = hidden[selected["sample_id"]]
        attempts.append(result["sample_id"])
        cpu += result["long_cpu_seconds"]
        if result["success"]:
            assert incumbent is None or result["workers"] < incumbent
            incumbent = result["workers"]
            recovered += 1
    return {"selected_samples": attempts, "calls": len(attempts), "cpu_seconds": cpu,
            "successful_calls": recovered, "final_workers": incumbent,
            "final_bill": bill(incumbent) if incumbent is not None else None}


def summarize(rows):
    certified = [r for r in rows if r["final_bill"] is not None]
    known = [r for r in rows if r["prior_upper_workers"] is not None]
    unknown = [r for r in rows if r["prior_upper_workers"] is None]
    comparable = [r for r in rows if r["final_bill"] is not None and r["best_recorded_bill"] is not None]
    return {"states": len(rows), "physical_contracts": len({r["physical_key"] for r in rows}),
            "families": len({r["family"] for r in rows}), "calls": sum(r["calls"] for r in rows),
            "total_cpu_seconds": sum(r["cpu_seconds"] for r in rows),
            "mean_cpu_seconds": float(np.mean([r["cpu_seconds"] for r in rows])),
            "final_certificates": len(certified), "initially_certified": len(known),
            "new_certificates": sum(r["final_bill"] is not None for r in unknown),
            "missed_recorded_first_certificates": sum(r["final_bill"] is None and r["best_recorded_bill"] is not None for r in unknown),
            "known_states_improved": sum(r["final_workers"] < r["prior_upper_workers"] for r in known),
            "mean_known_bill": float(np.mean([r["final_bill"] for r in known])) if known else None,
            "mean_known_saving": float(np.mean([bill(r["prior_upper_workers"]) - r["final_bill"] for r in known])) if known else None,
            "mean_known_gap_to_best_recorded": float(np.mean([r["final_bill"] - r["best_recorded_bill"] for r in known])) if known else None,
            "mean_gap_to_best_recorded_on_certified": float(np.mean([r["final_bill"] - r["best_recorded_bill"] for r in comparable])) if comparable else None}


def compare(rows, baseline):
    pairs = [(r, baseline[r["state_id"]]) for r in rows]
    common = [(r, b) for r, b in pairs if r["final_bill"] is not None and b["final_bill"] is not None]
    families = sorted({r["family"] for r in rows})
    blocks = np.array([[sum(b["cpu_seconds"] - r["cpu_seconds"] for r, b in pairs if r["family"] == f),
                        sum(r["family"] == f for r in rows),
                        sum(r["final_bill"] - b["final_bill"] for r, b in common if r["family"] == f),
                        sum(r["family"] == f for r, _ in common)] for f in families], dtype=float)
    rng = np.random.default_rng(909925)
    draws = blocks[rng.integers(len(families), size=(10000, len(families)))].sum(axis=1)
    bill_draws = draws[draws[:, 3] > 0]
    bill_deltas = [r["final_bill"] - b["final_bill"] for r, b in common]
    return {"states": len(rows), "mean_cpu_saved": float(np.mean([b["cpu_seconds"] - r["cpu_seconds"] for r, b in pairs])),
            "family_bootstrap_95_cpu_saved": list(map(float, np.quantile(draws[:, 0] / draws[:, 1], [.025, .975]))),
            "lost_certificates": sum(r["final_bill"] is None and b["final_bill"] is not None for r, b in pairs),
            "gained_certificates": sum(r["final_bill"] is not None and b["final_bill"] is None for r, b in pairs),
            "common_certificates": len(common), "mean_common_bill_delta": float(np.mean(bill_deltas)) if common else None,
            "family_bootstrap_95_common_bill_delta": list(map(float, np.quantile(bill_draws[:, 2] / bill_draws[:, 3], [.025, .975]))) if len(bill_draws) else None,
            "common_bills_higher": sum(x > 0 for x in bill_deltas), "common_bills_lower": sum(x < 0 for x in bill_deltas),
            "worst_common_bill_increase": max(bill_deltas, default=None), "best_common_bill_decrease": min(bill_deltas, default=None)}


def main():
    source = EXP / "runs/long_retry_dataset_v1/DATASET.json"
    forecasts = EXP / "runs/long_retry_study_v1/PREDICTIONS.jsonl"
    study = EXP / "runs/long_retry_study_v1/RESULTS.json"
    data = json.loads(source.read_text()); rows = data["rows"]
    predictions = defaultdict(dict)
    for line in forecasts.read_text().splitlines():
        row = json.loads(line)
        assert row["method"] not in predictions[row["sample_id"]]
        predictions[row["sample_id"]][row["method"]] = {k: row[k] for k in ["probability", "predicted_cpu_seconds"]}
    folds = {r["family"]: r for r in json.loads(study.read_text())["folds"]}
    groups = defaultdict(list)
    for row in rows:
        row["predictions"] = predictions[row["sample_id"]]
        fold = folds[row["families"][0]]
        probability = row["predictions"]["old_three_second_oof"]["probability"]
        if row["prior_upper_workers"] is None:
            probability *= .1
        row["predictions"]["old_corrected_no_certificate"] = {
            "probability": probability,
            "predicted_cpu_seconds": (1 - probability) * fold["training_mean_failed_cpu"] + probability * fold["training_mean_success_cpu"]}
        groups[row["round"], row["physical_key"]].append(row)
    methods = ["nearest_first", "cheapest_first", "old_three_second_oof", "old_corrected_no_certificate", "train_prior", "prior_only/logistic", "physical/boost", "with_prior/boost"]
    specifications = [(method, cap, 0.) for method in methods for cap in [1, 2, 3]]
    specifications += [(method, cap, .02) for method in ["old_three_second_oof", "old_corrected_no_certificate", "with_prior/boost"] for cap in [1, 2, 3]]
    records = []
    for (round_name, key), values in sorted(groups.items()):
        assert len({r["workers"] for r in values}) == len(values) <= 3
        assert len({r["prior_upper_workers"] for r in values}) == len({r["families"][0] for r in values}) == 1
        prior = values[0]["prior_upper_workers"]
        assert prior is None or all(r["workers"] < prior for r in values)
        feasible = [r["workers"] for r in values if r["success"]] + ([prior] if prior is not None else [])
        best = min(feasible, default=None)
        state_id = round_name + "/" + key
        for method, cap, cutoff in specifications:
            result = replay(values, method, cap, cutoff)
            if cap == 3 and cutoff == 0.:
                assert result["final_workers"] == best
            records.append({"state_id": state_id, "physical_key": key, "round": round_name,
                            "family": values[0]["families"][0], "panel": values[0]["panel"], "phase": values[0]["phase"],
                            "prior_upper_workers": prior, "best_recorded_bill": bill(best) if best is not None else None,
                            "method": method, "call_allowance": cap, "cutoff": cutoff, **result})
    summary, comparisons = [], []
    for method, cap, cutoff in specifications:
        selected = [r for r in records if (r["method"], r["call_allowance"], r["cutoff"]) == (method, cap, cutoff)]
        for axis in ["all", "family", "panel", "phase", "has_prior_certificate"]:
            slices = defaultdict(list)
            for row in selected:
                value = "all" if axis == "all" else str(row["prior_upper_workers"] is not None) if axis == "has_prior_certificate" else row[axis]
                slices[value].append(row)
            for value, subset in sorted(slices.items()):
                summary.append({"method": method, "call_allowance": cap, "cutoff": cutoff, "axis": axis, "value": value, **summarize(subset)})
        for control in ["nearest_first", "old_three_second_oof"]:
            baseline = {r["state_id"]: r for r in records if (r["method"], r["call_allowance"], r["cutoff"]) == (control, cap, 0.)}
            comparisons.append({"method": method, "control": control, "call_allowance": cap, "cutoff": cutoff, **compare(selected, baseline)})
    result = {"utc": datetime.now(timezone.utc).isoformat(), "scope": "Development replay with whole-family-excluded probabilities. Separate contract/round states; repeated physical contracts remain in one bootstrap family. Not prospective or full-pipeline evidence.",
              "budget": "One, two or three soft thirty-second backend call allowances, with recorded actual CPU reported. These are not hard 30/60/90 CPU-second budgets. Feature/model overhead is not measured for this candidate and is excluded.",
              "selection": "Known certificate: predicted success times immediate hire-bill saving divided by predicted CPU. No certificate: predicted success divided by predicted CPU. Refresh the incumbent after observed success; leave probabilities fixed at initial state. Workforce controls order nearest/highest or cheapest/lowest first. Dominated workforces are skipped after success.",
              "cheap_correction": "Post hoc development rule: multiply old probability by 0.1 if no certificate is known. It changes calibration and threshold choices, not within-state ordering without a cutoff.",
              "outcome_limit": "Only the one-to-three previously selected retry options are observed; no unseen workforce outcome is imputed. UNKNOWN is unresolved. Best recorded is an outcome-based diagnostic, never a policy input.",
              "states": len(groups), "calls": len(rows), "summary": summary, "comparisons": comparisons,
              "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [source, forecasts, study, Path(__file__)]}}
    output = EXP / "runs/long_retry_replay_v1"; output.mkdir(exist_ok=False)
    (output / "RESULTS.json").write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
    (output / "DECISIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in records))
    print(json.dumps([r for r in summary if r["axis"] == "all" and r["cutoff"] == 0.], indent=2))


if __name__ == "__main__":
    main()
