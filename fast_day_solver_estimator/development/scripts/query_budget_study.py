"""Replay adaptive query ordering against strong simple compiler controls."""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np

from baseline_study import cost
from screening import screen_reason


def tie(identifier):
    return hashlib.sha256(("layout_tie_v0:" + identifier).encode()).hexdigest()


def run_policy(rows, ranking, forecasts, strategy, record_calls=None):
    prices = cost(np.arange(1, 41))
    choices = [r for r in rows if not screen_reason(r)]
    queries = [{**q, "case": r["id"]} for r in choices for q in r["query_outcomes"]
               if strategy == "sequential" or q["workers"] >= r["lower_bound"]]
    # Every strategy pays for all candidate scoring. Query-model strategies
    # additionally pay 10 us per forecast. Replace these provisional charges
    # with measured complete C++ costs before an acceptance claim.
    overhead = len(rows) * .0001 + (len(queries) * .00001 if strategy.startswith("query_") else 0)
    elapsed, best, attempted, milestones = overhead, 41, set(), []
    switched = False
    deferral_time = None
    order = {r["id"]: (ranking[r["id"]], tie(r["id"])) for r in rows}
    while True:
        available = [q for q in queries if q["id"] not in attempted and (strategy == "sequential" or q["workers"] < best)]
        if not available:
            break
        if strategy in ["sequential", "pruned"]:
            query = min(available, key=lambda q: (order[q["case"]], -q["workers"]))
        elif strategy == "global_descending":
            query = min(available, key=lambda q: (-q["workers"], order[q["case"]]))
        elif strategy == "around" or (strategy.startswith("hybrid_") and not switched):
            query = min(available, key=lambda q: (order[q["case"]], abs(q["workers"] - ranking[q["case"]]), -q["workers"]))
        elif strategy.startswith("query_") or strategy.startswith("hybrid_"):
            def utility(q):
                f = forecasts[q["id"]]
                gain = 1 if best == 41 else prices[best - 1] - prices[q["workers"] - 1]
                seconds = max(.05, f["predicted_cpu_seconds"]) if strategy.endswith("_cpu") else 3
                return f["predicted_success"] * gain / seconds
            query = min(available, key=lambda q: (-utility(q), tie(q["id"])))
        else:
            raise ValueError(strategy)
        attempted.add(query["id"])
        if record_calls is not None:
            record_calls.append(query["id"])
        elapsed += query["cpu_seconds"]
        if query["status"] == "FEASIBLE":
            best = min(best, query["workers"])
            milestones.append({"seconds": elapsed, "workers": best, "cost": float(prices[best - 1]), "queries": len(attempted), "query_id": query["id"]})
        elif strategy.startswith("hybrid_") and not switched:
            switched = True
            deferral_time = elapsed
            charge = sum(q["id"] not in attempted and q["workers"] < best for q in queries) * .00001
            overhead += charge; elapsed += charge
    return milestones, elapsed, len(attempted), overhead, deferral_time


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("query_study", type=Path)
    parser.add_argument("ranking_study", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.mkdir(exist_ok=False)
    data = json.loads(args.dataset.read_text()); pools = defaultdict(list)
    for row in data["rows"]:
        pools[row["pool"]].append(row)
    scores = defaultdict(dict)
    for row in map(json.loads, (args.ranking_study / "PREDICTIONS.jsonl").read_text().splitlines()):
        scores[row["method"]][row["id"]] = row["ranking_score"]
    forecasts = defaultdict(dict)
    for row in map(json.loads, (args.query_study / "PREDICTIONS.jsonl").read_text().splitlines()):
        forecasts[row["method"]][row["query_id"]] = row
    policies = []
    for model in ["original_flat_10", "original_geometry", "ridge_floor", "timing_extra", "cost_direct_extra"]:
        if model not in scores:
            continue
        for strategy in ["sequential", "pruned", "global_descending"] + ([] if model == "original_flat_10" else ["around"]):
            policies.append((model + "/" + strategy, model, strategy, None))
    for model in forecasts:
        for strategy in ["query_nominal", "query_cpu"]:
            policies.append((model + "/" + strategy, "original_geometry", strategy, model))
        for strategy in ["hybrid_nominal", "hybrid_cpu"]:
            policies.append((model + "/" + strategy, "cost_direct_extra", strategy, model))
    results, traces = [], []
    for pool, rows in sorted(pools.items()):
        assert len(rows) == 5 and len({tuple(r["families"]) for r in rows}) == 1
        certified = [r["short_sweep_workers"] for r in rows if r["short_sweep_workers"] is not None]
        if not certified:
            continue
        best_workers = min(certified); oracle = float(cost([best_workers])[0])
        for policy, ranking, strategy, model in policies:
            milestones, total_cpu, calls, overhead, deferral_time = run_policy(rows, scores[ranking], forecasts[model] if model else {}, strategy)
            reached = next((r for r in milestones if r["workers"] <= best_workers), None)
            assert reached is not None, "a policy exhausted logged queries without retaining the best certificate"
            traces.append({"pool": pool, "policy": policy, "milestones": milestones, "deferral_seconds": deferral_time})
            for budget in [1, 3, 10, 30, 60, 120, 240]:
                completed = [r for r in milestones if r["seconds"] <= budget]
                best = completed[-1] if completed else None
                results.append({"pool": pool, "family": rows[0]["families"][0], "method": policy, "budget_cpu_seconds": budget,
                    "cost": best["cost"] if best else None, "oracle_cost": oracle, "regret": best["cost"] - oracle if best else None,
                    "time_to_oracle": reached["seconds"], "queries_to_oracle": reached["queries"], "full_search_cpu_seconds": total_cpu,
                    "calls": calls, "prediction_cpu_seconds": overhead, "deferral_seconds": deferral_time,
                    "deferred_within_budget": deferral_time is not None and deferral_time <= budget,
                    "deferred_before_best_reference": deferral_time is not None and deferral_time <= reached["seconds"]})
    groups = defaultdict(list)
    for row in results:
        groups[row["method"], row["budget_cpu_seconds"]].append(row)
    summaries = []
    for (method, budget), rows in sorted(groups.items()):
        valid = [r for r in rows if r["cost"] is not None]
        summary = {"method": method, "budget_cpu_seconds": budget, "pools": len(rows),
            "no_certificate_rate": 1 - len(valid) / len(rows), "mean_conditional_cost": float(np.mean([r["cost"] for r in valid])) if valid else None,
            "mean_conditional_regret": float(np.mean([r["regret"] for r in valid])) if valid else None,
            "oracle_recall": sum(r["regret"] == 0 for r in valid) / len(rows),
            "mean_time_to_oracle": float(np.mean([r["time_to_oracle"] for r in rows])),
            "mean_queries_to_oracle": float(np.mean([r["queries_to_oracle"] for r in rows])),
            "deferral_fraction_within_budget": float(np.mean([r["deferred_within_budget"] for r in rows])),
            "deferral_fraction_before_best_reference": float(np.mean([r["deferred_before_best_reference"] for r in rows]))}
        summaries.append(summary)
        if budget == 30:
            print(method, "cost", summary["mean_conditional_cost"], "fail", summary["no_certificate_rate"], "time_to_best", summary["mean_time_to_oracle"])
    report = {"scope": "Exposed development cold compiler proxy with held-family predictions. This does not reproduce the original warm course compiler.",
        "controls": "Literal sequential calls, dominated-cost pruning, workforce-first order and nearest-predicted-workforce order. All adaptive controls also skip worker counts below each candidate's necessary bound. They use the same frozen query pool and exact recorded CPU.",
        "decision_information": "The next query uses predictions, already completed outcomes and known hire prices. It never reads an unqueried success or actual duration.",
        "timing_limit": "100 us per proposal and 10 us per query forecast are provisional charges; no input construction, JSON or full original pipeline timing is included.",
        "cpu_predictor": "The first CPU regressor predicts geometric mean time from a log target; query_cpu is a heuristic, not an expected-time proof.",
        "hybrid": "Use direct-cost ranking and nearby workforce queries until the first observed compiler failure, then pay for and use the query model. No unqueried outcome triggers the switch.",
        "budget_handling": "Only complete calls within a budget yield a certificate. No result is credited from a call crossing the deadline.",
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.query_study / "PREDICTIONS.jsonl", args.ranking_study / "PREDICTIONS.jsonl"]},
        "summary": summaries}
    (args.output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")
    (args.output / "CASES.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in results))
    (args.output / "TRACES.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in traces))


if __name__ == "__main__":
    main()
