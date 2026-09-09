"""Check observable stopping decisions and time their full C++ implementation."""
import argparse
import csv
import hashlib
import json
import subprocess
from collections import defaultdict
from pathlib import Path

import numpy as np

from baseline_study import cost
from query_budget_study import tie
from stopping_query_study import stopped_policy


EXP = Path(__file__).resolve().parents[1]
POLICIES = [("around_exhaustive", "exhaustive", 0, 0), ("first_certificate", "first_certificate", 1, 0)]
POLICIES += [(f"{strategy}_{cut:g}", strategy, enum, cut) for strategy, enum in [("around", 2), ("utility", 3)] for cut in [0, .1, 1, 10]]
POLICIES += [(f"{strategy}_{cut:g}", strategy, enum, cut) for strategy, enum, cuts in [("call_cap", 4, [1, 2, 4]), ("time_cap", 5, [3, 10])] for cut in cuts]


def main():
    parser = argparse.ArgumentParser()
    for name in ["dataset", "index", "ranking", "forecasts", "output"]:
        parser.add_argument(name, type=Path)
    parser.add_argument("--scope", required=True)
    args = parser.parse_args(); args.output.mkdir(exist_ok=False)
    data = json.loads(args.dataset.read_text()); assert not data.get("pending_cases")
    paths = {r["physical_key"]: EXP / r["problem"] for r in map(json.loads, args.index.read_text().splitlines())}
    ranking = {r["id"]: r["ranking_score"] for r in map(json.loads, args.ranking.read_text().splitlines()) if r["method"] == "cost_direct_extra"}
    forecasts = {r["query_id"]: r for r in map(json.loads, args.forecasts.read_text().splitlines()) if r["method"] == "boost"}
    pools = defaultdict(list)
    for row in data["rows"]:
        pools[row["pool"]].append(row)
    lines, expected, definitions = [], {}, []
    for pool, rows in sorted(pools.items()):
        assert len(rows) == 5 and len({tuple(r["families"]) for r in rows}) == 1
        candidate_ties = {r["id"]: i for i, r in enumerate(sorted(rows, key=lambda r: tie(r["id"])))}
        queries = [q for r in rows for q in r["query_outcomes"]]
        query_ties = {q["id"]: i for i, q in enumerate(sorted(queries, key=lambda q: tie(q["id"])))}
        labels = [r["short_sweep_workers"] for r in rows if r["short_sweep_workers"] is not None]
        reference_cost = float(cost([min(labels)])[0]) if labels else None
        for name, strategy, enum, cutoff in POLICIES:
            test = pool + "_" + name
            expected[test] = stopped_policy(rows, ranking, forecasts, strategy, cutoff)
            definitions.append({"test": test, "pool": pool, "method": name, "family": rows[0]["families"][0], "reference_cost": reference_cost})
            lines.append(f"{test} {enum} {cutoff} {len(rows)}\n")
            for row in rows:
                lines.append(f"{paths[row['physical_key']]} {candidate_ties[row['id']]} {len(row['query_outcomes'])}\n")
                for q in row["query_outcomes"]:
                    lines.append(f"{q['workers']} {query_ties[q['id']]} {int(q['status'] == 'FEASIBLE')} {q['cpu_seconds']:.17g} {q['id']}\n")
    manifest = args.output / "INPUT.txt"; manifest.write_text("".join(lines))
    (args.output / "DEFINITIONS.json").write_text(json.dumps(definitions, indent=2) + "\n")
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                    str(EXP / "build/replay_refinement_cpp"), str(manifest), str(args.output / "cpp")], check=True)
    actual = defaultdict(list)
    for row in csv.DictReader((args.output / "cpp/CALLS.csv").open()):
        actual[row["test"]].append(row)
    tests = {r["test"]: r for r in csv.DictReader((args.output / "cpp/TESTS.csv").open())}
    cases, budget_cases = [], []
    for definition in definitions:
        test = definition["test"]; calls = actual[test]
        assert [r["query"] for r in calls] == expected[test]["calls"], test
        successful = [r for r in calls if r["success"] == "1"]
        bill = float(cost([int(successful[-1]["best_workers"])])[0]) if successful else None
        assert bill == expected[test]["final_cost"]
        measurement = tests[test]
        cases.append({**definition, "final_cost": bill, "regret": bill - definition["reference_cost"] if bill is not None else None,
            "cpu_seconds": float(measurement["total_cpu_seconds"]), "calls": int(measurement["calls"]),
            "prediction_cpu_seconds": float(measurement["scoring_cpu_seconds"]) + float(measurement["decision_cpu_seconds"]),
            "forecasts": int(measurement["forecasts"])})
        for budget in [1, 3, 10, 30, 120]:
            completed = [r for r in successful if float(r["elapsed_cpu_seconds"]) <= budget]
            budget_cases.append({**definition, "budget": budget,
                "cost": float(cost([int(completed[-1]["best_workers"])])[0]) if completed else None})
    summary = []
    for name, _, _, _ in POLICIES:
        group = [r for r in cases if r["method"] == name]; valid = [r for r in group if r["final_cost"] is not None]
        summary.append({"method": name, "pools": len(group), "uncertified": len(group) - len(valid),
            "mean_terminated_cpu": float(np.mean([r["cpu_seconds"] for r in group])),
            "p95_terminated_cpu": float(np.quantile([r["cpu_seconds"] for r in group], .95)),
            "mean_prediction_cpu_us": float(np.mean([r["prediction_cpu_seconds"] for r in group]) * 1e6),
            "mean_verified_cost": float(np.mean([r["final_cost"] for r in valid])) if valid else None,
            "mean_regret": float(np.mean([r["regret"] for r in valid])) if valid else None,
            "max_regret": max((r["regret"] for r in valid), default=None)})
    report = {"scope": args.scope, "tests": len(expected), "query_choices": sum(len(r) for r in actual.values()),
        "exact_python_choice_parity": True, "summary": summary,
        "timing": "Actual C++ cost scoring, context construction, lazy query inference and stopping decisions plus unchanged recorded complete cold-call CPU. Parsing and output excluded; no original warm-pipeline claim.",
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.index, args.ranking, args.forecasts,
            Path(__file__), EXP / "include/refinement_policy.hpp", EXP / "build/replay_refinement_cpp"]}}
    (args.output / "REPORT.json").write_text(json.dumps(report, indent=2) + "\n")
    for name, rows in [("CASES", cases), ("BUDGET_CASES", budget_cases)]:
        (args.output / (name + ".jsonl")).write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in rows))
    print(json.dumps({"tests": len(expected), "query_choices": report["query_choices"], "parity": True}))


if __name__ == "__main__":
    main()
