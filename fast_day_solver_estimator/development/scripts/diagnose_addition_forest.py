"""Explain frozen addition forecasts using their exact training-leaf weights."""
import hashlib
import json
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

import joblib
import numpy as np
from threadpoolctl import threadpool_limits

from baseline_study import cost


EXP = Path(__file__).resolve().parents[1]
PANELS = {
    "owned": ("expansion_owned_dataset_guided_v4", "expansion_owned_guided_pairs_v4"),
    "large": ("expansion_dataset_guided_v3", "expansion_guided_physical_pairs_v4"),
}


def read_json(path):
    return json.loads(path.read_text())


def rows(path):
    return [json.loads(line) for line in path.read_text().splitlines()]


def main():
    output = EXP / "runs/addition_forest_diagnosis_v1"
    output.mkdir(exist_ok=False)
    fit = EXP / "runs/cost_model_study_v3"
    train_data = read_json(fit / "DATASET.json")
    artifact = joblib.load(fit / "models.joblib")
    names = train_data["feature_names"]
    assert names == artifact["feature_names"]
    train = [r for r in train_data["rows"] if r["reference_workers"] is not None and len(r["families"]) == 1]
    x = np.array([r["features"] for r in train])
    y = np.array([r["reference_workers"] for r in train])
    prices = cost(y)
    forest = artifact["models"]["cost_direct_extra"].model
    leaves = forest.apply(x)
    trees = forest.estimators_
    assert len(trees) == 128 and trees[0].tree_.n_node_samples[0] == len(train)
    leaf_members = []
    max_reconstruction_error = 0.
    for t, tree in enumerate(trees):
        members = {int(leaf): np.flatnonzero(leaves[:, t] == leaf) for leaf in np.unique(leaves[:, t])}
        leaf_members.append(members)
        for leaf, indices in members.items():
            difference = abs(tree.tree_.value[leaf, 0, 0] - prices[indices].mean())
            max_reconstruction_error = max(max_reconstruction_error, float(difference))
    assert max_reconstruction_error < 1e-8
    lower, upper = x.min(axis=0), x.max(axis=0)
    scale = np.maximum(1., x.std(axis=0))
    training_ids = {r["id"] for r in train}
    paths = [fit / name for name in ["DATASET.json", "models.joblib", "models.py", "model_study.py"]]
    paths += [Path(__file__), EXP / "scripts/models.py", EXP / "scripts/baseline_study.py"]
    records, cases, parity_errors = [], [], []
    for panel, (dataset, pair_report) in PANELS.items():
        dataset_path = EXP / "runs" / dataset / "DATASET.json"
        errors_path = EXP / "runs" / pair_report / "ERRORS.jsonl"
        paths += [dataset_path, errors_path]
        data = read_json(dataset_path)
        assert data["feature_names"] == names and not data["pending_cases"]
        z = np.array([r["features"] for r in data["rows"]])
        by_id = {r["id"]: i for i, r in enumerate(data["rows"])}
        assert len(by_id) == len(data["rows"])
        lb = np.array([r["lower_bound"] for r in data["rows"]])
        predictions = {}
        for method in ["cost_direct_extra", "fitted_timing", "timing_extra", "timing_boost"]:
            raw = artifact["models"][method].predict(z)
            predictions[method] = np.clip(np.maximum(lb, raw), 1, 40)
        z_leaves = forest.apply(z)
        votes = np.column_stack([tree.tree_.value[z_leaves[:, t], 0, 0] for t, tree in enumerate(trees)])
        tree_cost = forest.predict(z)
        assert np.max(abs(votes.mean(axis=1) - tree_cost)) < 1e-8
        for i, row in enumerate(data["rows"]):
            weights = np.zeros(len(train))
            for t in range(len(trees)):
                indices = leaf_members[t][int(z_leaves[i, t])]
                weights[indices] += 1 / (len(trees) * len(indices))
            assert abs(weights.sum() - 1) < 1e-12
            assert abs(weights @ prices - tree_cost[i]) < 1e-8
            outside = np.maximum(lower - z[i], np.maximum(z[i] - upper, 0)) / scale
            changed = np.flatnonzero(outside > 1e-9)
            distance = np.mean(((x - z[i]) / scale) ** 2, axis=1) ** .5
            nearest = np.argsort(distance)[:5]
            top = np.argsort(weights)[-8:][::-1]
            record = {
                "panel": panel, "id": row["id"], "families": row["families"],
                "in_training": row["id"] in training_ids,
                "reference_workers": row["reference_workers"], "lower_bound": row["lower_bound"],
                "point_workers": float(predictions["cost_direct_extra"][i]),
                "raw_forest_cash": float(tree_cost[i]),
                "effective_training_support": float(1 / (weights @ weights)),
                "training_weight_at_least_15_workers": float(weights[y >= 15].sum()),
                "outside_training_feature_count": len(changed),
                "outside_training_max_scaled_gap": float(outside.max()),
                "outside_training_features": [
                    {"name": names[j], "value": float(z[i, j]), "min": float(lower[j]),
                     "max": float(upper[j]), "scaled_gap": float(outside[j])}
                    for j in sorted(changed, key=lambda j: outside[j], reverse=True)],
                "nearest_training_rows": [
                    {"id": train[j]["id"], "workers": int(y[j]), "distance": float(distance[j])}
                    for j in nearest],
                "highest_weight_training_rows": [
                    {"id": train[j]["id"], "workers": int(y[j]), "weight": float(weights[j])}
                    for j in top],
            }
            cases.append(record)
        by_case = {r["id"]: r for r in cases if r["panel"] == panel}
        for error in rows(errors_path):
            if error["method"] not in predictions or error["mode"] != "cold":
                continue
            a, b = by_id[error["baseline"]], by_id[error["candidate"]]
            prediction = float(np.diff(cost(predictions[error["method"]][[a, b]]))[0])
            parity_errors.append(abs(prediction - error["prediction"]))
            assert parity_errors[-1] < 1e-7, (panel, error["candidate"], error["method"], prediction, error["prediction"])
            if error["method"] != "cost_direct_extra":
                continue
            delta = votes[b] - votes[a]
            feature_delta = (z[b] - z[a]) / scale
            feature_order = np.argsort(abs(feature_delta))[::-1]
            method_forecasts = {method: {
                "baseline_workers": float(p[a]), "candidate_workers": float(p[b]),
                "marginal_cash": float(np.diff(cost(p[[a, b]]))[0]),
            } for method, p in predictions.items()}
            records.append({
                **error, "panel": panel, "forecasts": method_forecasts,
                "same_leaf_fraction": float(np.mean(z_leaves[a] == z_leaves[b])),
                "trees_increasing_cost": int(sum(delta > 1e-9)),
                "trees_decreasing_cost": int(sum(delta < -1e-9)),
                "positive_tree_mean_contribution": float(np.maximum(delta, 0).mean()),
                "negative_tree_mean_contribution": float(np.minimum(delta, 0).mean()),
                "changed_features": int(sum(abs(feature_delta) > 1e-9)),
                "largest_scaled_feature_changes": [
                    {"name": names[j], "baseline": float(z[a, j]), "candidate": float(z[b, j]),
                     "scaled_change": float(feature_delta[j])}
                    for j in feature_order[:12] if abs(feature_delta[j]) > 1e-9],
                "baseline_support": by_case[error["baseline"]],
                "candidate_support": by_case[error["candidate"]],
            })
    assert len(parity_errors) == 4 * len(records)
    summary = []
    for panel in PANELS:
        all_rows = [r for r in records if r["panel"] == panel]
        for slice_name, selected in [
            ("all_known", all_rows),
            ("underestimate_at_least_500", [r for r in all_rows if r["error"] <= -500]),
            ("other_known", [r for r in all_rows if r["error"] > -500]),
        ]:
            if not selected:
                continue
            summary.append({
                "panel": panel, "slice": slice_name, "pairs": len(selected),
                "mae": float(np.mean([abs(r["error"]) for r in selected])),
                "bias": float(np.mean([r["error"] for r in selected])),
                "negative_marginal_predictions": sum(r["prediction"] < 0 for r in selected),
                "median_same_leaf_fraction": float(np.median([r["same_leaf_fraction"] for r in selected])),
                "all_features_within_training_range": sum(r["candidate_support"]["outside_training_feature_count"] == 0 for r in selected),
                "candidate_above_largest_training_label": sum(r["candidate_workers"] > int(max(y)) for r in selected),
                "median_high_worker_training_weight": float(np.median([r["candidate_support"]["training_weight_at_least_15_workers"] for r in selected])),
                "physical_marginal_interval_contains_zero": sum(r["physical_delta_lower"] <= 0 <= r["physical_delta_upper"] for r in selected),
            })
    report = {
        "utc": datetime.now(timezone.utc).isoformat(),
        "scope": "Post hoc diagnosis of exposed addition panels. No fitting, threshold selection, or fresh holdout outcomes.",
        "target": "Differences between verified upper bills, not proven minimum marginal cost.",
        "training_rows": len(train), "training_workers": dict(sorted(Counter(map(int, y)).items())),
        "training_min_workers": int(min(y)), "training_max_workers": int(max(y)),
        "max_training_cost": float(max(prices)),
        "max_tree_leaf_cost": float(max(tree.tree_.value[tree.tree_.children_left == -1, 0, 0].max() for tree in trees)),
        "all_training_leaf_mean_max_error": max_reconstruction_error,
        "prediction_parity_checks": len(parity_errors), "prediction_parity_max_error": max(parity_errors),
        "summary": summary,
        "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
    }
    (output / "REPORT.json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    (output / "PAIRS.jsonl").write_text("".join(json.dumps(r, sort_keys=True, allow_nan=False) + "\n" for r in records))
    (output / "CASES.jsonl").write_text("".join(json.dumps(r, sort_keys=True, allow_nan=False) + "\n" for r in cases))
    worst = sorted(records, key=lambda r: r["error"])[:12]
    (output / "WORST.json").write_text(json.dumps(worst, indent=2, allow_nan=False) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "input_sha256"}, indent=2))


if __name__ == "__main__":
    with threadpool_limits(limits=1):
        main()
