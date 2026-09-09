"""Report a frozen estimator against completed new-wave reference evidence."""
import argparse
import hashlib
import json
import shutil
from collections import defaultdict
from pathlib import Path

import numpy as np

from baseline_study import cost, score


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("predictions", type=Path)
    parser.add_argument("name")
    parser.add_argument("--scope", default="Frozen parameters on completed new-wave contracts; novelty filtering is recorded in the dataset.")
    args = parser.parse_args()
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    data = json.loads(args.dataset.read_text())
    assert not data["pending_cases"], "do not evaluate a partial unseen wave"
    shutil.copyfile(args.dataset, output / "DATASET.json")
    cases = {r["id"]: r for r in data["rows"]}
    raw = [json.loads(line) for line in args.predictions.read_text().splitlines()]
    for row in data["rows"]:
        if row.get("original_geometry_workers") is not None:
            raw.append({"id": row["id"], "method": "original_geometry", "ranking_score": row["original_geometry_workers"],
                        "raw_prediction": row["original_geometry_workers"]})
            raw.append({"id": row["id"], "method": "original_geometry_with_bound", "ranking_score": max(row["lower_bound"], row["original_geometry_workers"]),
                        "raw_prediction": row["original_geometry_workers"]})
    predictions, unresolved = [], []
    for row in raw:
        if row["id"] not in cases:
            continue
        case = cases[row["id"]]
        prediction = {"id": row["id"], "family": "+".join(case["families"]), "method": row["method"],
                      "reference_workers": case["reference_workers"], "lower_bound": case["lower_bound"],
                      "prediction": row["ranking_score"], "raw_prediction": row["raw_prediction"]}
        (predictions if case["reference_workers"] is not None else unresolved).append(prediction)
    groups = defaultdict(list)
    for row in predictions:
        groups[row["method"]].append(row)
    summaries = []
    for method, rows in sorted(groups.items()):
        y = np.array([r["reference_workers"] for r in rows]); p = np.array([r["prediction"] for r in rows])
        error = p - y; cash_error = cost(p) - cost(y)
        families = sorted({f for r in rows for f in cases[r["id"]]["families"]})
        family = {f: score(np.array([r["reference_workers"] for r in rows if f in cases[r["id"]]["families"]]),
                          np.array([r["prediction"] for r in rows if f in cases[r["id"]]["families"]])) for f in families}
        summaries.append({"method": method, "labeled_contracts": len(rows), **score(y, p),
                          "under_by_at_least_2_workers": float(np.mean(error <= -2)),
                          "under_by_at_least_1_worker": float(np.mean(error <= -1)),
                          "over_by_at_least_1_worker": float(np.mean(error >= 1)),
                          "under_by_at_least_500_cash": float(np.mean(cash_error <= -500)),
                          "over_by_at_least_500_cash": float(np.mean(cash_error >= 500)),
                          "cash_bias": float(cash_error.mean()), "p95_absolute_worker_error": float(np.quantile(np.abs(error), .95)),
                          "family_macro_worker_mae": float(np.mean([r["worker_mae"] for r in family.values()])), "families": family})
    report = {"scope": args.scope,
              "target": "Cheapest verified upper bound; shorter-sweep and source witnesses remain separate. This is not minimum-cost truth.",
              "contracts": len(cases), "labeled": sum(r["reference_workers"] is not None for r in cases.values()),
              "unresolved": sum(r["reference_workers"] is None for r in cases.values()),
              "shared_family_contracts": sum(len(r["families"]) > 1 for r in cases.values()),
              "family_weighting": "Each physical contract counts once overall and once for each family in which it occurs when computing family macro metrics.",
              "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.predictions]},
              "summary": summaries}
    (output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")
    (output / "PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in predictions))
    (output / "UNRESOLVED.json").write_text(json.dumps(unresolved, indent=2) + "\n")
    for row in summaries:
        print(row["method"], "macroMAE", round(row["family_macro_worker_mae"], 4), "bias", round(row["worker_bias"], 4),
              "under2", row["under_by_at_least_2_workers"], "over1", row["over_by_at_least_1_worker"])


if __name__ == "__main__":
    main()
