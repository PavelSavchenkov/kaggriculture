"""Retain older verified calendar upper bounds without changing timed outcomes."""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("base", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("evidence", nargs="+", type=Path)
    args = parser.parse_args()
    data = json.loads(args.base.read_text()); assert not data["pending_cases"]
    rows = {r["contract_key"]: r for r in data["rows"]}
    original = {k: json.dumps([r["short_sweep_workers"], r["query_outcomes"]], sort_keys=True) for k, r in rows.items()}
    inputs, changes = {}, []
    for path in args.evidence:
        previous = json.loads(path.read_text()); assert not previous["pending_cases"]
        assert previous["feature_names"] == data["feature_names"]
        inputs[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
        for old in previous["rows"]:
            row = rows[old["contract_key"]]
            assert all(row[k] == old[k] for k in ["obligation_key", "active_hours", "hire_slots", "lower_bound"])
            assert np.array_equal(np.float32(row["features"]), np.float32(old["features"]))
            if old["reference_workers"] is None:
                continue
            assert old["reference_workers"] >= row["lower_bound"]
            row.setdefault("additional_upper_evidence", []).append({"dataset": str(path), "sha256": inputs[str(path)],
                "contract_key": old["contract_key"], "workers": old["reference_workers"]})
            if row["reference_workers"] is None or old["reference_workers"] < row["reference_workers"]:
                changes.append({"id": row["id"], "before": row["reference_workers"], "after": old["reference_workers"], "source": str(path)})
                row["reference_workers"] = old["reference_workers"]
    assert all(original[k] == json.dumps([r["short_sweep_workers"], r["query_outcomes"]], sort_keys=True) for k, r in rows.items())
    data["additional_evidence"] = {"scope": "Same full contract, horizon and hire menu only. Preserve older physical certificates; retain the base operational sweep byte-for-byte as serialized fields.",
        "changes": changes, "input_sha256": inputs | {str(args.base): hashlib.sha256(args.base.read_bytes()).hexdigest(),
                                                    str(Path(__file__)): hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}}
    args.output.parent.mkdir(exist_ok=False)
    args.output.write_text(json.dumps(data, indent=2) + "\n")
    print(json.dumps({"contracts": len(rows), "short_certificates": sum(r["short_sweep_workers"] is not None for r in rows.values()),
        "physical_certificates": sum(r["reference_workers"] is not None for r in rows.values()), "changes": changes}, indent=2))


if __name__ == "__main__":
    main()
