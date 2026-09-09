"""Replay day-local policies on the complete exposed warm-call catalog."""
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np

from baseline_study import cost


EXP = Path(__file__).resolve().parents[1]


def replay(rows, method):
    by_extra = {r["extra_hires"]: r for r in rows}; assert set(by_extra) == set(range(7))
    order = list(range(7)); total = 0.; deferred, tried, scored = [], [], []
    selected = None
    if method.startswith("oracle"):
        good = [r for r in rows if r["endpoint"]]
        if good:
            best = min(good, key=lambda r: r["extra_hires"] if method == "oracle_same_bill" else r["cpu_seconds"])
            order = [best["extra_hires"]]
    for index, extra in enumerate(order):
        row = by_extra[extra]
        if method == "frozen_defer_002" and not row["reused"] and index < 7:
            total += (row["predicted_feature_cpu_us"] + row["predicted_query_cpu_us"]) / 1e6
            scored.append(extra)
            if row["predicted_boost"] < .02:
                # Charge the entire non-solver residual, conservatively including
                # work the deferred path might avoid. A retry pays its full call.
                residual = row["cpu_seconds"] - row["repair_cpu_seconds"] - row["cold_cpu_seconds"]
                assert residual >= -1e-6
                total += max(0., residual); deferred.append(extra); order.append(extra)
                continue
        tried.append(extra); total += row["cpu_seconds"]
        if row["endpoint"]:
            selected = row; break
    return {"method": method, "cpu_seconds": total, "tried": tried, "deferred": deferred, "scored": scored,
            "certificate": selected is not None, "selected_extra": selected["extra_hires"] if selected else None,
            "hire_cost": float(cost([selected["workers"]])[0]) if selected else None}


def main():
    source = EXP / "runs/warm_call_context_v1_complete/DATASET.json"
    data = json.loads(source.read_text()); assert data["calls"] == 777 and data["groups"] == 111
    groups = defaultdict(list)
    for row in data["rows"]: groups[row["course"], row["day"]].append(row)
    records, executed = [], []
    for (course, day), rows in sorted(groups.items()):
        original = replay(rows, "original")
        executed.extend(r for r in rows if r["extra_hires"] in original["tried"])
        for method in ["original", "frozen_defer_002", "oracle_same_bill", "oracle_fastest_bill_unconstrained"]:
            result = replay(rows, method)
            records.append({"course": course, "day": day, **result,
                            "saved_cpu_seconds": original["cpu_seconds"] - result["cpu_seconds"],
                            "hire_cost_delta": result["hire_cost"] - original["hire_cost"] if original["certificate"] and result["certificate"] else None})
    summary = []
    for method in sorted({r["method"] for r in records}):
        rows = [r for r in records if r["method"] == method]
        families = sorted({r["course"] for r in rows})
        per_course = np.array([[sum(r["saved_cpu_seconds"] for r in rows if r["course"] == course),
                                sum(r["course"] == course for r in rows)] for course in families])
        rng = np.random.default_rng(909907)
        sampled = per_course[rng.integers(len(families), size=(10000, len(families)))].sum(axis=1)
        bills = [r["hire_cost_delta"] for r in rows if r["hire_cost_delta"] is not None]
        summary.append({"method": method, "reached_days": len(rows), "certified_days": sum(r["certificate"] for r in rows),
                        "total_cpu_seconds": sum(r["cpu_seconds"] for r in rows), "mean_cpu_seconds": float(np.mean([r["cpu_seconds"] for r in rows])),
                        "course_bootstrap_95_saved_mean_cpu": list(map(float, np.quantile(sampled[:, 0] / sampled[:, 1], [.025, .975]))),
                        "mean_common_bill_delta": float(np.mean(bills)), "max_common_bill_delta": max(bills),
                        "executed_calls": sum(len(r["tried"]) for r in rows), "deferred_calls": sum(len(r["deferred"]) for r in rows)})
    stages = []
    for outcome in [False, True]:
        for terminal in [False, True]:
            rows = [r for r in executed if r["endpoint"] == outcome and (r["day"] == 29) == terminal]
            stages.append({"endpoint": outcome, "terminal": terminal, "calls": len(rows),
                           **{k: sum(r[k] for r in rows) for k in ["cpu_seconds", "reuse_cpu_seconds", "repair_cpu_seconds", "cold_cpu_seconds"]}})
    probability_bins = []
    for lo, hi in [(0, .02), (.02, .05), (.05, .1), (.1, .2), (.2, .5), (.5, 1.000001)]:
        rows = [r for r in executed if lo <= r["predicted_boost"] < hi and not r["reused"]]
        probability_bins.append({"low_inclusive": lo, "high_exclusive": min(1., hi), "calls": len(rows),
                                 "endpoint_successes": sum(r["endpoint"] for r in rows),
                                 "observed_success": float(np.mean([r["endpoint"] for r in rows])) if rows else None,
                                 "predicted_success": float(np.mean([r["predicted_boost"] for r in rows])) if rows else None,
                                 "failed_cpu_seconds": sum(r["cpu_seconds"] for r in rows if not r["endpoint"])})
    result = {"scope": "Post hoc development replay of the unchanged .02 deferral rule and explicit hindsight ceilings on 111 reached dawn states; not a new-seed test or complete-course speed measurement.",
              "boundary": "Every hypothetical query started from its recorded common dawn. A policy selecting a different query may alter subsequent days; never sum those alternate decisions into a claimed verified season.",
              "timing": "Recorded complete per-call CPU, plus measured context/query scoring on non-reused attempts. Deferred attempts pay all non-solver residual CPU; retried attempts pay again. Launcher and common course-prefix overhead are outside this replay.",
              "oracles": "Hindsight diagnostics use recorded outcomes. The same-bill oracle jumps to the cheapest recorded successful query; fastest oracle permits higher bills. Unresolved days retain the full original sweep cost.",
              "summary": summary, "original_executed_stage_cpu": stages, "original_executed_probability_bins": probability_bins,
              "days": records, "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [source, Path(__file__)]}}
    output = EXP / "runs/warm_catalog_policy_diagnostic_v2"; output.mkdir(exist_ok=False)
    (output / "RESULTS.json").write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
    print(json.dumps({k: result[k] for k in ["summary", "original_executed_stage_cpu", "original_executed_probability_bins"]}, indent=2))


if __name__ == "__main__":
    main()
