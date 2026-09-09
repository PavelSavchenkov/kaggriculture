"""Check C++/Python query features and selection-context invariants."""
import argparse
import csv
import hashlib
import json
import subprocess
from pathlib import Path

import numpy as np

from context_query_models import design


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("data", type=Path)
    parser.add_argument("name")
    parser.add_argument("--features", nargs="+", type=Path)
    args = parser.parse_args()
    output = EXP / "runs" / args.name; output.mkdir(exist_ok=False)
    paths = args.features if args.features else [args.data / f"FEATURES_{h}.csv" for h in [23, 24]]
    records = [r for path in paths for r in csv.DictReader(path.open())]
    old_names = [k for k in records[0] if k not in ["id", "extraction_us"]]
    names = ["available_hire_slots" if k == "optional_hire_slots" else k.replace("optional_birth_", "selection_birth_") for k in old_names] + ["committed_workforce"]
    arrays, workers, ids = [], [], []
    for row in records:
        values = [float(row[k]) for k in old_names] + [1.0]
        for k in range(1, int(float(row["optional_hire_slots"])) + 2):
            arrays.append(values); workers.append(k); ids.append(row["id"])
    x = np.asarray(arrays, dtype=np.float32)
    expected = design(x, workers, names)
    manifest = output / "INPUT.txt"
    manifest.write_text("".join(f"{key} {k} " + " ".join(map(str, map(float, row))) + "\n" for key, k, row in zip(ids, workers, x)))
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                    str(EXP / "build/check_context_query"), str(manifest), str(output / "CPP.csv")], check=True)
    rows = list(csv.DictReader((output / "CPP.csv").open()))
    assert [r["id"] for r in rows] == ids and [int(r["workers"]) for r in rows] == workers
    actual = np.array([[float(row[f"q{i}"]) for i in range(expected.shape[1])] for row in rows])
    error = np.abs(actual - expected)
    assert np.all(error <= 1e-12 * np.maximum(1, np.abs(expected))), float(error.max())
    # Python also ignores all unused optional slots and their lower-bound cuts.
    changed = x.copy(); ix = {name: i for i, name in enumerate(names)}
    for n, k in enumerate(workers):
        for i in range(k - 1, 39): changed[n, ix[f"selection_birth_{i}"]] = changed[n, ix["active_hours"]]
        changed[n, ix["available_hire_slots"]] = k - 1
        changed[n, ix["committed_workforce"]] = k
    for name in ["planning_lower_bound", "supply_lower_bound", "supply_tight_release", "supply_tight_radius", "release_lower_bound", "release_tight_hour", "release_tight_radius"]:
        changed[:, ix[name]] += 13
    assert np.array_equal(expected, design(changed, workers, names))
    report = {"queries": len(rows), "dimensions": expected.shape[1], "max_absolute_error": float(error.max()),
              "checks": ["C++/Python parity", "unselected-slot isolation", "selected-birth permutation", "mandatory designation does not affect identical cold call"],
              "source_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [EXP / "include/context_query_features.hpp", EXP / "scripts/context_query_models.py", Path(__file__)]}}
    (output / "CHECK.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
