"""Held-family calendar ablation for individual bounded compiler calls."""
import argparse
import hashlib
import json
import shutil
from collections import defaultdict
from pathlib import Path

import joblib
import numpy as np
from sklearn.metrics import brier_score_loss, log_loss, roc_auc_score
from threadpoolctl import threadpool_limits

from context_query_models import design
from query_models import CpuModel, SuccessModel


EXP = Path(__file__).resolve().parents[1]


def observations(data, source):
    result, rejected = [], 0
    names = data["feature_names"]
    for row in data["rows"]:
        if len(row["families"]) != 1:
            continue
        missing = any(row.get(k, 0) > 0 for k in ["deadline_missing_quantity", "supply_missing", "seed_missing", "land_missing"])
        for query in row["query_outcomes"]:
            assert query["budget_seconds"] == 3 and query["status"] in ["FEASIBLE", "UNKNOWN"]
            if missing or query["workers"] < row["lower_bound"]:
                assert query["status"] == "UNKNOWN"
                rejected += 1
                continue
            identity = str(source) + ":" + query.get("sample_key", query["id"])
            result.append({"sample_key": hashlib.sha256(identity.encode()).hexdigest(), "contract_key": row["contract_key"],
                "query_id": query["id"], "batch": query.get("batch", str(source)),
                "obligation_key": row["obligation_key"], "family": row["families"][0], "profile": row["profile"],
                "active_hours": row["active_hours"], "parent_panel": row["parent_panel"], "workers": query["workers"],
                "success": query["status"] == "FEASIBLE", "cpu_seconds": query["cpu_seconds"], "features": row["features"]})
    matrix = design(np.array([r["features"] for r in result]), np.array([r["workers"] for r in result]), names)
    for row in result:
        del row["features"]
    return result, matrix, rejected


def metrics(rows):
    y = np.array([r["success"] for r in rows]); p = np.array([r["predicted_success"] for r in rows])
    cpu = np.array([r["cpu_seconds"] for r in rows]); predicted_cpu = np.array([r["predicted_cpu_seconds"] for r in rows])
    return {"queries": len(rows), "contracts": len({r["contract_key"] for r in rows}),
        "brier": float(brier_score_loss(y, p)), "log_loss": float(log_loss(y, p, labels=[False, True])),
        "auc": float(roc_auc_score(y, p)) if len(set(y)) == 2 else None,
        "observed_success": float(y.mean()), "predicted_success": float(p.mean()),
        "success_bias": float((p - y).mean()), "cpu_mae_seconds": float(np.abs(cpu - predicted_cpu).mean()),
        "cpu_bias_seconds": float((predicted_cpu - cpu).mean()),
        "failed_with_prediction_at_least_80pct": int(np.sum((p >= .8) & ~y)),
        "succeeded_with_prediction_at_most_20pct": int(np.sum((p <= .2) & y))}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("--training", nargs="+", type=Path, required=True)
    parser.add_argument("--testing", nargs="+", type=Path, required=True)
    parser.add_argument("--bases", nargs="+", choices=["physical_horizon", "calendar"], default=["physical_horizon", "calendar"])
    parser.add_argument("--kinds", nargs="+", choices=["logistic", "boost"], default=["logistic", "boost"])
    parser.add_argument("--skip-full-fit", action="store_true")
    args = parser.parse_args()
    output = EXP / "runs" / args.name; output.mkdir(exist_ok=False)
    sources = sorted(set(args.training + args.testing))
    data = {p: json.loads(p.read_text()) for p in sources}
    assert all(not d.get("pending_cases") for d in data.values())
    prepared = {p: observations(data[p], p) for p in sources}
    training = [r for p in args.training for r in prepared[p][0]]
    testing = [r for p in args.testing for r in prepared[p][0]]
    x = np.concatenate([prepared[p][1] for p in args.training]); z = np.concatenate([prepared[p][1] for p in args.testing])
    assert len({r["sample_key"] for r in training}) == len(training)
    assert len({r["sample_key"] for r in testing}) == len(testing)
    train_family = np.array([r["family"] for r in training]); test_family = np.array([r["family"] for r in testing])
    y = np.array([r["success"] for r in training]); cpu = np.array([r["cpu_seconds"] for r in training])
    for name in ["context_query_study.py", "context_query_models.py", "query_models.py"]:
        shutil.copyfile(EXP / "scripts" / name, output / name)
    protocol = {"scope": "Exposed development model selection. Exclude each test family from all fits, including every calendar, expansion and repeated call from its parents.",
        "target": "Observed success and CPU of an individual three-second compiler call. UNKNOWN is never treated as physical infeasibility.",
        "ablation": "physical_horizon uses the same 233 physical features, horizon and proposed k. calendar adds actual selected-hire capacities and input-release descriptors. Both use identical necessary screening.",
        "cpu_prediction": "Exponentiated predicted mean log CPU; a geometric-mean heuristic, not expected CPU.",
        "weighting": "Each logged call counts once. Repeated calls remain observations; contracts, families and calendar profiles are reported separately.",
        "training_queries": len(training), "test_queries": len(testing), "bases": args.bases, "kinds": args.kinds,
        "full_fit": not args.skip_full_fit,
        "screened_queries": {str(p): prepared[p][2] for p in sources},
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    predictions, folds = [], []
    with threadpool_limits(limits=1):
        for family in sorted(set(test_family)):
            keep, test = train_family != family, test_family == family
            assert keep.sum() and test.sum() and len(set(y[keep])) == 2
            fold = {"family": str(family), "training_queries": int(keep.sum()), "test_queries": int(test.sum()), "methods": {}}
            for basis in args.bases:
                columns = 235 if basis == "physical_horizon" else 320
                a, b = x[keep, :columns], z[test, :columns]
                seconds = CpuModel().fit(a, cpu[keep]).predict(b)
                for kind in args.kinds:
                    probability = SuccessModel(kind).fit(a, y[keep]).predict(b)
                    method = basis + "/" + kind
                    selected = [{**testing[i], "method": method, "predicted_success": float(p), "predicted_cpu_seconds": float(s)}
                                for i, p, s in zip(np.flatnonzero(test), probability, seconds)]
                    fold["methods"][method] = metrics(selected); predictions.extend(selected)
            folds.append(fold)
            print(family, {k: round(v["brier"], 4) for k, v in fold["methods"].items()}, flush=True)
        fitted = {}
        if not args.skip_full_fit:
            for basis in args.bases:
                columns = 235 if basis == "physical_horizon" else 320
                fitted[basis] = {kind: SuccessModel(kind).fit(x[:, :columns], y) for kind in args.kinds}
                fitted[basis]["cpu"] = CpuModel().fit(x[:, :columns], cpu)
    if fitted:
        joblib.dump({"models": fitted, "feature_schema": "context_query_320_v1"}, output / "models.joblib")
    groups = defaultdict(list)
    for row in predictions:
        groups[row["method"], "all", "all"].append(row)
        for axis in ["family", "profile", "active_hours", "parent_panel"]:
            groups[row["method"], axis, str(row[axis])].append(row)
    report = {**protocol, "folds": folds, "summary": [{"method": method, "axis": axis, "value": value, **metrics(rows)}
        for (method, axis, value), rows in sorted(groups.items())]}
    (output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")
    (output / "PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in predictions))


if __name__ == "__main__":
    main()
