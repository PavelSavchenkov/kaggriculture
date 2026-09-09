"""Signed calibration of frozen call forecasts, including large-workforce tails."""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np
from sklearn.metrics import brier_score_loss, log_loss

from screening import screen_reason


def metrics(rows):
    truth = np.array([r["success"] for r in rows]); p = np.array([r["predicted_success"] for r in rows])
    time = np.array([r["cpu_seconds"] for r in rows]); estimate = np.array([r["predicted_cpu_seconds"] for r in rows])
    return {"calls": len(rows), "contracts": len({r["contract"] for r in rows}), "observed_success": float(truth.mean()),
        "predicted_success": float(p.mean()), "success_bias": float((p - truth).mean()),
        "brier": float(brier_score_loss(truth, p)), "log_loss": float(log_loss(truth, p, labels=[False, True])),
        "failed_when_at_least_80pct": int(np.sum((p >= .8) & ~truth)),
        "succeeded_when_at_most_20pct": int(np.sum((p <= .2) & truth)),
        "cpu_mae_seconds": float(np.abs(estimate - time).mean()), "cpu_bias_seconds": float((estimate - time).mean())}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("predictions", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--scope", required=True)
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text()); assert not data["pending_cases"]
    forecasts = defaultdict(dict)
    for row in map(json.loads, args.predictions.read_text().splitlines()):
        assert row["query_id"] not in forecasts[row["method"]]
        forecasts[row["method"]][row["query_id"]] = row
    predictions, screened = [], 0
    for row in data["rows"]:
        for query in row["query_outcomes"]:
            if screen_reason(row) or query["workers"] < row["lower_bound"]:
                assert query["status"] == "UNKNOWN"; screened += 1; continue
            assert query["budget_seconds"] == 3 and query["status"] in ["FEASIBLE", "UNKNOWN"]
            for method, values in forecasts.items():
                prediction = values[query["id"]]
                predictions.append({"query_id": query["id"], "contract": row["id"], "families": row["families"],
                    "workers": query["workers"], "method": method, "success": query["status"] == "FEASIBLE",
                    "cpu_seconds": query["cpu_seconds"], "predicted_success": prediction["predicted_success"],
                    "predicted_cpu_seconds": prediction["predicted_cpu_seconds"]})
    groups = defaultdict(list)
    for row in predictions:
        groups[row["method"], "all", "all"].append(row)
        for lo, hi in [(1, 4), (5, 8), (9, 12), (13, 16), (17, 24), (25, 32), (33, 40)]:
            if lo <= row["workers"] <= hi:
                groups[row["method"], "workers", f"{lo}-{hi}"].append(row)
        for family in row["families"]: groups[row["method"], "family", family].append(row)
    result = {"scope": args.scope, "target": "Observed outcome of the frozen three-second compiler call. UNKNOWN is not physical infeasibility.",
        "screened_calls": screened, "cpu_limit": "The frozen CPU model estimates exponentiated mean log CPU; report its signed error without calling it an expected-time estimate.",
        "summary": [{"method": method, "axis": axis, "value": value, **metrics(rows)} for (method, axis, value), rows in sorted(groups.items())],
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.predictions, Path(__file__)]}}
    args.output.mkdir(exist_ok=False)
    (args.output / "RESULTS.json").write_text(json.dumps(result, indent=2) + "\n")
    (args.output / "PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in predictions))
    print(json.dumps([r for r in result["summary"] if r["axis"] != "family"], indent=2))


if __name__ == "__main__":
    main()
