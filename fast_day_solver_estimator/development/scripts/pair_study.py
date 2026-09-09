"""Fit and freeze plan-difference predictions with whole-family exclusion."""
import argparse
import hashlib
import json
import shutil
from collections import defaultdict
from pathlib import Path

import numpy as np
from threadpoolctl import threadpool_limits

from baseline_study import cost
from models import Formula
from pair_models import candidates


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
    names = training["feature_names"]
    pools = defaultdict(list)
    for row in proposal["rows"]:
        pools[row["pool"]].append(row)
    pair_rows = []
    for rows in pools.values():
        known = [r for r in rows if r["reference_workers"] is not None and len(r["families"]) == 1]
        for i, a in enumerate(known):
            for b in known[i + 1:]:
                assert a["families"] == b["families"]
                if a["physical_key"] != b["physical_key"]:
                    pair_rows.append((a, b))
    result, counts = [], []
    for family in sorted({r["families"][0] for rows in pools.values() for r in rows}):
        pairs = [(a, b) for a, b in pair_rows if a["families"] != [family]]
        keep = [r for r in training["rows"] if r["reference_workers"] is not None and len(r["families"]) == 1 and r["families"] != [family]]
        left = np.array([a["features"] for a, _ in pairs]); right = np.array([b["features"] for _, b in pairs])
        difference = np.array([b["reference_workers"] - a["reference_workers"] for a, b in pairs])
        cash_difference = np.array([float(cost([b["reference_workers"]])[0] - cost([a["reference_workers"]])[0]) for a, b in pairs])
        test = [r for rows in pools.values() for r in rows if r["families"] == [family]]
        baseline = {pool: next(r for r in rows if r["variant"] == "original") for pool, rows in pools.items()}
        a = np.array([baseline[r["pool"]]["features"] for r in test]); b = np.array([r["features"] for r in test])
        lower = np.array([r["lower_bound"] for r in test])
        with threadpool_limits(limits=1):
            base_model = Formula(names, "timing").fit(np.array([r["features"] for r in keep]), np.array([r["reference_workers"] for r in keep]))
            base_prediction = np.clip(base_model.predict(a), 1, 40)
            for target in ["workers", "cash"]:
                for name, model in candidates(names).items():
                    model.fit(left, right, difference if target == "workers" else cash_difference)
                    delta = model.predict(a, b)
                    if target == "workers":
                        raw = base_prediction + delta
                    else:
                        estimate = np.clip(cost(base_prediction) + delta, 0, float(cost([40])[0]))
                        raw = np.interp(estimate, cost(np.arange(1, 41)), np.arange(1, 41))
                    prediction = np.clip(np.maximum(lower, raw), 1, 40)
                    for row, value, change, before in zip(test, prediction, delta, base_prediction):
                        result.append({"id": row["id"], "method": name + "_" + target, "family": family,
                                       "ranking_score": float(value), "predicted_difference": float(change), "difference_unit": target,
                                       "baseline_prediction": float(before)})
                    # Inference must return exact zero for an identical pair.
                    assert np.max(np.abs(model.predict(a[:3], a[:3]))) == 0
        counts.append({"held_family": family, "training_pairs": len(pairs), "training_absolute": len(keep), "test": len(test)})
        print(family, len(pairs), "training pairs", len(test), "predictions", flush=True)
    (output / "PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in result))
    (output / "MANIFEST.json").write_text(json.dumps({"scope": "Development model selection; full parent-family exclusion from pair and baseline fits.",
        "target": "Differences between verified reference upper bounds, not known minimum differences.", "folds": counts,
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.training, args.proposals]}}, indent=2) + "\n")
    for name in ["pair_models.py", "pair_study.py", "models.py", "baseline_study.py"]:
        shutil.copyfile(EXP / "scripts" / name, output / name)


if __name__ == "__main__":
    main()
