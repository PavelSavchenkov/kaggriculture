"""Freeze normalized development reference queries before their solve outcomes."""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

from import_problems import physical_key


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--limit", type=int, default=400)
    parser.add_argument("--name", default="reference_v0")
    parser.add_argument("--upper-padding", type=int, default=2)
    parser.add_argument("--index", type=Path, default=EXP / "runs/inventory_v0/problems.jsonl")
    parser.add_argument("--active-hours", type=int, choices=[23, 24], default=24)
    parser.add_argument("--upper-policy", choices=["source_padding", "full40"], default="source_padding")
    args = parser.parse_args()
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    (output / "problems").mkdir()
    source = args.index
    groups = defaultdict(list)
    for row in map(json.loads, source.read_text().splitlines()):
        # Final-day contracts need a separate 23-action boundary; do not mix them
        # into ordinary dawn-to-dawn labels.
        if args.active_hours == 24 and any("/days/29/" in s or "_d29" in s for s in row["sources"]):
            continue
        if row.get("active_hours", 24) != args.active_hours:
            raise ValueError("explicit horizon does not match the input corpus")
        groups[row["physical_key"]].append(row)
    selected = sorted(groups, key=lambda key: hashlib.sha256(("reference_v0:" + key).encode()).hexdigest())[:args.limit]
    cases = []
    for key in selected:
        rows = groups[key]
        chosen = min(rows, key=lambda row: row["observed_workers"])
        p = json.loads((EXP / chosen["problem"]).read_text())
        p["buy_schedule"] = [e for e in p["buy_schedule"] if e["op"] != "hire"]
        occupied = {(e["hour"], e["order_index"]) for e in p["buy_schedule"]}
        menu = [(h, slot) for h in range(args.active_hours - 1) for slot in range(10) if (h, slot) not in occupied][:39]
        if len(menu) != 39:
            raise ValueError("fewer than39 optional hire slots; unsupported initial hiring rule")
        # The entire 39-slot menu comes only from fixed non-hire purchases,
        # never from the source answer workforce.
        upper = 40 if args.upper_policy == "full40" else min(40, max(row["observed_workers"] for row in rows) + args.upper_padding)
        tasks = sum(len(work["actions"]) for work in p["tile_work"])
        capacity, lower = args.active_hours, 1
        while capacity < tasks and lower < 40:
            capacity += args.active_hours - 1 - menu[lower - 1][0]
            lower += 1
        paths = []
        for workers in range(lower, upper + 1):
            q = json.loads(json.dumps(p))
            q["worker_count"] = workers
            q["buy_schedule"] += [{"hour": h, "order_index": s, "op": "hire", "item": -1, "quantity": 1}
                                   for h, s in menu[:workers - 1]]
            q["buy_schedule"].sort(key=lambda e: (e["hour"], e["order_index"]))
            path = output / "problems" / f"{key[:20]}_w{workers:02}.json"
            path.write_text(json.dumps(q, sort_keys=True, separators=(",", ":")) + "\n")
            if physical_key(q, args.active_hours) != key:
                raise ValueError("normalization changed the physical contract")
            paths.append({"workers": workers, "path": str(path.relative_to(EXP)),
                          "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
        cases.append({"id": key[:20], "physical_key": key, "queries": paths, "active_hours": args.active_hours,
                      "task_only_lower_bound": lower, "source_max_workers_unverified_under_new_rule": max(row["observed_workers"] for row in rows),
                      "source_families": sorted({r.get("source_family", "historical") for r in rows}),
                      "sources": sorted({s for row in rows for s in row["sources"]}),
                      "hiring_rule": "earliest_prefix_39_nonpurchase_slots_v0"})
    (output / "CASES.json").write_text(json.dumps(cases, indent=2) + "\n")
    protocol = {"purpose": "Offline reference evidence; inference cannot access case metadata or solve output.",
                "case_selection": "SHA256 order of canonical physical contracts, before query outcomes; terminal days excluded.",
                "cases": len(cases), "queries": sum(len(c["queries"]) for c in cases),
                "upper_padding": args.upper_padding,
                "upper_policy": args.upper_policy,
                "query_range": "Task-only capacity lower bound through 40; independent of source workforce." if args.upper_policy == "full40" else "Task-only capacity lower bound through source workforce plus padding; reference evidence only.",
                "active_hours": args.active_hours,
                "initial_query_seconds": 3, "solver": "unchanged root V30", "processes": 4,
                "order": "decreasing workforce per case; retain every solved schedule",
                "failure_semantics": "UNKNOWN at this budget, never an infeasibility label",
                "caveat": "A normalized hiring calendar may invalidate a source schedule; source count is metadata only.",
                "manifest_sha256": hashlib.sha256((output / "CASES.json").read_bytes()).hexdigest()}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    print(json.dumps({k: v for k, v in protocol.items() if k in ["cases", "queries", "processes"]}))


if __name__ == "__main__":
    main()
