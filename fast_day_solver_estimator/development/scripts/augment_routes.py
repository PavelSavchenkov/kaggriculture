"""Join input-derived route features to physical training contracts."""
import argparse
import csv
import hashlib
import json
from pathlib import Path

import numpy as np


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--indices", type=Path, nargs="+", required=True)
    parser.add_argument("--routes", type=Path, nargs="+", required=True)
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text())
    routes, names = {}, None
    for path in args.routes:
        for row in csv.DictReader(path.open()):
            columns = [k for k in row if k not in ["id", "route_us", "active_hours"]]
            if names is None:
                names = columns
            assert columns == names
            key = row["id"], int(row.get("active_hours", 24))
            values = [float(row[k]) for k in names]
            if key in routes:
                assert routes[key][0] == values
            routes[key] = values, float(row["route_us"])
    physical = {}
    for path in args.indices:
        for row in map(json.loads, path.read_text().splitlines()):
            key = row["source_sha256"][:20], row.get("active_hours", 24)
            if key not in routes:
                continue
            if row["physical_key"] in physical:
                assert physical[row["physical_key"]][0] == routes[key][0]
            physical[row["physical_key"]] = routes[key]
    assert not set(names) & set(data["feature_names"]), "route features already present"
    base = [k for k in data["feature_names"] if k != "active_hours"]
    for row in data["rows"]:
        old = dict(zip(data["feature_names"], row["features"]))
        hours = row.get("active_hours", 24)
        assert old.get("active_hours", hours) == hours
        route, timing = physical[row["physical_key"]]
        row["features"] = [old[k] for k in base] + [hours] + route
        row["route_feature_us"] = timing
        assert np.isfinite(row["features"]).all()
    data["feature_names"] = base + ["active_hours"] + names
    data["route_augmentation"] = {"input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
        for p in [args.dataset] + args.indices + args.routes}, "timing": "Three sequential calls per input in this first route-feature pass; not mixed-input timing.",
        "semantics": "Restricted whole-chain routing features, not bounds or feasibility certificates."}
    args.output.parent.mkdir(parents=True, exist_ok=False)
    args.output.write_text(json.dumps(data, indent=2) + "\n")
    print(f"Augmented {len(data['rows'])} contracts to{len(data['feature_names'])} features")


if __name__ == "__main__":
    main()
