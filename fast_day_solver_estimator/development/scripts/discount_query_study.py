"""Development-only cost penalties before a hybrid's first certificate."""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np

from baseline_study import cost
from improvement_report import bootstrap
from query_budget_study import run_policy, tie
from screening import screen_reason


def discounted_policy(rows, ranking, forecasts, scale_kind, use_cpu):
    prices = cost(np.arange(1, 41))
    candidates = [r for r in rows if not screen_reason(r)]
    outcomes = {q["id"]: q for row in candidates for q in row["query_outcomes"]}
    queries = [{"id": q["id"], "workers": q["workers"], "case": row["id"]}
               for row in candidates for q in row["query_outcomes"] if q["workers"] >= row["lower_bound"]]
    scale = max(1.0, min(cost([ranking[r["id"]]])[0] for r in candidates)) if scale_kind == "predicted" else float(scale_kind)
    elapsed, best, switched = len(rows) * .0001, 41, False
    attempted, milestones = set(), []
    order = {r["id"]: (ranking[r["id"]], tie(r["id"])) for r in rows}
    while True:
        available = [q for q in queries if q["id"] not in attempted and q["workers"] < best]
        if not available:
            break
        if not switched:
            chosen = min(available, key=lambda q: (order[q["case"]], abs(q["workers"] - ranking[q["case"]]), -q["workers"]))
        else:
            def utility(q):
                f = forecasts[q["id"]]
                gain = 1 / (1 + prices[q["workers"] - 1] / scale) if best == 41 else prices[best - 1] - prices[q["workers"] - 1]
                seconds = max(.05, f["predicted_cpu_seconds"]) if use_cpu else 3
                return f["predicted_success"] * gain / seconds
            chosen = min(available, key=lambda q: (-utility(q), tie(q["id"])))
        # Outcome and actual duration are read only after choosing the query.
        query = outcomes[chosen["id"]]; attempted.add(chosen["id"])
        elapsed += query["cpu_seconds"]
        if query["status"] == "FEASIBLE":
            best = min(best, query["workers"])
            milestones.append({"seconds": elapsed, "workers": best, "cost": float(prices[best - 1]), "queries": len(attempted), "query_id": query["id"]})
        elif not switched:
            switched = True
            elapsed += sum(q["id"] not in attempted and q["workers"] < best for q in queries) * .00001
    return milestones


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("ranking", type=Path)
    parser.add_argument("forecasts", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--scope", required=True)
    args = parser.parse_args()
    args.output.mkdir(exist_ok=False)
    data = json.loads(args.dataset.read_text()); assert not data.get("pending_cases")
    pools = defaultdict(list)
    for row in data["rows"]: pools[row["pool"]].append(row)
    ranking = {r["id"]: r["ranking_score"] for r in map(json.loads, args.ranking.read_text().splitlines()) if r["method"] == "cost_direct_extra"}
    forecasts = {r["query_id"]: r for r in map(json.loads, args.forecasts.read_text().splitlines()) if r["method"] == "boost"}
    cases, traces, unresolved = [], [], []
    for pool, rows in sorted(pools.items()):
        assert len(rows) == 5
        labels = [r["short_sweep_workers"] for r in rows if r["short_sweep_workers"] is not None]
        if not labels:
            unresolved.append(pool); continue
        best = min(labels)
        policies = {strategy: run_policy(rows, ranking, forecasts, strategy)[0] for strategy in ["around", "hybrid_nominal", "hybrid_cpu"]}
        for scale in ["10", "100", "1000", "predicted"]:
            for cpu in [False, True]:
                name = "discount_" + scale + ("_cpu" if cpu else "_nominal")
                policies[name] = discounted_policy(rows, ranking, forecasts, scale, cpu)
        for method, milestones in policies.items():
            reached = next(r for r in milestones if r["workers"] == best)
            first = milestones[0]
            traces.append({"pool": pool, "method": method, "milestones": milestones})
            for budget in [1, 3, 10, 30]:
                completed = [r for r in milestones if r["seconds"] <= budget]
                bill = completed[-1]["cost"] if completed else None
                cases.append({"pool": pool, "family": rows[0]["families"][0], "method": method, "budget": budget,
                    "time_to_best": reached["seconds"], "first_time": first["seconds"], "first_bill": first["cost"],
                    "cost": bill, "regret": bill - cost([best])[0] if bill is not None else None})
    grouped = defaultdict(list)
    for row in cases: grouped[row["method"], row["budget"]].append(row)
    summary = []
    for (method, budget), group in sorted(grouped.items()):
        valid = [r for r in group if r["cost"] is not None]
        summary.append({"method": method, "budget": budget, "pools": len(group), "uncertified": len(group) - len(valid),
            "mean_cost": float(np.mean([r["cost"] for r in valid])) if valid else None,
            "mean_regret": float(np.mean([r["regret"] for r in valid])) if valid else None,
            "mean_time_to_best": float(np.mean([r["time_to_best"] for r in group])),
            "mean_first_time": float(np.mean([r["first_time"] for r in group])), "mean_first_bill": float(np.mean([r["first_bill"] for r in group]))})
    lookup = {(r["method"], r["pool"]): r for r in cases if r["budget"] == 30}
    comparisons = []
    for method in sorted({r["method"] for r in cases} - {"around"}):
        paired = [{"family": b["family"], "seconds_saved": lookup["around", pool]["time_to_best"] - b["time_to_best"],
                   "first_bill_saved": lookup["around", pool]["first_bill"] - b["first_bill"]}
                  for (name, pool), b in lookup.items() if name == method]
        comparisons.append({"method": method, "saved_seconds_family_95": bootstrap(paired, lambda rows: float(np.mean([r["seconds_saved"] for r in rows]))),
                            "saved_first_bill_family_95": bootstrap(paired, lambda rows: float(np.mean([r["first_bill_saved"] for r in rows])))})
    result = {"scope": args.scope, "timing": "Development proxy: recorded cold CPU plus 100 us per proposal and 10 us per deferred forecast. Requires C++ implementation and fresh testing before adoption.",
        "change": "After the first failure and before any certificate, multiply success utility by 1/(1+hire_bill/scale). After a certificate, retain the original expected saving utility.",
        "unresolved_pools": unresolved, "summary": summary, "comparisons": comparisons,
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.ranking, args.forecasts, Path(__file__)]}}
    (args.output / "RESULTS.json").write_text(json.dumps(result, indent=2) + "\n")
    (args.output / "CASES.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in cases))
    (args.output / "TRACES.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in traces))
    print(json.dumps([r for r in summary if r["budget"] == 30], indent=2))


if __name__ == "__main__":
    main()
