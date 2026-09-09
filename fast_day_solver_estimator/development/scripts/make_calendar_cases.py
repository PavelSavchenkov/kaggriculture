"""Freeze explicit hiring-calendar variants without source workforce labels."""
import argparse
import hashlib
import json
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

from import_problems import physical_key


EXP = Path(__file__).resolve().parents[1]


def digest(value):
    return hashlib.sha256(value.encode()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("--indices", type=Path, nargs="+", required=True)
    parser.add_argument("--per-family", type=int, default=1)
    parser.add_argument("--scope", default="Development calendar transfer, not an untouched-family test.")
    args = parser.parse_args()
    output = EXP / "data" / args.name
    output.mkdir(exist_ok=False)
    (output / "problems").mkdir()
    parents, input_hashes, seen = [], {}, set()
    for index in args.indices:
        input_hashes[str(index)] = hashlib.sha256(index.read_bytes()).hexdigest()
        grouped, families = defaultdict(list), defaultdict(list)
        for row in map(json.loads, index.read_text().splitlines()):
            grouped[row["physical_key"]].append(row)
        for key, rows in grouped.items():
            if key in seen or len({r["source_family"] for r in rows}) != 1 or rows[0]["tasks"] == 0:
                continue
            families[rows[0]["source_family"]].append(rows[0])
        for family, rows in sorted(families.items()):
            selected = sorted(rows, key=lambda r: digest("calendar_v1:" + r["physical_key"]))[:args.per_family]
            parents.extend(selected); seen.update(r["physical_key"] for r in selected)
    specs = [("earliest", 0, 10), ("delay3", 3, 10), ("delay8", 8, 10), ("late17", 17, 10), ("paced2", 0, 2)]
    protocol = {"utc": datetime.now(timezone.utc).isoformat(), "scope": args.scope,
                "identity": "An obligation key excludes source workers/hires. A contract key additionally includes active horizon and the complete ordered optional hire menu. Never merge labels across different menus.",
                "selection": "Per input panel and single source family, select physical contracts by a fixed SHA256 ordering, before reference outcomes. No source crew cap.",
                "parents": len(parents), "per_family_per_panel": args.per_family, "profiles": specs,
                "query_range": "Task-only capacity lower bound through all optional workers, at most 40. Zero queries if even the full menu lacks raw action capacity.",
                "failure_semantics": "UNKNOWN is a budget miss. No certificate is a censored cost target, not an infeasibility label.",
                "source_sha256": input_hashes, "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    indices, cases, feature_lines = [], defaultdict(list), defaultdict(list)
    for parent in parents:
        p = json.loads((EXP / parent["problem"]).read_text())
        hours = parent.get("active_hours", 24)
        key = physical_key(p, hours)
        assert key == parent["physical_key"]
        p["worker_count"] = 1
        p["buy_schedule"] = [e for e in p["buy_schedule"] if e["op"] != "hire"]
        occupied = {(e["hour"], e["order_index"]) for e in p["buy_schedule"]}
        source = output / "problems" / (key[:20] + ".json")
        source.write_text(json.dumps(p, sort_keys=True, separators=(",", ":")) + "\n")
        for name, first, per_hour in specs:
            menu = [(h, slot) for h in range(first, hours - 1)
                    for slot in [s for s in range(10) if (h, s) not in occupied][:per_hour]][:39]
            identity = {"obligation_key": key, "active_hours": hours, "hire_slots": menu}
            contract = digest(json.dumps(identity, sort_keys=True, separators=(",", ":")))
            row = {"id": contract[:20], "contract_key": contract, **identity, "profile": name,
                   "problem": str(source.relative_to(EXP)), "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
                   "source_family": parent["source_family"], "parent_panel": str(parent["problem"]).split("/")[1],
                   "tasks": parent["tasks"], "source_provenance": parent["sources"]}
            indices.append(row)
            fields = " ".join(f"{h} {slot}" for h, slot in menu)
            feature_lines[hours].append(f"{row['id']} {source} {hours} {len(menu)} {fields}\n")
            capacity, lower = hours, 1
            while capacity < parent["tasks"] and lower <= len(menu):
                capacity += hours - 1 - menu[lower - 1][0]; lower += 1
            if capacity < parent["tasks"]: lower = len(menu) + 2
            reference = EXP / "runs" / (args.name.replace("development", "reference") + f"_h{hours}")
            queries = []
            for workers in range(lower, len(menu) + 2):
                q = json.loads(json.dumps(p)); q["worker_count"] = workers
                q["buy_schedule"] += [{"hour": h, "order_index": slot, "op": "hire", "item": -1, "quantity": 1} for h, slot in menu[:workers - 1]]
                q["buy_schedule"].sort(key=lambda e: (e["hour"], e["order_index"]))
                assert physical_key(q, hours) == key
                path = output / "problems" / f"{row['id']}_w{workers:02}.json"
                path.write_text(json.dumps(q, sort_keys=True, separators=(",", ":")) + "\n")
                queries.append({"workers": workers, "path": str(path.relative_to(EXP)), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
            cases[hours].append({**row, "queries": queries, "source_families": [parent["source_family"]],
                                 "task_only_lower_bound": lower, "hiring_rule": "explicit_optional_prefix_v1"})
    (output / "index.jsonl").write_text("".join(json.dumps(row, sort_keys=True) + "\n" for row in indices))
    summary = {"parents": len(parents), "calendar_contracts": len(indices), "horizons": {}}
    for hours, selected in cases.items():
        root = EXP / "runs" / (args.name.replace("development", "reference") + f"_h{hours}")
        root.mkdir(exist_ok=False)
        (root / "CASES.json").write_text(json.dumps(selected, indent=2) + "\n")
        (root / "PROTOCOL.json").write_text(json.dumps({**protocol, "active_hours": hours}, indent=2) + "\n")
        (output / f"FEATURES_{hours}.txt").write_text("".join(feature_lines[hours]))
        summary["horizons"][hours] = {"contracts": len(selected), "queries": sum(len(c["queries"]) for c in selected),
                                      "raw_capacity_exceeds_menu": sum(not c["queries"] for c in selected)}
    (output / "SUMMARY.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
