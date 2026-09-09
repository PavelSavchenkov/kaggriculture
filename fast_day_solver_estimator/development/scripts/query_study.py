"""Whole-family exclusion for frozen three-second compiler-call prediction."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

import joblib
import numpy as np
from sklearn.metrics import brier_score_loss, log_loss, roc_auc_score
from threadpoolctl import threadpool_limits

from query_models import CpuModel, SuccessModel, design
from screening import screen_reason


EXP = Path(__file__).resolve().parents[1]


def queries(rows):
    result = []
    for row in rows:
        if screen_reason(row):
            continue
        for query in row["query_outcomes"]:
            assert query["budget_seconds"] == 3 and query["status"] in ["FEASIBLE", "UNKNOWN"]
            result.append({"id": row["id"], "query_id": query["id"], "workers": query["workers"], "family": row["families"][0],
                           "features": row["features"], "lower_bound": row["lower_bound"],
                           "success": query["status"] == "FEASIBLE", "cpu_seconds": query["cpu_seconds"]})
    return result


def matrix(rows, names):
    return design(np.array([r["features"] for r in rows]), np.array([r["workers"] for r in rows]),
                  np.array([r["lower_bound"] for r in rows]), names)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("training", type=Path)
    parser.add_argument("test", type=Path)
    parser.add_argument("name")
    args = parser.parse_args()
    train_data, test_data = [json.loads(p.read_text()) for p in [args.training, args.test]]
    names = train_data["feature_names"]
    assert test_data["feature_names"] == names and "active_hours" not in names, "first query model is ordinary-day only"
    train_rows = [r for r in train_data["rows"] if len(r["families"]) == 1]
    test_rows = [r for r in test_data["rows"] if len(r["families"]) == 1]
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    for name in ["query_models.py", "query_study.py", "screening.py"]:
        shutil.copyfile(EXP / "scripts" / name, output / name)
    training, testing = queries(train_rows), queries(test_rows)
    a, b = matrix(training, names), matrix(testing, names)
    groups = np.array([r["family"] for r in training]); test_groups = np.array([r["family"] for r in testing])
    y = np.array([r["success"] for r in training]); cpu = np.array([r["cpu_seconds"] for r in training])
    predictions, folds = [], []
    with threadpool_limits(limits=1):
        for family in sorted(set(test_groups)):
            keep, selected = groups != family, test_groups == family
            truth = np.array([r["success"] for r, s in zip(testing, selected) if s])
            actual_cpu = np.array([r["cpu_seconds"] for r, s in zip(testing, selected) if s])
            cpu_model = CpuModel().fit(a[keep], cpu[keep]); predicted_cpu = cpu_model.predict(b[selected])
            report = {"held_family": str(family), "training_queries": int(keep.sum()), "test_queries": int(selected.sum()), "methods": {}}
            for kind in ["logistic", "boost"]:
                model = SuccessModel(kind).fit(a[keep], y[keep]); probability = model.predict(b[selected])
                report["methods"][kind] = {"brier": float(brier_score_loss(truth, probability)),
                    "log_loss": float(log_loss(truth, probability, labels=[False, True])),
                    "auc": float(roc_auc_score(truth, probability)) if len(set(truth)) == 2 else None,
                    "observed_success": float(truth.mean()), "predicted_success": float(probability.mean()),
                    "cpu_mae_seconds": float(np.abs(actual_cpu - predicted_cpu).mean())}
                for index, chance, seconds in zip(np.flatnonzero(selected), probability, predicted_cpu):
                    row = testing[index]
                    predictions.append({k: row[k] for k in ["id", "query_id", "workers", "family", "success", "cpu_seconds"]}
                                       | {"method": kind, "predicted_success": float(chance), "predicted_cpu_seconds": float(seconds)})
            folds.append(report); print(family, report["methods"], flush=True)
        fitted = {kind: SuccessModel(kind).fit(a, y) for kind in ["logistic", "boost"]}
        fitted["cpu"] = CpuModel().fit(a, cpu)
    joblib.dump({"feature_names": names, "models": fitted}, output / "models.joblib")
    (output / "PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in predictions))
    report = {"scope": "Development complete parent-family exclusion; predicts bounded compiler outcomes, never physical infeasibility.",
              "query_budget_seconds": 3, "workforce_input": "The explicitly proposed call argument k is legitimate. No answer workforce, source ceiling or certificate feature is read.",
              "action_range_limit": "Existing logged query ranges were frozen using source-ceiling metadata. A future untouched policy test must freeze an input-derived action range.",
              "inputs_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.training, args.test]}, "folds": folds}
    (output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
