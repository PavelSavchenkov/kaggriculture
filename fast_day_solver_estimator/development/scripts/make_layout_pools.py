"""Create same-work layout alternatives, grouped by whole source family."""
import argparse
import hashlib
import json
import random
from collections import defaultdict
from pathlib import Path

from import_problems import physical_key


EXP = Path(__file__).resolve().parents[1]


def digest(value):
    return hashlib.sha256(value.encode()).hexdigest()


def quadrant(tile):
    return (tile["x"] >= 5) + 2 * (tile["y"] >= 5)


def distance(cell):
    x, y = cell
    return min(abs(x - 4), abs(x - 5)) + min(abs(y - 4), abs(y - 5))


def transform(problem, variant, seed):
    result = json.loads(json.dumps(problem))
    if variant == "original":
        return result
    rng = random.Random(seed)
    tiles = result["start"]["managed_tiles"]
    workload = {w["tile"]: len(w["actions"]) + sum(a["output_quantity"] for a in w["actions"]) / 12
                for w in result["tile_work"]}
    for q in range(4):
        ids = [i for i, tile in enumerate(tiles) if quadrant(tile) == q]
        # All coordinates remain within their original ownership quadrant.
        # The work chain and required endpoint move with the tile state.
        cells = [(tiles[i]["x"], tiles[i]["y"]) for i in ids]
        ids.sort(key=lambda i: (-workload.get(i, 0), i))
        if variant == "near":
            cells.sort(key=lambda c: (distance(c), c))
        elif variant == "far":
            cells.sort(key=lambda c: (-distance(c), c))
        elif variant.startswith("shuffle"):
            rng.shuffle(cells)
        else:
            raise ValueError(variant)
        for i, (x, y) in zip(ids, cells):
            tiles[i]["x"], tiles[i]["y"] = x, y
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("index", type=Path)
    parser.add_argument("name")
    parser.add_argument("--parents-per-family", type=int, default=6)
    args = parser.parse_args()
    output = EXP / "data" / args.name
    output.mkdir(exist_ok=False)
    by_key = defaultdict(list)
    for row in map(json.loads, args.index.read_text().splitlines()):
        by_key[row["physical_key"]].append(row)
    by_family = defaultdict(list)
    for key, rows in by_key.items():
        families = {row["source_family"] for row in rows}
        if len(families) == 1 and rows[0]["tasks"] >= 10 and rows[0]["active_tiles"] >= 4:
            by_family[rows[0]["source_family"]].append(rows[0])
    selected = [row for family in sorted(by_family)
                for row in sorted(by_family[family], key=lambda r: digest("layout_pool_v0:" + r["physical_key"]))[:args.parents_per_family]]
    index, pools = [], []
    for parent in selected:
        p = json.loads((EXP / parent["problem"]).read_text())
        pool = {"id": parent["physical_key"][:20], "family": parent["source_family"], "parent": parent["problem"],
                "parent_sha256": parent["source_sha256"], "alternatives": []}
        for variant in ["original", "near", "far", "shuffle_a", "shuffle_b"]:
            case = transform(p, variant, digest(pool["id"] + variant))
            body = json.dumps(case, sort_keys=True, separators=(",", ":")) + "\n"
            sha = digest(body)
            folder = output / pool["id"] / variant
            folder.mkdir(parents=True)
            path = folder / "problem.json"
            path.write_text(body)
            key = physical_key(case)
            source = f"layout/{pool['family']}/{pool['id']}/{variant}"
            index.append({"source": source, "sources": [source], "source_family": pool["family"],
                          "pool": pool["id"], "variant": variant, "problem": str(path.relative_to(EXP)),
                          "source_sha256": sha, "physical_key": key, "observed_workers": p["worker_count"],
                          "worker_count_provenance": "parent witness only; not a certificate for the changed layout",
                          "tasks": parent["tasks"], "active_tiles": parent["active_tiles"], "witnesses": []})
            pool["alternatives"].append({"variant": variant, "physical_key": key, "problem_sha256": sha})
        pools.append(pool)
    (output / "index.jsonl").write_text("".join(json.dumps(row, sort_keys=True) + "\n" for row in index))
    protocol = {"selection": "Six eligible physical contracts per single development family by pre-outcome SHA256 order.",
                "purpose": "Choose among layouts with identical physical work and non-labor value. This is a layout-search proxy, not a whole-game score claim.",
                "split": "Keep every alternative with its parent family; exclude physical contracts shared across families.",
                "mutation": "Permute the existing coordinate set separately inside each quadrant. Preserve tile state, chain, endpoints, purchases and withdrawals.",
                "unknowns": "A changed layout can be infeasible. No source schedule is inherited. UNKNOWN remains censored.",
                "source_index_sha256": hashlib.sha256(args.index.read_bytes()).hexdigest(), "pools": pools}
    (output / "POOLS.json").write_text(json.dumps(protocol, indent=2) + "\n")
    print(f"{len(pools)} parent pools, {len(index)} candidates, {len({r['physical_key'] for r in index})} physical contracts")


if __name__ == "__main__":
    main()
