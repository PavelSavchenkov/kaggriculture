"""Add audited physical input-release features without changing reference labels."""
import argparse
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("supply", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text())
    supply = json.loads(args.supply.read_text())
    names = list(next(iter(supply.values())))
    assert not set(names) & set(data["feature_names"])
    base_names = [k for k in data["feature_names"] if k != "active_hours"]
    for row in data["rows"]:
        old = dict(zip(data["feature_names"], row["features"]))
        values = supply[row["physical_key"]]
        assert list(values) == names
        hours = row.get("active_hours", 24)
        assert old.get("active_hours", hours) == hours
        row["features"] = [old[k] for k in base_names] + [hours] + [values[k] for k in names]
        row["lower_bound_without_supply"] = row["lower_bound"]
        row["lower_bound"] = max(row["lower_bound"], int(values["supply_lower_bound"]))
        row["input_missing_quantity"] = values["supply_missing"]
        if row["reference_workers"] is not None:
            assert row["lower_bound"] <= row["reference_workers"] and not row["input_missing_quantity"]
    data["feature_names"] = base_names + ["active_hours"] + names
    data["supply_augmentation"] = {"input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.supply]},
                                   "semantics": "Necessary carried-input release conditions. Reference and short-compiler outcomes are unchanged."}
    args.output.parent.mkdir(parents=True, exist_ok=False)
    args.output.write_text(json.dumps(data, indent=2) + "\n")
    print(f"Added supply bounds to {len(data['rows'])} contracts; {len(data['feature_names'])} features")


if __name__ == "__main__":
    main()
