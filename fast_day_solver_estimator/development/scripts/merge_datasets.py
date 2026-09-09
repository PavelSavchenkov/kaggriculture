"""Merge development physical contracts while retaining their evidence sources."""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("inputs", nargs="+", type=Path)
    args = parser.parse_args()
    merged, provenance, names = {}, [], None
    for path in args.inputs:
        data = json.loads(path.read_text())
        if names is None:
            names = data["feature_names"]
        assert names == data["feature_names"]
        provenance.append({"path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
        for row in data["rows"]:
            key = row["physical_key"]
            if key not in merged:
                merged[key] = {**row, "evidence_datasets": [str(path)], "source_upper_bounds": [row["reference_workers"]]}
                continue
            existing = merged[key]
            assert np.allclose(existing["features"], row["features"], rtol=1e-7, atol=1e-7)
            assert existing["lower_bound"] == row["lower_bound"]
            existing["families"] = sorted(set(existing["families"] + row["families"]))
            existing["sources"] = sorted(set(existing["sources"] + row["sources"]))
            existing["evidence_datasets"].append(str(path)); existing["source_upper_bounds"].append(row["reference_workers"])
            bounds = [v for v in existing["source_upper_bounds"] if v is not None]
            existing["reference_workers"] = min(bounds) if bounds else None
    result = {"feature_names": names, "rows": list(merged.values()), "inputs": provenance,
              "target": "Smallest strictly verified workforce from merged development evidence, not a proven minimum.",
              "scope": "Train only; evaluation must keep every parent family outside training. Original per-run query outcomes remain in the input datasets."}
    args.output.parent.mkdir(parents=True, exist_ok=False)
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"physical_contracts": len(merged), "verified_upper": sum(r["reference_workers"] is not None for r in merged.values())}))


if __name__ == "__main__":
    main()
