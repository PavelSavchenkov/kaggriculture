"""Audit a frozen unseen wave without using any reference outcomes."""
import argparse
import hashlib
import json
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def read_index(path):
    return [json.loads(line) for line in path.read_text().splitlines()]


def work_key(path):
    value = json.loads(path.read_text())
    value.pop("worker_count")
    value["buy_schedule"] = sorted((r for r in value["buy_schedule"] if r["op"] != "hire"),
                                   key=lambda r: (r["hour"], r["order_index"]))
    end = {r["tile"]: {k: v for k, v in r.items() if k != "tile"} for r in value.pop("end_tiles")}
    work = {r["tile"]: {k: v for k, v in r.items() if k != "tile"} for r in value.pop("tile_work")}
    local = [{"start": {k: v for k, v in tile.items() if k not in ["x", "y"]},
              "end": end.get(i), "work": work.get(i)}
             for i, tile in enumerate(value["start"].pop("managed_tiles"))]
    value["local_contracts"] = sorted(json.dumps(r, sort_keys=True) for r in local)
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("--wave", type=Path, nargs="+", required=True)
    parser.add_argument("--exposed", type=Path, nargs="+", required=True)
    parser.add_argument("--training", type=Path, required=True)
    parser.add_argument("--training-includes-unlabeled", action="store_true")
    args = parser.parse_args()
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    training = json.loads(args.training.read_text())
    trained = {r["physical_key"] for r in training["rows"]
               if (r["reference_workers"] is not None or args.training_includes_unlabeled) and len(r["families"]) == 1}
    exposed = defaultdict(set)
    pattern = defaultdict(set)
    cache = {}
    for index in args.exposed:
        for row in read_index(index):
            key = row["physical_key"]
            exposed[key].add(str(index))
            if key not in cache:
                cache[key] = work_key(EXP / row["problem"])
            pattern[cache[key]].add(key)
    assert trained <= exposed.keys(), "frozen training is missing from exposure inventory"
    results = []
    for index in args.wave:
        for row in read_index(index):
            key = row["physical_key"]
            if key not in cache:
                cache[key] = work_key(EXP / row["problem"])
            results.append({"index": str(index), "source": row["source"],
                            "family": row["source_family"], "physical_key": key,
                            "frozen_training_overlap": key in trained,
                            "exposed_overlap": key in exposed,
                            "exposed_indices": sorted(exposed.get(key, [])),
                            "same_work_ignoring_coordinates": cache[key] in pattern,
                            "same_work_key": cache[key]})
    summaries = []
    for index in args.wave:
        rows = [r for r in results if r["index"] == str(index)]
        unique = {r["physical_key"]: r for r in rows}.values()
        summaries.append({"index": str(index), "records": len(rows), "physical_contracts": len(unique),
                          "frozen_training_overlap": sum(r["frozen_training_overlap"] for r in unique),
                          "exposed_overlap": sum(r["exposed_overlap"] for r in unique),
                          "same_work_ignoring_coordinates": sum(r["same_work_ignoring_coordinates"] for r in unique),
                          "novel_contracts_by_family": dict(Counter(r["family"] for r in unique if not r["exposed_overlap"]))})
    report = {"utc": datetime.now(timezone.utc).isoformat(), "frozen_training_contracts": len(trained),
              "training_includes_unlabeled_compiler_outcomes": args.training_includes_unlabeled,
              "exposed_contracts": len(exposed), "summaries": summaries,
              "interpretation": "Exact physical overlap excludes a contract from the primary novel stratum. Coordinate-free matches are diagnostic. Team splitting does not establish independent authorship.",
              "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in args.wave + args.exposed + [args.training]}}
    (output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")
    (output / "RECORDS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in results))
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
