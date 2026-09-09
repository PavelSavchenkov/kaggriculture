"""Forecast an explicit catalog of calls without reading their outcomes."""
import argparse
import hashlib
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

import joblib
import numpy as np
from threadpoolctl import threadpool_limits

from context_query_models import design


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("study", type=Path)
    parser.add_argument("dataset", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--scope", required=True)
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text()); assert not data["pending_cases"]
    sys.path.insert(0, str(args.study.resolve()))
    models = joblib.load(args.study / "models.joblib")["models"]["calendar"]
    catalog, x, counts = [], [], []
    for row in data["rows"]:
        for query in row["query_outcomes"]:
            # Only action identifiers and explicitly proposed workers are read.
            catalog.append({"query_id": query["id"], "workers": query["workers"], "context_id": row["id"]})
            x.append(row["features"]); counts.append(query["workers"])
    assert len({r["query_id"] for r in catalog}) == len(catalog), "use one logged batch per call id for this exporter"
    matrix = design(np.array(x), np.array(counts), data["feature_names"])
    predictions = []
    with threadpool_limits(limits=1):
        seconds = models["cpu"].predict(matrix)
        for method in ["boost", "logistic"]:
            probabilities = models[method].predict(matrix)
            predictions.extend({**r, "method": method, "predicted_success": float(p), "predicted_cpu_seconds": float(t)}
                               for r, p, t in zip(catalog, probabilities, seconds))
    args.output.mkdir(exist_ok=False)
    target = args.output / "PREDICTIONS.jsonl"
    target.write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in predictions))
    report = {"utc": datetime.now(timezone.utc).isoformat(), "scope": args.scope, "calls": len(catalog), "predictions": len(predictions),
        "input_boundary": "Pure forecasts use physical/context features and the proposed k. Stored statuses, costs, source workforces and observed runtimes do not enter predictions.",
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.study / "models.joblib", args.dataset, Path(__file__)]}}
    (args.output / "MANIFEST.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "input_sha256"}, indent=2))


if __name__ == "__main__":
    main()
