"""Expose one ordinary earliest-menu cost target per physical obligation."""
import argparse
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("context", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data = json.loads(args.context.read_text())
    assert not data["pending_cases"]
    keys = [r["obligation_key"] for r in data["rows"]]
    assert len(set(keys)) == len(keys), "never merge different hire calendars"
    rows = []
    for row in data["rows"]:
        assert row["active_hours"] == 24 and row["profile"] == "earliest"
        rows.append({"id": row["obligation_key"][:20], "physical_key": row["obligation_key"],
            "contract_key": row["contract_key"], "families": row["families"], "sources": row["source_datasets"],
            "features": row["features"], "reference_workers": row["reference_workers"], "lower_bound": row["lower_bound"]})
    output = {"scope": "Exposed development cost fitting with whole-family exclusion. Only ordinary days and the input-derived earliest menu.",
        "target": "Smallest verified workforce upper bound across available evidence. This is neither a proven minimum nor the result of a single fixed-budget call.",
        "rows": rows, "feature_names": data["feature_names"], "pending_cases": [],
        "input_sha256": {str(args.context): hashlib.sha256(args.context.read_bytes()).hexdigest(),
                         str(Path(__file__)): hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}}
    args.output.parent.mkdir(exist_ok=False)
    args.output.write_text(json.dumps(output, indent=2) + "\n")
    print(json.dumps({"contracts": len(rows), "labeled": sum(r["reference_workers"] is not None for r in rows),
                      "families": len({f for r in rows for f in r["families"]}), "features": len(data["feature_names"])}))


if __name__ == "__main__":
    main()
