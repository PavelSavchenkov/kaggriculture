"""Keep unseen physical contracts, removing whole contaminated proposal pools."""
import argparse
import hashlib
import json
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("overlap", type=Path)
    parser.add_argument("name")
    parser.add_argument("--pools", action="store_true")
    args = parser.parse_args()
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    data = json.loads(args.dataset.read_text())
    assert not data["pending_cases"], "freeze only the completed wave"
    audit = [json.loads(line) for line in args.overlap.read_text().splitlines()]
    seen = {r["physical_key"] for r in audit if r["exposed_overlap"]}
    assert {r["physical_key"] for r in data["rows"]} <= {r["physical_key"] for r in audit}
    pools = {r["pool"] for r in data["rows"] if r["physical_key"] in seen}
    before = len(data["rows"])
    data["rows"] = [r for r in data["rows"] if r["pool"] not in pools] if args.pools else [r for r in data["rows"] if r["physical_key"] not in seen]
    data["novelty_filter"] = {"source_sha256": hashlib.sha256(args.dataset.read_bytes()).hexdigest(),
                              "audit_sha256": hashlib.sha256(args.overlap.read_bytes()).hexdigest(),
                              "remove_whole_pools": args.pools, "excluded_rows": before - len(data["rows"]),
                              "excluded_pools": sorted(pools) if args.pools else []}
    (output / "DATASET.json").write_text(json.dumps(data, indent=2) + "\n")
    print(json.dumps({"before": before, "after": len(data["rows"]), "removed_pools": len(pools) if args.pools else None}))


if __name__ == "__main__":
    main()
