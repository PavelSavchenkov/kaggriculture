"""Compare independent release conditions with existing necessary bounds."""
import argparse
import csv
import hashlib
import json
from pathlib import Path

import numpy as np

from import_problems import physical_key


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("study", type=Path)
    parser.add_argument("source_study", type=Path)
    parser.add_argument("datasets", type=Path, nargs="+")
    args = parser.parse_args()
    by_key, inputs, audit, hashes = {}, [], [], {}
    supply = json.loads((args.source_study / "PHYSICAL_FEATURES.json").read_text())
    for hours in [23, 24]:
        manifest = args.source_study / f"features_{hours}.txt"
        paths = dict(line.split() for line in manifest.read_text().splitlines())
        values = list(csv.DictReader((args.study / f"features_{hours}.csv").open()))
        for row in values:
            key = physical_key(json.loads(Path(paths[row["id"]]).read_text()), hours)
            fields = {k: float(v) for k, v in row.items() if k not in ["id", "active_hours", "release_us"]}
            if key in by_key:
                assert by_key[key] == fields
            by_key[key] = fields
        inputs.extend(values)
        audit.extend(csv.DictReader((args.study / f"audit_{hours}.csv").open()))
        for path in [manifest, args.source_study / f"audit_{hours}.txt", args.study / f"features_{hours}.csv", args.study / f"audit_{hours}.csv"]:
            hashes[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
    summaries = []
    for path in args.datasets:
        data = json.loads(path.read_text())
        cases = []
        for row in data["rows"]:
            bound, old = by_key[row["physical_key"]], supply[row["physical_key"]]
            old_lower = max(row["lower_bound"], old["supply_lower_bound"])
            old_missing = row["deadline_missing_quantity"] > 0 or old["supply_missing"] > 0
            missing = bound["seed_missing"] > 0 or bound["land_missing"] > 0
            if row["reference_workers"] is not None:
                assert not missing and bound["release_lower_bound"] <= row["reference_workers"]
            new_missing = missing and not old_missing
            tightened = not old_missing and not missing and old_lower < bound["release_lower_bound"]
            avoided = [q for q in row["query_outcomes"] if not old_missing and q["workers"] >= old_lower and (missing or q["workers"] < bound["release_lower_bound"])]
            assert all(q["status"] != "FEASIBLE" for q in avoided)
            cases.append({"physical_key": row["physical_key"], "families": row["families"], "reference_workers": row["reference_workers"],
                          "previous_lower_bound": old_lower, "previous_missing": old_missing, **bound,
                          "new_unreachable": new_missing, "new_stronger_bound": tightened,
                          "additional_avoidable_queries": len(avoided), "additional_avoidable_cpu_seconds": sum(q["cpu_seconds"] for q in avoided)})
        summaries.append({"dataset": str(path), "cases": len(cases), "new_unreachable": sum(c["new_unreachable"] for c in cases),
                          "new_stronger_bound": sum(c["new_stronger_bound"] for c in cases),
                          "additional_avoidable_queries": sum(c["additional_avoidable_queries"] for c in cases),
                          "additional_avoidable_cpu_seconds": sum(c["additional_avoidable_cpu_seconds"] for c in cases)})
        (args.study / (path.parent.name + "_CASES.json")).write_text(json.dumps(cases, indent=2) + "\n")
        hashes[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
    times = np.array([float(row["release_us"]) for row in inputs])
    report = {"feature_inputs": len(inputs), "physical_inputs": len(by_key), "audited_witnesses": len(audit),
              "valid_witnesses": sum(int(row["strict"]) for row in audit), "witness_contradictions": 0,
              "microseconds": {"mean": float(times.mean()), "p50": float(np.quantile(times, .5)), "p95": float(np.quantile(times, .95))},
              "timing_scope": "One call including earliest menu, excluding parsing and subsequent audit checks.",
              "datasets": summaries, "input_sha256": hashes}
    (args.study / "REPORT.json").write_text(json.dumps(report, indent=2) + "\n")
    (args.study / "PHYSICAL_FEATURES.json").write_text(json.dumps(by_key, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "input_sha256"}, indent=2))


if __name__ == "__main__":
    main()
