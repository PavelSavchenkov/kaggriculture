"""Verify frozen query decisions and measure the actual C++ inference path."""
import argparse
import csv
import hashlib
import importlib.util
import json
import subprocess
from collections import defaultdict
from pathlib import Path

import numpy as np

from query_budget_study import run_policy, tie


EXP = Path(__file__).resolve().parents[1]
POLICIES = [
    ("original_flat_10/sequential", "original_flat_10", "sequential", 0, 2, "boost"),
    ("original_geometry/sequential", "original_geometry", "sequential", 0, 1, "boost"),
    ("original_geometry/around", "original_geometry", "around", 2, 1, "boost"),
    ("cost_direct_extra/around", "cost_direct_extra", "around", 2, 0, "boost"),
    ("cost_direct_extra/sequential", "cost_direct_extra", "sequential", 0, 0, "boost"),
    ("boost/hybrid_nominal", "cost_direct_extra", "hybrid_nominal", 5, 0, "boost"),
    ("boost/hybrid_cpu", "cost_direct_extra", "hybrid_cpu", 5, 0, "boost"),
    ("logistic/hybrid_cpu", "cost_direct_extra", "hybrid_cpu", 5, 0, "logistic"),
    ("boost/query_nominal", "original_geometry", "query_nominal", 4, 1, "boost"),
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("index", type=Path)
    parser.add_argument("predictions", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.mkdir(exist_ok=False)
    spec = importlib.util.spec_from_file_location("frozen_policy", EXP / "snapshots/search_wave_v2/policy/query_budget_study.py")
    frozen = importlib.util.module_from_spec(spec); spec.loader.exec_module(frozen)
    data = json.loads(args.dataset.read_text())
    assert not data["pending_cases"]
    index = [json.loads(line) for line in args.index.read_text().splitlines()]
    paths = {r["physical_key"]: EXP / r["problem"] for r in index}
    scores, forecasts, pools = defaultdict(dict), defaultdict(dict), defaultdict(list)
    for r in map(json.loads, (args.predictions / "PREDICTIONS.jsonl").read_text().splitlines()):
        scores[r["method"]][r["id"]] = r["ranking_score"]
    for r in map(json.loads, (args.predictions / "QUERY_PREDICTIONS.jsonl").read_text().splitlines()):
        forecasts[r["method"]][r["query_id"]] = r
    task_index = data["feature_names"].index("tasks")
    for row in data["rows"]:
        scores["original_geometry"][row["id"]] = row["original_geometry_workers"]
        scores["original_flat_10"][row["id"]] = 10 * row["features"][task_index]
        pools[row["pool"]].append(row)
    lines, expected, definitions = [], {}, []
    for pool, rows in sorted(pools.items()):
        assert pool and len(rows) == 5
        candidate_ties = {r["id"]: i for i, r in enumerate(sorted(rows, key=lambda r: tie(r["id"])))}
        queries = [q for row in rows for q in row["query_outcomes"]]
        query_ties = {q["id"]: i for i, q in enumerate(sorted(queries, key=lambda q: tie(q["id"])))}
        for policy, ranking, strategy, enum, cost_model, model in POLICIES:
            name = pool + "_" + policy.replace("/", "_")
            sequence = []
            result = run_policy(rows, scores[ranking], forecasts[model], strategy, sequence)
            assert result == frozen.run_policy(rows, scores[ranking], forecasts[model], strategy)
            expected[name] = sequence
            definitions.append({"test": name, "pool": pool, "policy": policy, "family": rows[0]["families"][0],
                                "available_certificates": sum(r["short_sweep_workers"] is not None for r in rows)})
            lines.append(f"{name} {enum} {cost_model} {int(strategy.endswith('_cpu'))} {int(model == 'logistic')} 5\n")
            for row in rows:
                lines.append(f"{paths[row['physical_key']]} {candidate_ties[row['id']]} {len(row['query_outcomes'])}\n")
                for q in row["query_outcomes"]:
                    lines.append(f"{q['workers']} {query_ties[q['id']]} {int(q['status'] == 'FEASIBLE')} {q['cpu_seconds']:.17g} {q['id']}\n")
    manifest = args.output / "INPUT.txt"; manifest.write_text("".join(lines))
    (args.output / "DEFINITIONS.json").write_text(json.dumps(definitions, indent=2) + "\n")
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                    str(EXP / "build/replay_search_cpp"), str(manifest), str(args.output / "cpp")], check=True)
    actual = defaultdict(list)
    calls = list(csv.DictReader((args.output / "cpp/CALLS.csv").open()))
    for row in calls:
        actual[row["test"]].append(row["query"])
    for test, sequence in expected.items():
        assert actual.get(test, []) == sequence, (test, actual.get(test), sequence)
    tests = list(csv.DictReader((args.output / "cpp/TESTS.csv").open()))
    times = np.array([float(r["scoring_cpu_seconds"]) / int(r["proposals"]) * 1e6 for r in tests])
    report = {"tests": len(expected), "calls": len(calls), "exact_frozen_query_choices": True,
        "scoring_cpu_us_per_proposal_mixed_methods": {"mean": float(times.mean()), "p95": float(np.quantile(times, .95))},
        "scope": "Real C++ physical features, bounds, model inference, lazy deferral and policy decisions. Cached reference outcomes are revealed only after each next-query decision. Parsing and CSV output are outside scoring and decision timers.",
        "all_no_certificate_pools": sorted({r["pool"] for r in definitions if r["available_certificates"] == 0}),
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.index,
            args.predictions / "PREDICTIONS.jsonl", args.predictions / "QUERY_PREDICTIONS.jsonl", EXP / "build/replay_search_cpp"]}}
    (args.output / "REPORT.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
