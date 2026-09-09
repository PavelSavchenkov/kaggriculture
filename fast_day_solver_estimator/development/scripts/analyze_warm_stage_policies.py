"""Development diagnostic for deferring cold fallback after observed repair failure."""
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np

from analyze_warm_catalog_policies import replay
from baseline_study import cost


EXP = Path(__file__).resolve().parents[1]


def stage_replay(rows, threshold):
    by_extra = {r["extra_hires"]: r for r in rows}; assert set(by_extra) == set(range(7))
    order = list(range(7)); cpu, deferred, called = 0., [], []
    selected = None
    for index, extra in enumerate(order):
        row = by_extra[extra]
        if index >= 7:
            assert row["cold_called"] and not row["repair_succeeded"] and not row["reused"]
            cpu += row["cpu_seconds"] - row["repair_cpu_seconds"] - row["reuse_cpu_seconds"]
        else:
            if row["cold_called"]:
                cpu += (row["predicted_feature_cpu_us"] + row["predicted_query_cpu_us"]) / 1e6
                if row["predicted_boost"] < threshold:
                    cpu += row["cpu_seconds"] - row["cold_cpu_seconds"]
                    deferred.append(extra); order.append(extra)
                    continue
            cpu += row["cpu_seconds"]
        called.append(extra)
        if row["endpoint"]:
            selected = row; break
    return {"cpu_seconds": cpu, "deferred": deferred, "completed_calls": called,
            "certificate": selected is not None, "selected_extra": selected["extra_hires"] if selected else None,
            "hire_cost": float(cost([selected["workers"]])[0]) if selected else None}


def main():
    path = EXP / "runs/warm_call_context_v1_complete/DATASET.json"
    data = json.loads(path.read_text()); groups = defaultdict(list)
    for row in data["rows"]: groups[row["course"], row["day"]].append(row)
    records = []
    for (course, day), rows in sorted(groups.items()):
        original = replay(rows, "original")
        for threshold in [.02, .05, .1, .2, .25]:
            result = stage_replay(rows, threshold)
            records.append({"course": course, "day": day, "threshold": threshold, **result,
                            "original_cpu_seconds": original["cpu_seconds"],
                            "saved_cpu_seconds": original["cpu_seconds"] - result["cpu_seconds"],
                            "hire_cost_delta": result["hire_cost"] - original["hire_cost"] if original["certificate"] and result["certificate"] else None,
                            "lost_certificate": original["certificate"] and not result["certificate"]})
    summary = []
    for threshold in [.02, .05, .1, .2, .25]:
        rows = [r for r in records if r["threshold"] == threshold]
        old = sum(r["original_cpu_seconds"] for r in rows); new = sum(r["cpu_seconds"] for r in rows)
        courses = sorted({r["course"] for r in rows})
        matrix = np.array([[sum(r["saved_cpu_seconds"] for r in rows if r["course"] == course), sum(r["course"] == course for r in rows)] for course in courses])
        rng = np.random.default_rng(909908)
        samples = matrix[rng.integers(len(courses), size=(10000, len(courses)))].sum(axis=1)
        bills = [r["hire_cost_delta"] for r in rows if r["hire_cost_delta"] is not None]
        summary.append({"threshold": threshold, "reached_days": len(rows), "certified_days": sum(r["certificate"] for r in rows),
                        "original_cpu_seconds": old, "candidate_cpu_seconds": new, "cpu_saving_fraction": 1 - new / old,
                        "course_bootstrap_95_saved_mean_cpu": list(map(float, np.quantile(samples[:, 0] / samples[:, 1], [.025, .975]))),
                        "mean_common_bill_delta": float(np.mean(bills)), "worst_bill_delta": max(bills),
                        "deferred_calls": sum(len(r["deferred"]) for r in rows), "lost_certificates": sum(r["lost_certificate"] for r in rows)})
    cold = [r for r in data["rows"] if r["cold_called"]]
    output = EXP / "runs/warm_stage_policy_diagnostic_v1"; output.mkdir(exist_ok=False)
    result = {"scope": "Development only, same recorded 111 dawn states; no complete-course or unseen speed claim. Thresholds are exploratory.",
              "decision": "Run normal reuse and repair. After observed repair failure, defer a low-probability cold call. Retry deferred cold calls only if all ordinary counts fail, without rerunning repair.",
              "information": "Existing context-model probability and own observed repair failure are allowed to the call policy. No cold outcome or candidate schedule enters its decision.",
              "timing": "Conservatively retain all non-cold CPU on deferral. Retry pays original full CPU minus already completed repair/reuse. Charge measured prediction CPU only after failed repair.",
              "all_catalog_cold_calls": len(cold), "all_catalog_cold_successes": sum(r["endpoint"] for r in cold),
              "lowest_success_probability_among_cold_successes": min(r["predicted_boost"] for r in cold if r["endpoint"]),
              "summary": summary, "days": records,
              "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [path, Path(__file__), EXP / "scripts/analyze_warm_catalog_policies.py"]}}
    (output / "RESULTS.json").write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k not in ["days", "input_sha256"]}, indent=2))


if __name__ == "__main__":
    main()
