"""Replay a common frozen compiler at actual CPU budgets, including scoring."""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np

from baseline_study import cost


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("layout_dataset", type=Path)
    parser.add_argument("study", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--prediction-us", type=float, default=100)
    args = parser.parse_args()
    args.output.mkdir(exist_ok=False)
    data = json.loads(args.layout_dataset.read_text())
    cases = {r["id"]: r for r in data["rows"]}
    pools = defaultdict(list)
    for p in map(json.loads, (args.study / "PREDICTIONS.jsonl").read_text().splitlines()):
        pools[(p["pool"], p["method"])].append(p)
    results = []
    for (pool, method), rows in sorted(pools.items()):
        if len(rows) != 5:
            continue
        known = [r for r in rows if r["workers"] is not None]
        if not known:
            continue
        oracle = min(float(cost([r["workers"]])[0]) for r in known)
        ordered = sorted((r for r in rows if not r["screened"]), key=lambda r: (
            r["ranking_score"], hashlib.sha256(("layout_tie_v0:" + r["id"]).encode()).hexdigest()))
        # Charge every modern method100 us per proposal as a conservative
        # first sensitivity point; favor original baselines with zero overhead.
        overhead = 0.0 if method.startswith("original_") else len(rows) * args.prediction_us / 1e6
        elapsed, best = overhead, None
        milestones = []
        for candidate in ordered:
            # Same cold backend for every estimator: all workforce counts,
            # decreasing order, with the recorded three-second V30 calls.
            for q in reversed(cases[candidate["id"]]["query_outcomes"]):
                elapsed += q["cpu_seconds"]
                if q["status"] == "FEASIBLE":
                    value = float(cost([q["workers"]])[0])
                    best = value if best is None else min(best, value)
                    milestones.append({"cpu_seconds": elapsed, "cost": best, "candidate": candidate["id"], "workers": q["workers"]})
        first_oracle = next((r["cpu_seconds"] for r in milestones if r["cost"] <= oracle), None)
        for budget in [10, 30, 60, 120, 240]:
            finished = [r for r in milestones if r["cpu_seconds"] <= budget]
            value = finished[-1]["cost"] if finished else None
            results.append({"pool": pool, "family": rows[0]["family"], "method": method, "budget_cpu_seconds": budget,
                            "cost": value, "oracle_cost": oracle, "regret": value - oracle if value is not None else None,
                            "time_to_oracle": first_oracle, "full_search_cpu_seconds": elapsed,
                            "prediction_cpu_seconds": overhead})
    groups = defaultdict(list)
    for row in results:
        groups[(row["method"], row["budget_cpu_seconds"])].append(row)
    summary = []
    for (method, budget), rows in sorted(groups.items()):
        valid = [r for r in rows if r["cost"] is not None]
        summary.append({"method": method, "budget_cpu_seconds": budget, "pools": len(rows),
                        "no_certificate_rate": 1 - len(valid) / len(rows),
                        "mean_conditional_cost": float(np.mean([r["cost"] for r in valid])) if valid else None,
                        "mean_conditional_regret": float(np.mean([r["regret"] for r in valid])) if valid else None,
                        "oracle_recall": sum(r["regret"] == 0 for r in valid) / len(rows),
                        "mean_time_to_oracle": float(np.mean([r["time_to_oracle"] for r in rows if r["time_to_oracle"] is not None]))})
    report = {"scope": "Replay of frozen independent cold V30 query outcomes and measured query CPU. This is a search proxy, not the full prior warm compiler or a whole-game improvement.",
              "budget_handling": "Only query completions within budget can supply certificates. A partially completed query contributes no schedule.",
              "inputs": "Physical contracts already constructed; input preparation is not included.",
              "modern_prediction_charge_us": args.prediction_us, "original_prediction_charge_us": 0,
              "summary": summary}
    (args.output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")
    (args.output / "CASES.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in results))
    for row in summary:
        if row["budget_cpu_seconds"] == 60:
            print(row["method"], "cost", row["mean_conditional_cost"], "failed", row["no_certificate_rate"],
                  "seconds_to_oracle", round(row["mean_time_to_oracle"], 3))


if __name__ == "__main__":
    main()
