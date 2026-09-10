import argparse
import json
from pathlib import Path

import numpy as np


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--baseline", default="parent")
    parser.add_argument("--candidate", default="compact_sales")
    args = parser.parse_args()
    rng = np.random.default_rng(202609091850)
    results = []
    for panel in ("independent", "native"):
        for opponent in ("pass", "nanare", "ahmed_v25"):
            baseline = json.loads((args.directory / f"{panel}_{args.baseline}_{opponent}.json").read_text())["games"]
            candidate = json.loads((args.directory / f"{panel}_{args.candidate}_{opponent}.json").read_text())["games"]
            original = {(r["seed"], r["seat"]): r for r in baseline}
            by_seed = {}
            for row in candidate:
                old = original[row["seed"], row["seat"]]
                gain = (row["cash"] - row["opponent_cash"]) - (old["cash"] - old["opponent_cash"])
                score = lambda r: float(r["cash"] > r["opponent_cash"]) + 0.5 * (r["cash"] == r["opponent_cash"])
                by_seed.setdefault(row["seed"], []).append([gain, score(row) - score(old), row["cash"] - old["cash"]])
            values = np.array([np.mean(by_seed[seed], axis=0) for seed in sorted(by_seed)])
            sample = values[rng.integers(0, len(values), size=(10000, len(values)))].mean(axis=1)
            bounds = np.quantile(sample, [0.025, 0.975], axis=0)
            results.append({"panel": panel, "opponent": opponent, "seed_clusters": len(values),
                            "mean": dict(zip(("margin", "win_utility", "cash"), values.mean(axis=0).tolist())),
                            "ci95": {key: bounds[:, i].tolist() for i, key in enumerate(("margin", "win_utility", "cash"))}})
    report = {"scope": "Development screening on three exposed opponents; seed-cluster bootstrap,10000resamples. This is not source-family generalization or final promotion.", "results": results}
    (args.directory / "STATISTICS.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
