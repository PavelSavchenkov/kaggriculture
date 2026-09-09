"""Project complete evidence while retaining physical aliases and novelty rules."""
import argparse
import copy
import hashlib
import json
from collections import defaultdict
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    for name in ["dataset", "index", "novelty", "output"]: parser.add_argument(name, type=Path)
    parser.add_argument("--novel", action="store_true")
    parser.add_argument("--whole-pools", action="store_true")
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text()); assert not data["pending_cases"]
    lookup = {r["physical_key"]: r for r in data["rows"]}
    members = [json.loads(line) for line in args.index.read_text().splitlines()]
    audit = [r for r in map(json.loads, args.novelty.read_text().splitlines()) if r["panel"] == args.index.parent.name]
    assert audit and len(audit) == len(members)
    valid = {(r["physical_key"], r["pool"], r["variant"], r["family"]): r["novel"] for r in audit}
    removed_pools = {r["pool"] for r in audit if not r["novel"]} if args.novel and args.whole_pools else set()
    selected = [r for r in members if not args.novel or
                (valid[r["physical_key"], r.get("pool"), r.get("variant"), r["source_family"]] and r.get("pool") not in removed_pools)]
    grouped = defaultdict(list)
    for row in selected: grouped[row["physical_key"]].append(row)
    rows = []
    for key, aliases in grouped.items():
        row = copy.deepcopy(lookup[key])
        pools, variants = {r.get("pool") for r in aliases}, {r.get("variant") for r in aliases}
        row.update(pool=next(iter(pools)) if len(pools) == 1 else None,
                   variant=sorted(variants, key=lambda v: str(v))[0],
                   reference_families=row["families"], families=sorted({r["source_family"] for r in aliases}),
                   sources=sorted({s for r in aliases for s in r["sources"]}),
                   panel_memberships=[{"pool": r.get("pool"), "variant": r.get("variant"), "family": r["source_family"]} for r in aliases])
        rows.append(row)
    args.output.mkdir(exist_ok=False)
    data["rows"] = rows
    data["panel_projection"] = {"novel": args.novel, "whole_pools": args.whole_pools,
                                "input_memberships": len(members), "kept_memberships": len(selected),
                                "removed_pools": sorted(p for p in removed_pools if p is not None),
                                "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.index, args.novelty, Path(__file__)]}}
    (args.output / "DATASET.json").write_text(json.dumps(data, indent=2) + "\n")
    (args.output / "INDEX.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in selected))
    print(json.dumps({"physical": len(rows), "memberships": len(selected), "novel": args.novel}))


if __name__ == "__main__":
    main()
