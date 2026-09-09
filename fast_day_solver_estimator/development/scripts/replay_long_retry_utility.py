"""Value-of-compute development check restricted to known-certificate states."""
import hashlib
import json
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

from replay_long_retry import EXP, bill, compare, summarize


def replay(values, method, minimum_utility):
    prior = values[0]["prior_upper_workers"]
    assert prior is not None
    incumbent, cpu, successes, attempts = prior, 0., 0, []
    remaining = [{k: r[k] for k in ["sample_id", "workers", "probability", "predicted_cpu_seconds"]} for r in values]
    hidden = {r["sample_id"]: r for r in values}
    while remaining:
        eligible = [r for r in remaining if r["workers"] < incumbent]
        if not eligible:
            break
        def utility(row):
            return row["probability"] * (bill(incumbent) - bill(row["workers"])) / row["predicted_cpu_seconds"]
        row = max(eligible, key=lambda r: (utility(r), r["workers"]))
        if utility(row) < minimum_utility:
            break
        remaining.remove(row)
        result = hidden[row["sample_id"]]
        cpu += result["long_cpu_seconds"]
        attempts.append(row["sample_id"])
        if result["success"]:
            incumbent = row["workers"]
            successes += 1
    first = values[0]
    best = min([prior] + [r["workers"] for r in values if r["success"]])
    return {"state_id": first["round"] + "/" + first["physical_key"], "physical_key": first["physical_key"],
            "family": first["family"], "method": method, "minimum_utility": minimum_utility,
            "prior_upper_workers": prior, "best_recorded_bill": bill(best), "selected_samples": attempts,
            "calls": len(attempts), "cpu_seconds": cpu, "successful_calls": successes,
            "final_workers": incumbent, "final_bill": bill(incumbent)}


def main():
    source = EXP / "runs/long_retry_study_v1/PREDICTIONS.jsonl"
    first_replay = EXP / "runs/long_retry_replay_v1/DECISIONS.jsonl"
    methods = ["old_three_second_oof", "train_prior", "prior_only/logistic", "physical/boost", "with_prior/boost"]
    thresholds = [0., .001, .01, .1, 1., 10.]
    groups = defaultdict(list)
    for line in source.read_text().splitlines():
        row = json.loads(line)
        if row["prior_upper_workers"] is not None and row["method"] in methods:
            groups[row["method"], row["round"], row["physical_key"]].append(row)
    records = [replay(values, method, threshold) for (method, _, _), values in sorted(groups.items()) for threshold in thresholds]
    assert len(records) == 365 * len(methods) * len(thresholds)
    baseline = {r["state_id"]: r for r in map(json.loads, first_replay.read_text().splitlines())
                if r["method"] == "nearest_first" and r["call_allowance"] == 3 and r["cutoff"] == 0. and r["prior_upper_workers"] is not None}
    summary, comparisons = [], []
    for method in methods:
        for threshold in thresholds:
            selected = [r for r in records if r["method"] == method and r["minimum_utility"] == threshold]
            total_cpu = sum(r["cpu_seconds"] for r in selected)
            total_bill = sum(r["final_bill"] for r in selected)
            summary.append({"method": method, "minimum_utility": threshold, **summarize(selected),
                            "total_bill_plus_compute_value": total_bill + threshold * total_cpu})
            old = {r["state_id"]: r for r in records if r["method"] == "old_three_second_oof" and r["minimum_utility"] == threshold}
            for control, lookup in [("all_nearest", baseline), ("old_same_utility", old)]:
                comparisons.append({"method": method, "minimum_utility": threshold, "control": control, **compare(selected, lookup),
                                    "mean_bill_plus_compute_value_delta": sum(r["final_bill"] - lookup[r["state_id"]]["final_bill"] + threshold * (r["cpu_seconds"] - lookup[r["state_id"]]["cpu_seconds"]) for r in selected) / len(selected)})
    report = {"utc": datetime.now(timezone.utc).isoformat(),
              "scope": "Post hoc development whole-family-excluded retry choices on 365 known-certificate contract/round states. Cases without a certificate are outside this optional refinement policy, not silently rejected.",
              "objective": "Returned verified hire bill plus an explicitly varied value per CPU second. This is a tradeoff curve, not a no-quality-loss speed gate or an established user preference for compute price.",
              "policy": "Choose highest predicted success times immediate bill saving per predicted CPU; stop below the stated threshold. Update incumbent on observed success, leave initial forecasts fixed. At most the available one-to-three soft thirty-second calls; actual CPU is charged after each attempt.",
              "limits": "Model inference and outer-pipeline CPU are excluded. Family bootstrap is descriptive with only eight exposed families. Observed upper bills are not proven minima. The old probability control is also family-excluded.",
              "summary": summary, "comparisons": comparisons,
              "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [source, first_replay, Path(__file__), EXP / "scripts/replay_long_retry.py"]}}
    output = EXP / "runs/long_retry_utility_v1"; output.mkdir(exist_ok=False)
    (output / "RESULTS.json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    (output / "DECISIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in records))
    for name in ["replay_long_retry.py", "replay_long_retry_utility.py", "baseline_study.py"]:
        (output / name).write_bytes((EXP / "scripts" / name).read_bytes())
    print("method utility calls cpu known_bill_gap")
    for row in summary:
        print(row["method"], row["minimum_utility"], row["calls"], round(row["total_cpu_seconds"], 3), round(row["mean_known_gap_to_best_recorded"], 6))


if __name__ == "__main__":
    main()
