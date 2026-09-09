"""Report complete paired warm-compiler CPU and certificate quality."""
import argparse
import csv
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("benchmark", type=Path)
    args = parser.parse_args()
    root = args.benchmark
    protocol = json.loads((root / "PROTOCOL.json").read_text())
    assert protocol["status"] == "completed", "do not report a partial benchmark"
    results = json.loads((root / "RESULTS.json").read_text())
    paired, by_course = [], defaultdict(list)
    guidance, common_cost, common_cash = [], [], []
    for case in results:
        modes = {r["mode"]: r for r in case["runs"]}
        assert set(modes) == {"original", "guided"}
        a, b = modes["original"], modes["guided"]
        row = {"id": case["id"], "spec": case["spec"], "repeat": case["repeat"],
               "original_cpu": a["cpu_seconds"], "guided_cpu": b["cpu_seconds"], "saved_cpu": a["cpu_seconds"] - b["cpu_seconds"],
               "original_wall": a["wall_seconds"], "guided_wall": b["wall_seconds"],
               "original_returncode": a["returncode"], "guided_returncode": b["returncode"],
               "original_status": a.get("compiler_status"), "guided_status": b.get("compiler_status")}
        if a["returncode"] == b["returncode"] == 0:
            row["hire_cost_delta"] = b["independent"]["hire_cost"] - a["independent"]["hire_cost"]
            row["own_cash_delta"] = b["independent"]["cash"] - a["independent"]["cash"]
            row["rival_cash_delta"] = b["independent"]["rival_cash"] - a["independent"]["rival_cash"]
            row["production_equal"] = all(a["independent"][key] == b["independent"][key] for key in ["produced0", "produced1"])
            common_cost.append(row["hire_cost_delta"]); common_cash.append(row["own_cash_delta"])
        for mode, run in modes.items():
            path = root / "jobs" / case["id"] / mode / "course/guidance.csv"
            if path.is_file():
                guidance.extend({"case": case["id"], **r} for r in csv.DictReader(path.open()))
        paired.append(row); by_course[case["spec"]].append(row)
    original = np.array([r["original_cpu"] for r in paired]); guided = np.array([r["guided_cpu"] for r in paired])
    courses = sorted(by_course); rng = np.random.default_rng(909716)
    samples = []
    for _ in range(10000):
        selected = [row for course in rng.choice(courses, len(courses), replace=True) for row in by_course[course]]
        old = np.mean([r["original_cpu"] for r in selected]); new = np.mean([r["guided_cpu"] for r in selected])
        samples.append((old - new, 1 - new / old))
    samples = np.array(samples)
    lost = sum(r["original_returncode"] == 0 and r["guided_returncode"] != 0 for r in paired)
    gained = sum(r["original_returncode"] != 0 and r["guided_returncode"] == 0 for r in paired)
    fraction = float(1 - guided.mean() / original.mean())
    interval = np.quantile(samples, [.025, .975], axis=0)
    gates = {"mean_cpu_saving_at_least_20_percent": fraction >= .2, "positive_course_cluster_interval": bool(interval[0, 0] > 0),
             "no_lost_certificates": lost == 0, "no_worse_mean_common_hire_bill": bool(common_cost) and bool(np.mean(common_cost) <= 0)}
    report = {"scope": protocol["scope"], "paired_runs": len(paired), "courses": len(courses),
              "original_mean_cpu": float(original.mean()), "guided_mean_cpu": float(guided.mean()),
              "cpu_saving_fraction": fraction, "course_cluster_95_saved_cpu": list(map(float, interval[:, 0])),
              "course_cluster_95_saving_fraction": list(map(float, interval[:, 1])),
              "original_mean_wall": float(np.mean([r["original_wall"] for r in paired])), "guided_mean_wall": float(np.mean([r["guided_wall"] for r in paired])),
              "original_failed_runs": sum(r["original_returncode"] != 0 for r in paired), "guided_failed_runs": sum(r["guided_returncode"] != 0 for r in paired),
              "lost_certificates": lost, "gained_certificates": gained, "common_successes": len(common_cost),
              "mean_hire_cost_delta_on_common_successes": float(np.mean(common_cost)) if common_cost else None,
              "worst_hire_cost_delta": max(common_cost, default=None), "mean_own_cash_delta_on_common_successes": float(np.mean(common_cash)) if common_cash else None,
              "worst_own_cash_delta": min(common_cash, default=None), "deferred_queries": sum(int(r["deferred"]) != 0 for r in guidance),
              "prediction_calls": len(guidance), "total_prediction_wall_seconds": sum(float(r["prediction_wall_seconds"]) for r in guidance),
              "gates": gates, "passes_fixed_development_gate": all(gates.values()),
              "caveats": "Finite-budget four-thread warm repair can vary between runs. These are exposed fixed scenarios; report quality and individual differences alongside mean speed. Independent verification is outside the timed compiler invocation.",
              "pairs": paired, "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [root / "PROTOCOL.json", root / "RESULTS.json", Path(__file__)]}}
    serialized = json.dumps(report, indent=2) + "\n"
    with (root / "REPORT.json").open("x") as stream: stream.write(serialized)
    print(json.dumps({k: v for k, v in report.items() if k not in ["pairs", "input_sha256"]}, indent=2))


if __name__ == "__main__":
    main()
