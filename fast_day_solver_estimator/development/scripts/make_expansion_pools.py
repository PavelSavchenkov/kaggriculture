"""Construct new-asset obligations on unchanged real dawn farms."""
import argparse
import csv
import hashlib
import json
import subprocess
from collections import defaultdict
from pathlib import Path

from check_conservation import check
from import_problems import physical_key


EXP = Path(__file__).resolve().parents[1]


def digest(value):
    return hashlib.sha256(value.encode()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("index", type=Path)
    parser.add_argument("name")
    parser.add_argument("--owned", action="store_true")
    parser.add_argument("--scope", default="Exposed development expansion proposals")
    args = parser.parse_args()
    output = EXP / "data" / args.name
    output.mkdir(exist_ok=False)
    grouped = defaultdict(list)
    for row in map(json.loads, args.index.read_text().splitlines()):
        grouped[row["physical_key"]].append(row)
    families = defaultdict(list)
    for key, rows in grouped.items():
        if len({r["source_family"] for r in rows}) != 1 or rows[0]["tasks"] < (1 if args.owned else 30):
            continue
        p = json.loads((EXP / rows[0]["problem"]).read_text())
        worked = {r["tile"] for r in p["tile_work"] if r["actions"]}
        owned = sum(i not in worked and t["state"]["kind"] in ["empty", "weed"] for i, t in enumerate(p["start"]["managed_tiles"]))
        locked = sum(i not in worked and t["state"]["kind"] == "locked" for i, t in enumerate(p["start"]["managed_tiles"]))
        if (owned >= 1 if args.owned else owned >= 4 or locked >= 10):
            families[rows[0]["source_family"]].append(rows[0])
    parents = []
    for family, rows in sorted(families.items()):
        chosen = []
        for lower, upper in [(1 if args.owned else 30, 70), (70, 120), (120, 10000)]:
            candidates = sorted((r for r in rows if lower <= r["tasks"] < upper), key=lambda r: digest("expansion_v0:" + r["physical_key"]))
            chosen.extend(candidates[:2])
        used = {r["physical_key"] for r in chosen}
        remaining = sorted((r for r in rows if r["physical_key"] not in used), key=lambda r: digest("expansion_fill_v0:" + r["physical_key"]))
        chosen.extend(remaining[:6 - len(chosen)])
        parents.extend(chosen)
    specifications, lines = [], []
    for parent in parents:
        pool = parent["physical_key"][:20]
        seed = int(digest("expansion_geometry_v0:" + pool)[:8], 16)
        crop, animal = seed % 5, 9 + seed % 3
        variants = [("original", 0, crop, 0, 0, -1), ("crop4_near", 4, crop, 0, 0, -1),
                    ("crop4_far", 4, crop, 1, 0, -1), ("crop4_far_late", 4, crop, 1, 12, -1),
                    ("crop8_far", 8, crop, 1, 0, -1), ("animal4_far", 4, animal, 1, 0, -1),
                    ("land_crop8", 8, crop, 1, 0, 0), ("land_crop9", 9, crop, 1, 0, 0),
                    ("land_crop10", 10, crop, 1, 0, 0), ("land_animal6_late", 6, animal, 1, 12, 12)]
        if args.owned:
            variants = [("original", 0, crop, 0, 0, -1), ("crop1_near", 1, crop, 0, 0, -1),
                        ("crop1_far", 1, crop, 1, 0, -1), ("crop2_far", 2, crop, 1, 0, -1),
                        ("crop2_far_late", 2, crop, 1, 12, -1), ("crop4_far", 4, crop, 1, 0, -1),
                        ("animal1_far", 1, animal, 1, 0, -1), ("animal2_far", 2, animal, 1, 0, -1)]
        for variant, count, item, geometry, release, land in variants:
            name = pool + "_" + variant
            spec = {"id": name, "pool": pool, "variant": variant, "parent": parent, "count": count, "item": item,
                    "geometry": geometry, "input_release": release, "land_release": land, "seed": seed}
            specifications.append(spec)
            lines.append(f"{name} {EXP / parent['problem']} {count} {item} {geometry} {release} {land} {seed}\n")
    manifest = output / "GENERATOR.txt"; manifest.write_text("".join(lines))
    protocol = {"scope": args.scope + ". Each keeps the original dawn state and original work/output/withdrawals, adding new crop or animal assets and explicit purchases.",
        "selection": "Up to six single-family parents, with two hash-selected examples per task band, filling missing bands by a separate hash order. No solver labels or model predictions select the parents.",
        "task_bands": ["1-69" if args.owned else "30-69", "70-119", "120+"], "owned_additions_only": args.owned,
        "endpoint": "Each new asset's local endpoint is independently simulated through night. Local workers and carried stock are only endpoint-oracle devices; no global workforce or routing witness is inherited.",
        "market": "Use the specified purchase hour and first free non-hire slots; preserve all original non-hire purchases. Record unavailable space, land or slots explicitly. Added purchases exactly cover added input consumption, so old stock endpoints and withdrawals stay fixed.",
        "value": "New assets have future value and purchase/land cost outside today's labor target. Evaluate signed marginal labor, never rank these as equal-value alternatives.",
        "horizon": 24, "source_sha256": hashlib.sha256(args.index.read_bytes()).hexdigest(), "specifications": specifications,
        "binary_sha256": hashlib.sha256((EXP / "build/generate_expansions").read_bytes()).hexdigest(),
        "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                    str(EXP / "build/generate_expansions"), str(manifest), str(output / "contracts")], check=True)
    status = {r["id"]: r["status"] for r in csv.DictReader((output / "contracts/STATUS.csv").open())}
    rows, excluded = [], []
    for spec in specifications:
        if status[spec["id"]] != "OK":
            excluded.append({"id": spec["id"], "reason": status[spec["id"]]}); continue
        path = output / "contracts" / (spec["id"] + ".json")
        p = json.loads(path.read_text()); parent = spec["parent"]
        original = json.loads((EXP / parent["problem"]).read_text())
        check(p)
        assert p["start"] == original["start"]
        assert p["end_shed"] == original["end_shed"] and p["end_seeds"] == original["end_seeds"]
        assert p["shed_availability"] == original["shed_availability"]
        old_work = {r["tile"]: r["actions"] for r in original["tile_work"]}
        new_work = {r["tile"]: r["actions"] for r in p["tile_work"]}
        assert all(new_work[tile] == actions for tile, actions in old_work.items())
        key = physical_key(p)
        if spec["variant"] == "original":
            assert key == parent["physical_key"]
        source = "expansion/" + parent["source_family"] + "/" + spec["id"]
        rows.append({"source": source, "sources": [source], "source_family": parent["source_family"],
            "pool": spec["pool"], "variant": spec["variant"], "problem": str(path.relative_to(EXP)),
            "source_sha256": hashlib.sha256(path.read_bytes()).hexdigest(), "physical_key": key,
            "observed_workers": p["worker_count"], "worker_count_provenance": "Original answer metadata only for identity; changed plans use one unscheduled placeholder worker. Reference range must be full40.",
            "tasks": sum(len(w["actions"]) for w in p["tile_work"]), "active_tiles": len(p["tile_work"]), "witnesses": [],
            "added_assets": spec["count"], "added_item": spec["item"], "input_release": spec["input_release"], "land_release": spec["land_release"],
            "added_orders": [e for e in p["buy_schedule"] if e["op"] != "hire" and e not in original["buy_schedule"]]})
    (output / "index.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in rows))
    (output / "EXCLUDED.json").write_text(json.dumps(excluded, indent=2) + "\n")
    print(json.dumps({"parents": len(parents), "contracts": len(rows), "excluded": len(excluded), "families": len(families)}))


if __name__ == "__main__":
    main()
