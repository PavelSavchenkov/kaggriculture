"""Family-exclusion study of conditional long-retry probabilities."""
import hashlib
import json
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

import joblib
import numpy as np
from sklearn.metrics import brier_score_loss, log_loss, roc_auc_score
from threadpoolctl import threadpool_limits

from long_retry_models import design
from query_models import SuccessModel


EXP = Path(__file__).resolve().parents[1]


def metrics(rows):
    y = np.array([r["success"] for r in rows]); p = np.array([r["probability"] for r in rows])
    result = {"calls": len(rows), "physical_contracts": len({r["physical_key"] for r in rows}), "recoveries": int(y.sum()),
              "brier": float(brier_score_loss(y, p)), "log_loss": float(log_loss(y, p, labels=[False, True])),
              "observed_recovery": float(y.mean()), "predicted_recovery": float(p.mean()), "probability_bias": float((p - y).mean()),
              "auc": float(roc_auc_score(y, p)) if len(set(y)) == 2 else None,
              "cpu_mae_seconds": float(np.mean([abs(r["predicted_cpu_seconds"] - r["long_cpu_seconds"]) for r in rows])),
              "cpu_bias_seconds": float(np.mean([r["predicted_cpu_seconds"] - r["long_cpu_seconds"] for r in rows]))}
    result["deferral"] = []
    for cutoff in [.01, .02, .05, .1, .2, .5]:
        mask = p < cutoff
        result["deferral"].append({"cutoff": cutoff, "flagged": int(mask.sum()), "flagged_fraction": float(mask.mean()),
                                   "deferred_recoveries": int(y[mask].sum()), "retained_recovery_fraction": float(y[~mask].sum() / y.sum()) if y.any() else None,
                                   "recorded_retry_cpu_avoided": sum(r["long_cpu_seconds"] for r, flagged in zip(rows, mask) if flagged)})
    return result


def main():
    source = EXP / "runs/long_retry_dataset_v1/DATASET.json"
    data = json.loads(source.read_text()); rows = data["rows"]
    assert not any(r["necessary_screen"] or len(r["families"]) != 1 for r in rows)
    output = EXP / "runs/long_retry_study_v1"; output.mkdir(exist_ok=False)
    families = np.array([r["families"][0] for r in rows]); names = sorted(set(families))
    specifications = [("prior_only", "logistic"), ("physical", "logistic"), ("physical", "boost"), ("with_prior", "logistic"), ("with_prior", "boost")]
    matrices = {view: design(rows, data["feature_names"], view) for view, _ in specifications}
    altered = [{**r, "success": not r["success"], "long_cpu_seconds": 1e9, "source_workers": 999,
                "phase": "changed", "panel": "changed", "families": ["changed"]} for r in rows]
    for view, matrix in matrices.items(): assert np.array_equal(matrix, design(altered, data["feature_names"], view))
    truth = np.array([r["success"] for r in rows]); cpu = np.array([r["long_cpu_seconds"] for r in rows])
    predictions, folds = [], []
    with threadpool_limits(limits=1):
        for family in names:
            test = families == family; train = ~test
            assert not ({r["physical_key"] for r, use in zip(rows, test) if use} & {r["physical_key"] for r, use in zip(rows, train) if use})
            assert truth[train].any() and (~truth[train]).any()
            mean_failed = float(cpu[train & ~truth].mean()); mean_success = float(cpu[train & truth].mean())
            scored = {"train_prior": np.full(test.sum(), truth[train].mean()),
                      "old_three_second_oof": np.array([r["old_three_second_oof_probability"] for r, use in zip(rows, test) if use])}
            for view, kind in specifications:
                model = SuccessModel(kind).fit(matrices[view][train], truth[train])
                scored[view + "/" + kind] = model.predict(matrices[view][test])
            indices = np.flatnonzero(test)
            for method, probabilities in scored.items():
                for index, probability in zip(indices, probabilities):
                    row = rows[index]
                    predictions.append({k: row[k] for k in ["sample_id", "query_id", "physical_key", "contract_key", "workers", "panel", "phase", "round", "prior_upper_workers", "success", "long_cpu_seconds"]} |
                                       {"family": family, "method": method, "probability": float(probability),
                                        "predicted_cpu_seconds": mean_failed * (1 - probability) + mean_success * probability})
            folds.append({"family": family, "train_calls": int(train.sum()), "test_calls": int(test.sum()), "test_recoveries": int(truth[test].sum()),
                          "training_mean_failed_cpu": mean_failed, "training_mean_success_cpu": mean_success})
            print(family, "complete", flush=True)
        models = {view + "/" + kind: SuccessModel(kind).fit(matrices[view], truth) for view, kind in specifications}
    groups = defaultdict(list)
    for row in predictions:
        groups[row["method"], "all", "all"].append(row)
        for axis in ["family", "panel", "phase"]: groups[row["method"], axis, row[axis]].append(row)
        groups[row["method"], "prior_certificate", str(row["prior_upper_workers"] is not None)].append(row)
    summary = [{"method": method, "axis": axis, "value": value, **metrics(values)} for (method, axis, value), values in sorted(groups.items())]
    report = {"utc": datetime.now(timezone.utc).isoformat(), "scope": "Development whole-family exclusion on 1,333 conditional retry observations from 440 physical contracts. Repeated attempts stay in their physical family; this is not a fresh-family test.",
              "target": "Thirty-second verified recovery after the same exact query failed at three seconds.",
              "inputs": "Selected physical query features, optionally known prior certificate and observed failed-call timing. Families, panels, phases, source workforce and long outcomes are excluded from features. The old three-second control uses existing whole-family-excluded forecasts, not a refit on long labels.",
              "cpu_model": "Probability mixture of arithmetic mean successful and unsuccessful long-call CPU from the training families only; no mean-log claim.",
              "decision_limit": "Deferral summaries show lost recoveries and recorded CPU avoided. They are not full optimization speed gains. Evaluate fixed-budget decisions before choosing or exporting a candidate.",
              "families": len(names), "calls": len(rows), "input_boundary_checks": len(rows) * len(matrices), "folds": folds, "summary": summary,
              "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [source, Path(__file__), EXP / "scripts/long_retry_models.py", EXP / "scripts/query_models.py"]}}
    (output / "RESULTS.json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    (output / "PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in predictions))
    joblib.dump({"models": models, "feature_names": data["feature_names"], "failed_mean_cpu": float(cpu[~truth].mean()), "success_mean_cpu": float(cpu[truth].mean())}, output / "models.joblib")
    for name in ["long_retry_models.py", "query_models.py", "context_query_models.py", "long_retry_study.py"]:
        (output / name).write_bytes((EXP / "scripts" / name).read_bytes())
    print(json.dumps([{k: v for k, v in r.items() if k != "deferral"} for r in summary if r["axis"] == "all"], indent=2))


if __name__ == "__main__":
    main()
