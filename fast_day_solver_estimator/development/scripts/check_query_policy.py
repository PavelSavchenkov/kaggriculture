"""Check every C++ query choice against the frozen policy, including failures."""
import argparse
import csv
import importlib.util
import json
import subprocess
from collections import defaultdict
from pathlib import Path

import numpy as np

from query_budget_study import run_policy, tie
from screening import screen_reason


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    args = parser.parse_args()
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    spec = importlib.util.spec_from_file_location("frozen_policy", EXP / "snapshots/search_wave_v2/policy/query_budget_study.py")
    frozen = importlib.util.module_from_spec(spec); spec.loader.exec_module(frozen)
    data = json.loads((EXP / "runs/layout_dataset_complete_v1/DATASET.json").read_text())
    pools = defaultdict(list)
    for row in data["rows"]:
        pools[row["pool"]].append(row)
    scores, forecasts = defaultdict(dict), defaultdict(dict)
    for row in map(json.loads, (EXP / "runs/cost_layout_study_v3/PREDICTIONS.jsonl").read_text().splitlines()):
        scores[row["method"]][row["id"]] = row["ranking_score"]
    for row in map(json.loads, (EXP / "runs/query_layout_study_v2/PREDICTIONS.jsonl").read_text().splitlines()):
        forecasts[row["method"]][row["query_id"]] = row
    policies = [("sequential", 0, "original_flat_10", "boost"), ("pruned", 1, "original_geometry", "boost"),
                ("around", 2, "cost_direct_extra", "boost"), ("global_descending", 3, "original_geometry", "boost"),
                ("query_nominal", 4, "original_geometry", "boost"), ("query_cpu", 4, "original_geometry", "logistic"),
                ("hybrid_nominal", 5, "cost_direct_extra", "boost"), ("hybrid_cpu", 5, "cost_direct_extra", "boost")]
    lines, expected = [], {}
    for pool, rows in sorted(pools.items()):
        candidate_ties = {row["id"]: i for i, row in enumerate(sorted(rows, key=lambda r: tie(r["id"])))}
        all_queries = [q for row in rows for q in row["query_outcomes"]]
        query_ties = {q["id"]: i for i, q in enumerate(sorted(all_queries, key=lambda q: tie(q["id"])))}
        for strategy, enum, ranking, model in policies:
            test_id = pool + "_" + strategy
            trace = []
            result = run_policy(rows, scores[ranking], forecasts[model], strategy, trace)
            assert result == frozen.run_policy(rows, scores[ranking], forecasts[model], strategy), "trace instrumentation changed the frozen policy"
            expected[test_id] = trace
            lines.append(f"{test_id} {enum} {int(strategy.endswith('_cpu'))} {int(model == 'logistic')} {len(rows)}\n")
            for row in rows:
                lines.append(f"{row['id']} {row['lower_bound']} {int(screen_reason(row) is not None)} {scores[ranking][row['id']]:.17g} {candidate_ties[row['id']]} {len(row['query_outcomes'])}\n")
                for q in row["query_outcomes"]:
                    f = forecasts[model].get(q["id"], {"predicted_success": 0, "predicted_cpu_seconds": 3})
                    lines.append(f"{q['workers']} {f['predicted_success']:.17g} {f['predicted_cpu_seconds']:.17g} {query_ties[q['id']]} {int(q['status'] == 'FEASIBLE')} {q['id']}\n")
    manifest = output / "INPUT.txt"; manifest.write_text("".join(lines))
    predictions = output / "CPP.csv"
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                    str(EXP / "build/check_query_policy"), str(manifest), str(predictions)], check=True)
    actual = defaultdict(list)
    rows = list(csv.DictReader(predictions.open()))
    for row in rows:
        actual[row["test"]].append(row["query"])
    for key, sequence in expected.items():
        assert actual.get(key, []) == sequence, (key, actual.get(key), sequence)
    times = [float(r["decision_us"]) for r in rows]
    report = {"cases": len(expected), "decisions": len(rows), "exact_query_order_matches": True,
              "frozen_policy_unchanged_by_trace_instrumentation": True,
              "decision_us_with_precomputed_forecasts": {"mean": float(np.mean(times)), "p95": float(np.quantile(times, .95))},
              "scope": "C++ policy logic receives held-family forecasts. Outcomes are supplied only after each selected query. Model numerical parity is tested separately."}
    (output / "REPORT.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
