"""Export frozen secondary query controls; never read reference outcomes."""
import csv
import hashlib
import json
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

import joblib
import numpy as np
from threadpoolctl import threadpool_limits

from context_query_models import design


EXP = Path(__file__).resolve().parents[1]


def main():
    snapshot = EXP / "snapshots/planning_wave_v3"
    source = EXP / "runs/holdout_b_inputs_v3/INPUT_DATASET.json"
    data = json.loads(source.read_text()); assert data["schema"] == "unlabeled_planning_inputs_v3"
    assert all(set(q) == {"id", "workers"} for r in data["rows"] for q in r["query_outcomes"])
    output = EXP / "runs/holdout_b_secondary_query_controls_v3"; output.mkdir(exist_ok=False)
    records = {r["id"]: r for r in data["rows"]}
    sys.path.insert(0, str(snapshot / "context_study"))
    models = joblib.load(snapshot / "context_study/models.joblib")["models"]["physical_horizon"]
    ids, workers, features = [], [], []
    for row in records.values():
        for k in range(1, len(row["hire_slots"]) + 2):
            ids.append(row["id"]); workers.append(k); features.append(row["features"])
    matrix = design(np.array(features), np.array(workers), data["feature_names"])[:, :235]
    forecasts = []
    with threadpool_limits(limits=1):
        cpu = models["cpu"].predict(matrix)
        for method in ["boost", "logistic"]:
            ps = models[method].predict(matrix)
            forecasts.extend({"context_id": key, "query_id": f"{key}_w{k:02}", "workers": k,
                              "method": "physical_horizon/" + method, "predicted_success": float(p), "predicted_cpu_seconds": float(t)}
                             for key, k, p, t in zip(ids, workers, ps, cpu))
    manifest = EXP / "runs/holdout_b_inputs_v3/CONTEXT_ORIGINAL.txt"
    binary = snapshot / "build/predict_search"
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                    str(binary), str(manifest), str(output / "OLD_CPP.csv"), "1", "supply"], cwd=EXP, check=True)
    for row in csv.DictReader((output / "OLD_CPP.csv").open()):
        key, k = row["id"], int(row["workers"])
        assert row["pass"] == "0" and key in records
        for method in ["boost", "logistic"]:
            forecasts.append({"context_id": key, "query_id": f"{key}_w{k:02}", "workers": k,
                              "method": "old_ignores_calendar/" + method,
                              "predicted_success": float(row[method]), "predicted_cpu_seconds": float(row["cpu"])})
    assert len(forecasts) == len(ids) * 4
    (output / "PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in forecasts))
    paths = [source, manifest, snapshot / "FREEZE.json", snapshot / "context_study/models.joblib", binary, Path(__file__)]
    report = {"utc": datetime.now(timezone.utc).isoformat(), "predictions": len(forecasts),
              "scope": "Secondary controls with parameters frozen before holdout_b download; predictions exported after reference calls started, before consulting their outcomes. This is a declared export-timing deviation. Primary candidate and calendar cost controls were already frozen before any call.",
              "controls": {"physical_horizon": "Same pre-frozen augmented training data and physical features, horizon and proposed workforce; no selected-hire capacity features.",
                           "old_ignores_calendar": "Exact old frozen query binary, using its original H24 earliest-hire feature computation. H23 and delayed calendars are unsupported transfer diagnostics, not calibrated predictions."},
              "boundary": "Only fixed feature inputs, manifest and frozen weights/binary read. No reference paths, statuses, schedules or costs.",
              "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
    (output / "MANIFEST.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "input_sha256"}, indent=2))


if __name__ == "__main__":
    main()
