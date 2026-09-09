"""Add input-only necessary bounds without changing recorded compiler calls."""
import argparse
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("bounds", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text()); bounds = json.loads(args.bounds.read_text())
    for row in data["rows"]:
        bound = bounds[row["id"]]
        assert bound["lower_bound"] >= row["lower_bound"]
        if row["reference_workers"] is not None:
            assert bound["lower_bound"] <= row["reference_workers"]
            assert not bound["deadline_missing_quantity"] and not bound["input_missing_quantity"]
        row.update(bound)
    data["added_necessary_bounds"] = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.bounds]}
    args.output.parent.mkdir(exist_ok=False)
    args.output.write_text(json.dumps(data, indent=2) + "\n")


if __name__ == "__main__":
    main()
