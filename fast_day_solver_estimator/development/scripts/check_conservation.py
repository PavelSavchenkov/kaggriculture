"""Independently check whole-day item and seed conservation in public inputs."""
import argparse
import json
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]
FERTILIZER = 8
WHEAT = 0


def check(problem):
    items = list(problem["start"]["shed"])
    seeds = list(problem["start"]["seeds"])
    for event in problem["buy_schedule"]:
        if event["op"] in ["buy_product", "buy_animal"]:
            items[event["item"]] += event["quantity"]
        elif event["op"] == "buy_seed":
            seeds[event["item"]] += event["quantity"]
        else:
            assert event["op"] in ["hire", "buy_land"], event["op"]
    for work in problem["tile_work"]:
        for action in work["actions"]:
            if action["output_quantity"]:
                items[action["output_item"]] += action["output_quantity"]
            op = action["op"]
            if op == "plant":
                seeds[action["arg"]] -= action["quantity"]
            elif op == "place":
                items[action["arg"]] -= action["quantity"]
            elif op == "feed":
                items[WHEAT] -= action["quantity"]
            elif op == "fertilize":
                items[FERTILIZER] -= action["quantity"]
    items = [n - w for n, w in zip(items, problem["shed_availability"][-1])]
    assert items == problem["end_shed"], (items, problem["end_shed"])
    assert seeds == problem["end_seeds"], (seeds, problem["end_seeds"])
    assert all(n >= 0 for n in items + seeds)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("index", type=Path)
    args = parser.parse_args()
    rows = [json.loads(line) for line in args.index.read_text().splitlines()]
    for row in rows:
        check(json.loads((EXP / row["problem"]).read_text()))
    print(f"Item and seed conservation passed on {len(rows)} contracts; this is not a scheduling-feasibility certificate.")


if __name__ == "__main__":
    main()
