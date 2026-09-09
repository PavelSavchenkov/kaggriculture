"""Attach panel membership to one shared, complete physical reference dataset."""
import argparse
import copy
import hashlib
import json
from collections import defaultdict
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("index", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text())
    assert not data["pending_cases"]
    lookup = {r["physical_key"]: r for r in data["rows"]}
    groups = defaultdict(list)
    for line in args.index.read_text().splitlines():
        row = json.loads(line); groups[row["physical_key"]].append(row)
    rows = []
    for key, members in groups.items():
        assert len({(r.get("pool"), r.get("variant")) for r in members}) == 1, "one physical contract belongs to multiple panel positions"
        row = copy.deepcopy(lookup[key])
        row.update(pool=members[0].get("pool"), variant=members[0].get("variant"),
                   reference_families=row["families"], families=sorted({r["source_family"] for r in members}),
                   sources=sorted({s for r in members for s in r["sources"]}))
        rows.append(row)
    data["rows"] = rows
    data["panel_projection"] = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.index]}
    args.output.parent.mkdir(exist_ok=False)
    args.output.write_text(json.dumps(data, indent=2) + "\n")
    print(f"Projected {len(rows)} physical contracts with unchanged reference evidence.")


if __name__ == "__main__":
    main()
