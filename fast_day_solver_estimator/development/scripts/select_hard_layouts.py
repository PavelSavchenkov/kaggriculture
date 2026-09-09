"""Freeze deeper reference work for costly development ranking disagreements."""
import argparse
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("study", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text())
    decisions = [r for l in (args.study / "DECISIONS.jsonl").open() if (r := json.loads(l))["mode"] == "cold" and r["candidate_budget"] == 1]
    severity = {}
    for row in decisions:
        value = row["regret"] if row["regret"] is not None else 1000
        severity[row["pool"]] = max(severity.get(row["pool"], 0), value)
    selected = sorted(severity, key=lambda k: (-severity[k], k))[:12]
    data["rows"] = [r for r in data["rows"] if r["pool"] in selected]
    data["hard_case_selection"] = {"rule": "Twelve development pools with largest top-one regret or no-certificate choices across all compared methods.",
                                    "selected": selected, "source_dataset_sha256": hashlib.sha256(args.dataset.read_bytes()).hexdigest(),
                                    "warning": "Purposeful diagnostic subset, not a representative test set."}
    args.output.write_text(json.dumps(data, indent=2) + "\n")
    print(f"Selected{len(selected)} development pools/{len(data['rows'])} alternatives")


if __name__ == "__main__":
    main()
