"""Report seed- and course-clustered uncertainty for complete warm pairs."""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np


def main():
    parser = argparse.ArgumentParser(); parser.add_argument("benchmark", type=Path); args = parser.parse_args()
    root = args.benchmark
    protocol = json.loads((root / "PROTOCOL.json").read_text()); assert protocol["status"] == "completed"
    data = json.loads((root / "RESULTS.json").read_text()); pairs = []
    for case in data:
        modes = {r["mode"]: r for r in case["runs"]}; assert set(modes) == {"original", "guided"}
        a, b = modes["original"], modes["guided"]
        common = a["returncode"] == b["returncode"] == 0
        pairs.append({"id": case["id"], "seed": case["seed"], "spec": case["spec"],
            "original_cpu": a["cpu_seconds"], "guided_cpu": b["cpu_seconds"],
            "original_failed": a["returncode"] != 0, "guided_failed": b["returncode"] != 0,
            "lost": a["returncode"] == 0 and b["returncode"] != 0,
            "gained": a["returncode"] != 0 and b["returncode"] == 0,
            "hire_delta": b["independent"]["hire_cost"] - a["independent"]["hire_cost"] if common else None,
            "cash_delta": b["independent"]["cash"] - a["independent"]["cash"] if common else None,
            "production_equal": all(a["independent"][k] == b["independent"][k] for k in ["produced0", "produced1"]) if common else None})
    rng = np.random.default_rng(909850)

    def totals(rows):
        bills = [r["hire_delta"] for r in rows if r["hire_delta"] is not None]
        return [sum(r["original_cpu"] for r in rows), sum(r["guided_cpu"] for r in rows), len(rows),
                sum(r["lost"] for r in rows), sum(r["gained"] for r in rows), sum(bills), len(bills),
                sum(r["original_failed"] for r in rows), sum(r["guided_failed"] for r in rows)]

    uncertainty = {}
    for axis in ["seed", "spec"]:
        groups = sorted({r[axis] for r in pairs})
        matrix = np.array([totals([r for r in pairs if r[axis] == group]) for group in groups], dtype=float)
        sampled = matrix[rng.integers(len(groups), size=(10000, len(groups)))].sum(axis=1)
        metrics = {"saved_mean_cpu": (sampled[:, 0] - sampled[:, 1]) / sampled[:, 2],
                   "cpu_saving_fraction": 1 - sampled[:, 1] / sampled[:, 0],
                   "failure_rate_delta": (sampled[:, 8] - sampled[:, 7]) / sampled[:, 2],
                   "mean_common_hire_delta": np.divide(sampled[:, 5], sampled[:, 6], out=np.full(10000, np.nan), where=sampled[:, 6] > 0)}
        uncertainty[axis] = {"groups": len(groups),
                             **{k: list(map(float, np.quantile(v[np.isfinite(v)], [.025, .975]))) if np.isfinite(v).any() else None for k, v in metrics.items()},
                             "bootstrap_samples_without_common_success": int(np.sum(sampled[:, 6] == 0))}
    old, new, n, lost, gained, bill, common, old_fail, new_fail = totals(pairs)
    mean_bill = bill / common if common else None
    gates = {"mean_cpu_saving_at_least_20_percent": 1 - new / old >= .2,
             "positive_seed_cluster_saved_cpu_interval": uncertainty["seed"]["saved_mean_cpu"][0] > 0,
             "no_lost_certificates": lost == 0, "no_worse_mean_common_hire_bill": mean_bill is not None and mean_bill <= 0}
    result = {"scope": protocol["scope"], "paired_runs": n, "seeds": len({r["seed"] for r in pairs}),
              "original_mean_cpu": old / n, "guided_mean_cpu": new / n, "cpu_saving_fraction": 1 - new / old,
              "original_failures": old_fail, "guided_failures": new_fail, "lost_certificates": lost, "gained_certificates": gained,
              "common_successes": common, "mean_common_hire_delta": mean_bill,
              "common_production_equal": all(r["production_equal"] for r in pairs if r["production_equal"] is not None),
              "gates": gates, "passes_strict_gate": all(gates.values()), "cluster_95_intervals": uncertainty,
              "caveat": "The strict quality gate remains failed if any paired certificate is lost or mean common bill rises, even when cluster intervals include zero. Source/rival and shop sequence are fixed exposed fixtures.",
              "pairs": pairs, "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [root / "PROTOCOL.json", root / "RESULTS.json", Path(__file__)]}}
    with (root / "SEED_REPORT.json").open("x") as stream: stream.write(json.dumps(result, indent=2, allow_nan=False) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k not in ["pairs", "input_sha256"]}, indent=2))


if __name__ == "__main__":
    main()
