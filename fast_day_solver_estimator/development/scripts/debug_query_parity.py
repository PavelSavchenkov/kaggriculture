"""Diagnose source-feature precision before changing any frozen model."""
import csv
import json
import sys
from pathlib import Path

import joblib
import numpy as np
from threadpoolctl import threadpool_limits

EXP = Path(__file__).resolve().parents[1]
STUDY = EXP / "runs/query_layout_study_v1"
sys.path.insert(0, str(STUDY))
from query_models import design

artifact = joblib.load(STUDY / "models.joblib")
feature_rows = {r["id"]: r for r in csv.DictReader((EXP / "runs/fresh_reference_v0/features_v1.csv").open())}
rows = [r for r in csv.DictReader((EXP / "models/search_candidate_dev_v1/fresh_predictions.csv").open()) if r["pass"] == "0"]
x = np.array([[float(feature_rows[r["id"]][n]) for n in artifact["feature_names"]] for r in rows])
k = np.array([int(r["workers"]) for r in rows]); lower = np.array([int(r["lower_bound"]) for r in rows])
report = {}
with threadpool_limits(limits=1):
    for precision, features in [("csv_float64", x), ("canonical_float32", x.astype(np.float32).astype(np.float64))]:
        q = design(features, k, lower, artifact["feature_names"])
        errors = {}
        for name in ["logistic", "boost", "cpu"]:
            predicted = artifact["models"][name].predict(q)
            actual = np.array([float(r[name]) for r in rows])
            error = np.abs(predicted - actual)
            worst = int(error.argmax())
            errors[name] = {"max_absolute": float(error.max()), "different_by_more_than_1e_7": int((error > 1e-7).sum()),
                            "worst_id": rows[worst]["id"], "worst_workers": int(k[worst]),
                            "python": float(predicted[worst]), "cpp": float(actual[worst])}
        report[precision] = errors
target = EXP / "models/search_candidate_dev_v1/PRECISION_DIAGNOSIS.json"
target.write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(report, indent=2))
