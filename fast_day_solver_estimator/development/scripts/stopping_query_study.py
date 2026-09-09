"""Replay observable stopping after the first verified schedule."""
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


def stopped_policy(rows, ranking, forecasts, strategy, threshold):
    prices = cost(np.arange(1, 41))
    outcomes = {q["id"]: q for r in rows for q in r["query_outcomes"]}
    queries = [{"id": q["id"], "case": r["id"], "workers": q["workers"]}
               for r in rows if not screen_reason(r) for q in r["query_outcomes"]
               if q["workers"] >= r["lower_bound"]]
    assert len({q["id"] for q in queries}) == len(queries)
    order = {r["id"]: (ranking[r["id"]], tie(r["id"])) for r in rows}
    elapsed, overhead, best = len(rows) * .0001, len(rows) * .0001, 41
    attempted, milestones, calls = set(), [], []
    charged = False
    first_call_count = None
    reason, remaining_utility = "exhausted", None
    while True:
        available = [q for q in queries if q["id"] not in attempted and q["workers"] < best]
        if not available:
            break
        if best < 41 and strategy == "first_certificate":
            reason = "first_certificate"
            break
        if best < 41 and strategy == "call_cap" and len(calls) - first_call_count >= threshold:
            reason = "refinement_call_cap"
            break
        if best < 41 and strategy == "time_cap" and elapsed - milestones[0]["seconds"] >= threshold:
            reason = "refinement_time_cap"
            break
        if best < 41 and strategy in ["around", "utility"]:
            if not charged:
                charge = len(available) * .00001
                elapsed += charge; overhead += charge; charged = True
            def utility(q):
                return forecasts[q["id"]]["predicted_success"] * (prices[best - 1] - prices[q["workers"] - 1]) / 3
            utilities = {q["id"]: utility(q) for q in available}
            remaining_utility = max(utilities.values())
            if remaining_utility < threshold:
                reason = "expected_saving_threshold"
                break
        if best < 41 and strategy == "utility":
            chosen = min(available, key=lambda q: (-utilities[q["id"]], tie(q["id"])))
        else:
            chosen = min(available, key=lambda q: (order[q["case"]], abs(q["workers"] - ranking[q["case"]]), -q["workers"]))
        # Policy sees the result and measured duration only after making its choice.
        query = outcomes[chosen["id"]]
        elapsed += query["cpu_seconds"]
        attempted.add(chosen["id"]); calls.append(chosen["id"])
        if query["status"] == "FEASIBLE":
            best = query["workers"]
            milestones.append({"seconds": elapsed, "workers": best, "cost": float(prices[best - 1]), "query_id": query["id"]})
            if first_call_count is None:
                first_call_count = len(calls)
    return {"milestones": milestones, "cpu_seconds": elapsed, "calls": calls, "prediction_cpu_seconds": overhead,
            "stop_reason": reason, "remaining_expected_saving_per_second": remaining_utility,
            "final_cost": float(prices[best - 1]) if best < 41 else None}


def main():
    parser = argparse.ArgumentParser()
    for name in ["dataset", "ranking", "forecasts", "output"]:
        parser.add_argument(name, type=Path)
    parser.add_argument("--scope", required=True)
    args = parser.parse_args()
    args.output.mkdir(exist_ok=False)
    data = json.loads(args.dataset.read_text()); assert not data.get("pending_cases")
    pools = defaultdict(list)
    for row in data["rows"]:
        pools[row["pool"]].append(row)
    ranking = {r["id"]: r["ranking_score"] for r in map(json.loads, args.ranking.read_text().splitlines()) if r["method"] == "cost_direct_extra"}
    forecasts = {r["query_id"]: r for r in map(json.loads, args.forecasts.read_text().splitlines()) if r["method"] == "boost"}
    policies = [("around_exhaustive", "exhaustive", 0), ("first_certificate", "first_certificate", 0)]
    policies += [(f"{strategy}_{threshold:g}", strategy, threshold) for strategy in ["around", "utility"] for threshold in [0, .1, 1, 10]]
    policies += [(f"{strategy}_{threshold:g}", strategy, threshold) for strategy, values in [("call_cap", [1, 2, 4]), ("time_cap", [3, 10])] for threshold in values]
    cases, traces, budgets = [], [], []
    checks = 0
    for pool, rows in sorted(pools.items()):
        assert len(rows) == 5 and len({tuple(r["families"]) for r in rows}) == 1
        labels = [r["short_sweep_workers"] for r in rows if r["short_sweep_workers"] is not None]
        reference = float(cost([min(labels)])[0]) if labels else None
        first = None
        for name, strategy, threshold in policies:
            result = stopped_policy(rows, ranking, forecasts, strategy, threshold)
            milestones = result["milestones"]
            this_first = milestones[0] if milestones else None
            if name == "around_exhaustive":
                old_calls = []
                old = run_policy(rows, ranking, {}, "around", old_calls)
                assert old_calls == result["calls"] and abs(old[1] - result["cpu_seconds"]) < 1e-8
                assert result["final_cost"] == reference
                first = this_first
            assert this_first == first, "post-certificate refinement changed the first certificate"
            checks += 1
            record = {"pool": pool, "family": rows[0]["families"][0], "method": name,
                      **{k: v for k, v in result.items() if k not in ["milestones", "calls"]},
                      "call_count": len(result["calls"]), "reference_cost": reference,
                      "regret": result["final_cost"] - reference if reference is not None else None}
            cases.append(record); traces.append({"pool": pool, "method": name, **result})
            for budget in [1, 3, 10, 30, 120]:
                completed = [r for r in milestones if r["seconds"] <= budget]
                budgets.append({"pool": pool, "method": name, "budget": budget,
                                "cost": completed[-1]["cost"] if completed else None})
    lookup = {(r["method"], r["pool"]): r for r in cases}
    summary, comparisons = [], []
    for name, _, _ in policies:
        group = [r for r in cases if r["method"] == name]
        valid = [r for r in group if r["final_cost"] is not None]
        summary.append({"method": name, "pools": len(group), "uncertified": len(group) - len(valid),
            "mean_terminated_cpu": float(np.mean([r["cpu_seconds"] for r in group])),
            "p95_terminated_cpu": float(np.quantile([r["cpu_seconds"] for r in group], .95)),
            "mean_verified_cost": float(np.mean([r["final_cost"] for r in valid])) if valid else None,
            "mean_regret": float(np.mean([r["regret"] for r in valid])) if valid else None,
            "worse_than_reference": sum(r["regret"] > 0 for r in valid),
            "regret_at_least_100": sum(r["regret"] >= 100 for r in valid),
            "max_regret": max((r["regret"] for r in valid), default=None)})
        if name == "around_exhaustive":
            continue
        paired = [{"family": r["family"], "saved_cpu": lookup["around_exhaustive", r["pool"]]["cpu_seconds"] - r["cpu_seconds"],
                   "extra_cost": r["regret"]} for r in group]
        comparisons.append({"method": name, "saved_cpu_family_95": bootstrap(paired, lambda rs: float(np.mean([r["saved_cpu"] for r in rs]))),
            "extra_cost_family_95": bootstrap([r for r in paired if r["extra_cost"] is not None], lambda rs: float(np.mean([r["extra_cost"] for r in rs])))})
    budget_summary = []
    for name, _, _ in policies:
        for budget in [1, 3, 10, 30, 120]:
            group = [r for r in budgets if r["method"] == name and r["budget"] == budget]
            valid = [r["cost"] for r in group if r["cost"] is not None]
            budget_summary.append({"method": name, "budget": budget, "uncertified": len(group) - len(valid),
                                   "mean_verified_cost": float(np.mean(valid)) if valid else None})
    report = {"scope": args.scope, "policy": "Unchanged around order until first success; then stop if every remaining call has predicted success times saving / 3 seconds below threshold. Utility variants also change improvement-call order.",
        "termination": "Observable completion or all cheaper calls exhausted. No reference answer controls stopping. Unknown results do not prove infeasibility.",
        "timing": "Recorded complete cold-call CPU plus provisional 100 us per proposal and 10 us per deferred forecast; C++ policy/whole-pipeline timing still required.",
        "budget_handling": "No crossing call contributes a certificate to its budget report; full termination is separately timed. Simple refinement time caps stop after a complete call crosses the cap and report the actual overshoot.",
        "control_and_first_certificate_checks": checks, "summary": summary, "comparisons": comparisons, "budget_summary": budget_summary,
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.ranking, args.forecasts, Path(__file__)]}}
    (args.output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")
    for filename, rows in [("CASES", cases), ("TRACES", traces)]:
        (args.output / (filename + ".jsonl")).write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in rows))
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
