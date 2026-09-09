"""Remove asset obligations and their goods, preserving all remaining chains."""
import argparse
import hashlib
import json
import random
from collections import defaultdict
from pathlib import Path

from import_problems import physical_key
from make_layout_pools import distance


EXP = Path(__file__).resolve().parents[1]


def digest(value):
    return hashlib.sha256(value.encode()).hexdigest()


def transform(problem, variant, seed):
    result = json.loads(json.dumps(problem))
    if variant == "original":
        return result, []
    rng = random.Random(seed)
    tiles = result["start"]["managed_tiles"]
    active = [r["tile"] for r in result["tile_work"] if r["actions"]]
    fraction, rule = variant.split("_")
    count = max(1, round(len(active) * int(fraction.removeprefix("remove")) / 100))
    if rule in ["near", "far"]:
        active.sort(key=lambda i: (distance((tiles[i]["x"], tiles[i]["y"])), i), reverse=rule == "far")
    elif rule == "shuffle":
        rng.shuffle(active)
    else:
        raise ValueError(variant)
    removed = set(active[:count])
    removed_output = [0] * len(result["end_shed"])
    restored_input = [0] * len(result["end_shed"])
    # Public JSON uses operation names; quantities are exact task outputs.
    for work in result["tile_work"]:
        if work["tile"] not in removed:
            continue
        for action in work["actions"]:
            if action["output_quantity"]:
                removed_output[action["output_item"]] += action["output_quantity"]
            op = action["op"]
            if op == "plant":
                result["end_seeds"][action["arg"]] += action["quantity"]
            elif op == "place":
                restored_input[action["arg"]] += action["quantity"]
            elif op == "feed":
                restored_input[0] += action["quantity"]
            elif op == "fertilize":
                restored_input[8] += action["quantity"]
    # Retain early withdrawals where possible. Remove missing production from
    # the terminal stock first, then latest withdrawals. Never create supply.
    for item, lost in enumerate(removed_output):
        remaining = result["end_shed"][item] + restored_input[item] - lost
        if remaining < 0:
            shortage = -remaining
            previous = 0
            increments = []
            for hour in range(24):
                total = result["shed_availability"][hour][item]
                increments.append(total - previous)
                previous = total
            for hour in reversed(range(24)):
                reduction = min(shortage, increments[hour])
                increments[hour] -= reduction
                shortage -= reduction
            if shortage:
                return None, sorted(removed)
            total = 0
            for hour, increment in enumerate(increments):
                total += increment
                result["shed_availability"][hour][item] = total
            remaining = 0
        result["end_shed"][item] = remaining
    keep = [i for i in range(len(tiles)) if i not in removed]
    remap = {old: new for new, old in enumerate(keep)}
    result["start"]["managed_tiles"] = [tiles[i] for i in keep]
    for field in ["end_tiles", "tile_work"]:
        result[field] = [row for row in result[field] if row["tile"] not in removed]
        for row in result[field]:
            row["tile"] = remap[row["tile"]]
    return result, sorted(removed)


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
    families = defaultdict(list)
    for key, rows in by_key.items():
        if len({r["source_family"] for r in rows}) == 1 and rows[0]["active_tiles"] >= 8 and rows[0]["tasks"] >= 15:
            families[rows[0]["source_family"]].append(rows[0])
    selected = [row for family in sorted(families) for row in sorted(families[family],
                key=lambda r: digest("work_pool_v0:" + r["physical_key"]))[:args.parents_per_family]]
    index, pools, excluded = [], [], []
    for parent in selected:
        problem = json.loads((EXP / parent["problem"]).read_text())
        pool = {"id": parent["physical_key"][:20], "family": parent["source_family"], "parent": parent["problem"], "alternatives": []}
        for variant in ["original", "remove10_near", "remove10_far", "remove25_shuffle", "remove50_shuffle"]:
            case, removed = transform(problem, variant, digest(pool["id"] + variant))
            if case is None:
                excluded.append({"pool": pool["id"], "variant": variant, "removed_tiles": removed,
                                 "reason": "remaining consumption exceeds all retained supply"})
                continue
            body = json.dumps(case, sort_keys=True, separators=(",", ":")) + "\n"
            sha = digest(body); key = physical_key(case)
            path = output / pool["id"] / variant / "problem.json"
            path.parent.mkdir(parents=True)
            path.write_text(body)
            source = f"work_subset/{pool['family']}/{pool['id']}/{variant}"
            index.append({"source": source, "sources": [source], "source_family": pool["family"], "pool": pool["id"],
                          "variant": variant, "problem": str(path.relative_to(EXP)), "source_sha256": sha, "physical_key": key,
                          "observed_workers": problem["worker_count"], "worker_count_provenance": "parent query ceiling only; not a changed-contract certificate",
                          "tasks": sum(len(r["actions"]) for r in case["tile_work"]), "active_tiles": len(case["tile_work"]), "witnesses": []})
            pool["alternatives"].append({"variant": variant, "physical_key": key, "removed_tiles": removed})
        pools.append(pool)
    (output / "index.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in index))
    protocol = {"selection": "Pre-outcome hash selection, six eligible source contracts per single family.",
                "scope": "Counterfactual asset/work subsets with different output value. Not legal instantaneous removal of an established farm.",
                "mutation": "Omit selected managed tiles and chains. Restore unused inputs and seeds to end stock; subtract lost output from end stock then latest withdrawals. Do not invent supply or inherit source certificates.",
                "metric": "Signed marginal labor error; later net-value ranking must account for lost output separately.",
                "families": "Every derivative stays with all parent lineage; exclude shared physical contracts.",
                "source_sha256": hashlib.sha256(args.index.read_bytes()).hexdigest(), "pools": pools, "excluded": excluded}
    (output / "POOLS.json").write_text(json.dumps(protocol, indent=2) + "\n")
    print(json.dumps({"pools": len(pools), "candidates": len(index), "excluded": len(excluded)}))


if __name__ == "__main__":
    main()
