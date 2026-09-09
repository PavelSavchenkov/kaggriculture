"""Complete-panel CPU, certification and signed bill comparisons by world."""
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

import numpy as np


EXP = Path(__file__).resolve().parents[1]


def pair_records(data, candidate):
    pairs = []
    for case in data:
        runs = {r["mode"]: r for r in case["runs"]}
        original, proposed = runs["original"], runs[candidate]
        a, b = original["returncode"] == 0, proposed["returncode"] == 0
        pairs.append({"id": case["id"], "world": case.get("world", str(case["seed"])), "seed": case["seed"],
                      "spec": case["spec"], "shop_kind": case.get("shop_kind", "fixed"), "candidate": candidate,
                      "original_cpu": original["cpu_seconds"], "candidate_cpu": proposed["cpu_seconds"],
                      "original_wall": original["wall_seconds"], "candidate_wall": proposed["wall_seconds"],
                      "original_certificate": a, "candidate_certificate": b,
                      "original_returncode": original["returncode"], "candidate_returncode": proposed["returncode"],
                      "original_native_failure": original["returncode"] not in [0, 3, 4], "candidate_native_failure": proposed["returncode"] not in [0, 3, 4],
                      "bill_delta": proposed["independent"]["hire_cost"] - original["independent"]["hire_cost"] if a and b else None,
                      "production_equal": proposed["independent"]["produced0"] == original["independent"]["produced0"] if a and b else None})
    return pairs


def metrics(pairs):
    if not pairs:
        return {"paired_runs": 0, "passes_gate": False, "reason": "no_eligible_pairs"}
    worlds = sorted({r["world"] for r in pairs})
    common = [r for r in pairs if r["bill_delta"] is not None]
    a = np.array([r["original_cpu"] for r in pairs]); b = np.array([r["candidate_cpu"] for r in pairs])
    bills = np.array([r["bill_delta"] for r in common])
    blocks = np.array([[sum(r["original_cpu"] for r in pairs if r["world"] == world),
                        sum(r["candidate_cpu"] for r in pairs if r["world"] == world),
                        sum(r["world"] == world for r in pairs),
                        sum(r["bill_delta"] for r in common if r["world"] == world),
                        sum(r["world"] == world for r in common)] for world in worlds], dtype=float)
    rng = np.random.default_rng(909926)
    draws = blocks[rng.integers(len(worlds), size=(10000, len(worlds)))].sum(axis=1)
    saved_interval = list(map(float, np.quantile((draws[:, 0] - draws[:, 1]) / draws[:, 2], [.025, .975])))
    bill_draws = draws[draws[:, 4] > 0]
    bill_interval = list(map(float, np.quantile(bill_draws[:, 3] / bill_draws[:, 4], [.025, .975]))) if len(bill_draws) else None
    lost = sum(r["original_certificate"] and not r["candidate_certificate"] for r in pairs)
    gates = {"coverage_at_least_24_pairs_six_worlds": len(pairs) >= 24 and len(worlds) >= 6,
             "mean_cpu_saving_at_least_20_percent": float((a.sum() - b.sum()) / a.sum()) >= .2,
             "positive_world_cluster_saved_cpu_interval": saved_interval[0] > 0,
             "no_lost_certificates": lost == 0, "no_higher_mean_common_bill": bool(len(common) and bills.mean() <= 0),
             "equal_common_success_production": bool(common) and all(r["production_equal"] for r in common)}
    return {"paired_runs": len(pairs), "worlds": len(worlds), "original_mean_cpu": float(a.mean()), "candidate_mean_cpu": float(b.mean()),
            "cpu_saving_fraction": float((a.sum() - b.sum()) / a.sum()), "world_cluster_95_saved_mean_cpu": saved_interval,
            "world_cluster_95_cpu_saving_fraction": list(map(float, np.quantile((draws[:, 0] - draws[:, 1]) / draws[:, 0], [.025, .975]))),
            "original_mean_wall": float(np.mean([r["original_wall"] for r in pairs])), "candidate_mean_wall": float(np.mean([r["candidate_wall"] for r in pairs])),
            "original_certificates": sum(r["original_certificate"] for r in pairs), "candidate_certificates": sum(r["candidate_certificate"] for r in pairs),
            "lost_certificates": lost, "gained_certificates": sum(not r["original_certificate"] and r["candidate_certificate"] for r in pairs),
            "original_native_failures": sum(r["original_native_failure"] for r in pairs), "candidate_native_failures": sum(r["candidate_native_failure"] for r in pairs),
            "common_successes": len(common), "mean_common_bill_delta": float(bills.mean()) if common else None,
            "world_cluster_95_common_bill_delta": bill_interval, "bootstrap_draws_without_common_success": int((draws[:, 4] == 0).sum()),
            "bills_higher": int((bills > 0).sum()), "bills_lower": int((bills < 0).sum()), "bills_equal": int((bills == 0).sum()),
            "worst_bill_increase": float(bills.max()) if common else None, "best_bill_decrease": float(bills.min()) if common else None,
            "production_equal_on_common": bool(common) and all(r["production_equal"] for r in common), "gates": gates, "passes_gate": all(gates.values())}


def main():
    root = EXP / "runs/warm_shop_benchmark_v1"
    protocol = json.loads((root / "PROTOCOL.json").read_text()); assert protocol["status"] == "completed"
    data = json.loads((root / "RESULTS.json").read_text())
    assert len(data) == len(protocol["cases"]) and len({r["id"] for r in data}) == len(data)
    for case in data:
        assert {r["mode"] for r in case["runs"]} == {"original", "guided", "stage"}
        for row in case["runs"]:
            assert row["status"] == "completed" and row["shops"] == case["shops"] and row["seed"] == case["seed"]
            if row["returncode"] == 0: assert row["independent"]["real_transitions"] == 719
    summary, records = [], []
    for method in ["guided", "stage"]:
        pairs = pair_records(data, method); records.extend(pairs)
        for axis in ["all", "shop_kind", "world", "spec"]:
            values = ["all"] if axis == "all" else sorted({r[axis] for r in pairs})
            for value in values:
                selected = pairs if axis == "all" else [r for r in pairs if r[axis] == value]
                summary.append({"candidate": method, "axis": axis, "value": value, **metrics(selected)})
    report = {"utc": datetime.now(timezone.utc).isoformat(), "scope": protocol["scope"], "primary": "guided", "secondary": "stage",
              "limits": "Designed shop/environment transfer with fixed source/rival agents. Complete CPU includes failures, but does not include independent post-run verification. Individual bills and finite-sample quality uncertainty remain visible. Slice gates do not replace the complete-panel primary gate.",
              "summary": summary, "pairs": records,
              "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [root / "PROTOCOL.json", root / "RESULTS.json", Path(__file__)]}}
    (root / "REPORT.json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    print(json.dumps([r for r in summary if r["axis"] == "all"], indent=2))


if __name__ == "__main__":
    main()
