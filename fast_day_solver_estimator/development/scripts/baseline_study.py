"""Development comparison against completed, bounded-search reference sweeps."""
import argparse
import csv
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

import numpy as np
from scipy.optimize import nnls
from sklearn.ensemble import ExtraTreesRegressor, HistGradientBoostingRegressor, RandomForestRegressor
from sklearn.linear_model import Ridge
from sklearn.neighbors import KNeighborsRegressor
from sklearn.pipeline import make_pipeline
from sklearn.preprocessing import StandardScaler
from threadpoolctl import threadpool_limits


EXP = Path(__file__).resolve().parents[1]


def read_rows(path):
    return [json.loads(line) for line in path.read_text().splitlines(keepends=True) if line.endswith("\n")]


def cost(workers):
    prices, a, b = [0.0], 1, 1
    for _ in range(39):
        prices.append(prices[-1] + a)
        a, b = b, a + b
    return np.interp(np.clip(workers, 1, 40), np.arange(1, 41), prices)


def score(truth, prediction):
    gap = cost(prediction) - cost(truth)
    return {"worker_mae": float(np.mean(np.abs(truth - prediction))),
            "worker_rmse": float(np.mean((truth - prediction) ** 2) ** .5),
            "worker_bias": float(np.mean(prediction - truth)),
            "reference_cost_mae": float(np.mean(np.abs(gap))),
            "under_by_2_workers": float(np.mean(prediction < truth - 2)),
            "under_by_500_cash": float(np.mean(gap < -500))}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("--reference", default="reference_v0")
    parser.add_argument("--index", type=Path, default=EXP / "runs/inventory_v0/problems.jsonl")
    parser.add_argument("--features", type=Path, default=EXP / "runs/inventory_v0/features.csv")
    args = parser.parse_args()
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    root = EXP / "runs" / args.reference
    cases = json.loads((root / "CASES.json").read_text())
    outcomes = {row["id"]: row for path in (root / "initial_3s").glob("worker*/results.jsonl") for row in read_rows(path)}
    inputs = read_rows(args.index)
    features = {r["id"]: r for r in csv.DictReader(args.features.open())}
    columns = [k for k in next(iter(features.values())) if k not in ["id", "extraction_us"]]
    physical_features = {r["physical_key"]: features[r["source_sha256"][:20]] for r in inputs}
    selected, skipped = [], []
    for case in cases:
        ids = [f"{case['id']}_w{q['workers']:02}" for q in case["queries"]]
        if not all(key in outcomes for key in ids):
            continue
        successful = [outcomes[key]["workers"] for key in ids if outcomes[key]["status"] == "FEASIBLE"]
        if not successful:
            skipped.append({"id": case["id"], "reason": "no_verified_upper_bound"})
            continue
        f = physical_features[case["physical_key"]]
        families = case.get("source_families", [])
        if families and families != ["historical"]:
            if len(families) != 1:
                skipped.append({"id": case["id"], "reason": "shared_across_families"})
                continue
            group = families[0]
        else:
            group = "root_solver" if any(s.startswith("benchmarks/") for s in case["sources"]) else "composition_archive"
        selected.append({"id": case["id"], "reference_workers": min(successful), "group": group,
                         "lower_bound": max(case["task_only_lower_bound"], 1 + int(np.ceil(max(0, float(f["task_motion_bound"]) - 24) / 23))),
                         "features": [float(f[k]) for k in columns]})
    if len(selected) < 40:
        raise ValueError("too few completed references for a useful development comparison")
    (output / "DATASET.json").write_text(json.dumps({"feature_names": columns, "rows": selected, "unresolved": skipped}, indent=2) + "\n")
    x = np.array([r["features"] for r in selected]); y = np.array([r["reference_workers"] for r in selected])
    groups = np.array([r["group"] for r in selected]); bound = np.array([r["lower_bound"] for r in selected])
    ix = {name: i for i, name in enumerate(columns)}
    tasks = x[:, ix["tasks"]]
    # Adapt the old aggregate-work formula to the same explicit work contract.
    # It is not a claim of byte-for-byte whole-composition estimator parity.
    transport = (2 * (x[:, ix["input_distance"]] + x[:, ix["output_distance"]])
                 + x[:, ix["withdrawal_total"]]) / 12
    proxy = 2 * tasks + transport + 2 * x[:, ix["distance_max"]]
    formula = np.maximum(1, np.ceil(proxy / 22))
    geometry = np.column_stack([np.ones(len(x)), tasks, x[:, ix["rooted_mst"]], transport, x[:, ix["distance_max"]],
                                x[:, ix["early_withdrawal_deficit"]]])
    folds = []
    for group in sorted(set(groups)):
        train, test = groups != group, groups == group
        if sum(train) < 20 or sum(test) < 10:
            continue
        predictions = {"constant": np.full(sum(test), np.mean(y[train])), "old_work_formula": formula[test]}
        task_design = np.column_stack([np.ones(len(x)), tasks])
        task_coefficient, _ = nnls(task_design[train], y[train])
        predictions["fitted_tasks"] = task_design[test] @ task_coefficient
        coefficient, _ = nnls(geometry[train], y[train])
        predictions["fitted_geometry"] = geometry[test] @ coefficient
        models = {
            "ridge": make_pipeline(StandardScaler(), Ridge(alpha=20)),
            "pattern_5": make_pipeline(StandardScaler(), KNeighborsRegressor(n_neighbors=5, weights="distance")),
            "extra_trees": ExtraTreesRegressor(n_estimators=128, max_depth=14, min_samples_leaf=3, random_state=909, n_jobs=1),
            "random_forest": RandomForestRegressor(n_estimators=128, max_depth=14, min_samples_leaf=3, random_state=909, n_jobs=1),
            "boosted": HistGradientBoostingRegressor(max_iter=160, max_leaf_nodes=15, learning_rate=.06,
                                                     l2_regularization=5, early_stopping=False, random_state=909),
        }
        for name, model in models.items():
            with threadpool_limits(limits=1):
                model.fit(x[train], y[train])
                predictions[name] = model.predict(x[test])
        rows = []
        for name, prediction in predictions.items():
            prediction = np.maximum(bound[test], prediction)
            rows.append({"method": name, **score(y[test], prediction)})
        folds.append({"held_group": str(group), "training_cases": int(sum(train)), "test_cases": int(sum(test)), "results": rows})
    report = {"utc": datetime.now(timezone.utc).isoformat(), "cases": len(selected), "skipped_cases": len(skipped),
              "target": "Cheapest strictly verified workforce in the completed frozen 3-second-per-query sweep; not proven minimum.",
              "scope": "Exposed development diagnosis. Historical groups share broad lineage; these results are not a final unseen claim.",
              "dataset_sha256": hashlib.sha256((output / "DATASET.json").read_bytes()).hexdigest(), "folds": folds}
    (output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")
    for fold in folds:
        print(fold["held_group"], fold["test_cases"], {r["method"]: round(r["worker_mae"], 3) for r in fold["results"]})


if __name__ == "__main__":
    main()
