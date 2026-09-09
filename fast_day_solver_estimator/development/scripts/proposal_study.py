"""Predict related plans with held-family fits; leave net-value ranking separate."""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from threadpoolctl import threadpool_limits

from models import candidates
from screening import screen_reason


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("training", type=Path)
    parser.add_argument("proposals", type=Path)
    parser.add_argument("name")
    args = parser.parse_args()
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    training = json.loads(args.training.read_text()); proposal = json.loads(args.proposals.read_text())
    assert training["feature_names"] == proposal["feature_names"]
    names = training["feature_names"]; ix = {name: i for i, name in enumerate(names)}
    predictions, folds = [], []
    for family in sorted({r["families"][0] for r in proposal["rows"]}):
        keep = [r for r in training["rows"] if r["reference_workers"] is not None and len(r["families"]) == 1 and r["families"] != [family]]
        test = [r for r in proposal["rows"] if r["families"] == [family]]
        x = np.array([r["features"] for r in keep]); y = np.array([r["reference_workers"] for r in keep])
        z = np.array([r["features"] for r in test]); lower = np.array([r["lower_bound"] for r in test])
        values = {}
        with threadpool_limits(limits=1):
            for name, model in candidates(names).items():
                model.fit(x, y)
                values[name] = np.clip(np.maximum(lower, model.predict(z)), 1, 40)
        for coefficient in [0, 10, 25]:
            values[f"original_flat_{coefficient}"] = coefficient * z[:, ix["tasks"]]
        if all(r.get("original_geometry_workers") is not None for r in test):
            values["original_geometry"] = np.array([r["original_geometry_workers"] for r in test])
        for name, scores in values.items():
            for row, score in zip(test, scores):
                predictions.append({"id": row["id"], "pool": row["pool"], "variant": row["variant"], "family": family,
                    "method": name, "ranking_score": float(score), "screened": screen_reason(row) is not None,
                    "screen_reason": screen_reason(row),
                    "workers": row["short_sweep_workers"], "cpu_seconds": row["actual_cpu_seconds"], "nominal_seconds": row["nominal_budget_seconds"]})
        folds.append({"held_family": family, "training": len(keep), "test": len(test)})
        print(family, len(test), "proposal predictions complete", flush=True)
    (output / "PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in predictions))
    report = {"scope": "Development full-family exclusion; models see neither the parent nor other members of the held family.",
              "warning": "These proposals can have different output value; labor-only scores are not net-value rankings.", "folds": folds,
              "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.training, args.proposals]}}
    (output / "MANIFEST.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
