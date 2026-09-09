"""Copy exposed day problems; record provenance without retaining source dependencies."""
import argparse
import hashlib
import json
from collections import Counter
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def physical_key(problem, active_hours=24):
    # Every tile reference indexes managed_tiles. Canonicalize coordinates first.
    value = json.loads(json.dumps(problem))
    if active_hours != 24:
        assert active_hours == 23
        value["active_hours"] = active_hours
    tiles = value["start"]["managed_tiles"]
    order = sorted(range(len(tiles)), key=lambda i: (tiles[i]["y"], tiles[i]["x"]))
    remap = {old: new for new, old in enumerate(order)}
    value["start"]["managed_tiles"] = [tiles[i] for i in order]
    for field in ["end_tiles", "tile_work"]:
        for row in value[field]:
            row["tile"] = remap[row["tile"]]
        value[field].sort(key=lambda row: row["tile"])
    value.pop("worker_count")
    value["buy_schedule"] = sorted(
        (row for row in value["buy_schedule"] if row["op"] != "hire"),
        key=lambda row: (row["hour"], row["order_index"]),
    )
    return digest(json.dumps(value, sort_keys=True, separators=(",", ":")).encode())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("--kind", choices=["archive", "root_solver"], required=True)
    args = parser.parse_args()
    source = args.source.resolve()
    output = EXP / "data/development" / args.kind
    output.mkdir(exist_ok=False)
    if args.kind == "archive":
        inventory = source / "final/ARTIFACT_INVENTORY.jsonl"
        entries = [json.loads(line) for line in inventory.read_text().splitlines()]
        paths = [source / row["path"] for row in entries if row["kind"] == "file"
                 and Path(row["path"]).name.startswith("problem")
                 and Path(row["path"]).suffix == ".json"
                 and "source_snapshot" not in row["path"]]
    else:
        paths = sorted((source / "benchmarks/cases").glob("*.json"))
    rows, skipped = [], []
    for path in sorted(paths):
        raw = path.read_bytes()
        problem = json.loads(raw)
        if not isinstance(problem, dict) or problem.get("format_version") != 3:
            skipped.append({"source": str(path.relative_to(source)), "reason": "not_public_v3"})
            continue
        sha = digest(raw)
        directory = output / sha[:20]
        directory.mkdir(exist_ok=True)
        target = directory / "problem.json"
        if not target.exists():
            target.write_bytes(raw)
        row = {"source": str(path.relative_to(source)), "source_sha256": sha,
               "problem": str(target.relative_to(EXP)), "physical_key": physical_key(problem),
               "observed_workers": problem["worker_count"],
               "tasks": sum(len(w["actions"]) for w in problem["tile_work"]),
               "active_tiles": len(problem["tile_work"]), "witnesses": []}
        candidates = []
        if path.name == "problem.json":
            candidates += [path.with_name("actions.txt"), path.with_name("original_actions.txt")]
            candidates += sorted(path.parent.glob("raw_schedule*.txt"))
        elif path.stem.startswith("problem_h"):
            suffix = path.stem.removeprefix("problem_")
            candidates += [path.with_name(f"raw_schedule_{suffix}.txt"), path.with_name(f"executable_{suffix}.txt")]
        for witness in candidates:
            if not witness.is_file():
                continue
            data = witness.read_bytes()
            witness_hash = digest(data)
            copied = directory / f"witness_{witness_hash[:20]}.txt"
            if not copied.exists():
                copied.write_bytes(data)
            row["witnesses"].append({"source": str(witness.relative_to(source)), "sha256": witness_hash,
                                     "path": str(copied.relative_to(EXP)), "verified": False})
        rows.append(row)
    (output / "index.jsonl").write_text("".join(json.dumps(row, sort_keys=True) + "\n" for row in rows))
    summary = {"source_kind": args.kind, "source_root_provenance_only": str(source),
               "input_files": len(paths), "records": len(rows),
               "distinct_problem_hashes": len({row["source_sha256"] for row in rows}),
               "distinct_physical_contracts": len({row["physical_key"] for row in rows}),
               "records_with_unverified_witness": sum(bool(row["witnesses"]) for row in rows),
               "task_counts": dict(sorted(Counter(row["tasks"] for row in rows).items())),
               "skipped": skipped}
    (output / "SUMMARY.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps({key: value for key, value in summary.items() if key not in ["task_counts", "skipped"]}))


if __name__ == "__main__":
    main()
