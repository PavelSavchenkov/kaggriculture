"""Generate deterministic, state-aware differential traces from the official engine."""
import contextlib
from collections import Counter
import io
from pathlib import Path
import random
import sys

from kaggle_environments import make
from kaggle_environments.envs.kaggriculture import kaggriculture as engine

from export_trace import (ENGINE_SOURCE_HASH, ENGINE_VERSION, ITEMS, canonical_values,
                          enc_order, enc_unit, parity_hash)


PRODUCTS = ITEMS[:9]
CROPS = ITEMS[:5]
ANIMALS = ITEMS[9:]
UNIT_OPS = ["PASS", "NORTH", "SOUTH", "EAST", "WEST", "PICKUP", "DROP", "PLACE",
            "PLANT", "WATER", "HARVEST", "FERTILIZE", "DIG", "BUILD_COOP",
            "BUILD_PASTURE", "FEED", "COLLECT_FERTILIZER", "CARE"]
QUANTITIES = [-2147483648, -10, -1, 0, 1, 2, 7, 100, 40000, 2147483647]
CONFIG_KEYS = ["episodeSteps", "boardSize", "startingMoney", "maxMarketOrdersPerTurn",
               "turnsPerDay", "shedCapacity", "weedSpawnChance", "townShopUnlockInterval",
               "townShopSellInterval", "townCenterSellInterval", "farmHandCostMult"]


def at_shed(position, board_size=10):
    half = board_size // 2
    return position[0] in (half - 1, half) and position[1] in (half - 1, half)


def arbitrary_unit(rng):
    op = rng.choice([*UNIT_OPS, "UNKNOWN"])
    item = rng.choice([*ITEMS, "UNKNOWN"])
    if op in {"PICKUP", "PLACE"}:
        return [op, item, rng.choice(QUANTITIES)]
    if op == "PLANT":
        return [op, item]
    return [op]


def unit_action(rng, farm, private, index):
    positions = [farm["farmer"], *farm["hands"]]
    if index >= len(positions) or rng.random() < 0.35:
        return arbitrary_unit(rng)

    position = positions[index]
    x, y = position
    tile = farm["tiles"][y][x]
    inventory = private["inventories"][index]
    candidates = [["PASS"], ["NORTH"], ["SOUTH"], ["EAST"], ["WEST"]]
    if at_shed(position):
        candidates.extend([["PICKUP", item, rng.choice([1, 2, 100, 40000])]
                           for item, count in private["shed"].items() if count > 0])
    if inventory:
        candidates.append(["DROP"])
        candidates.extend([["PLACE", item, rng.choice([1, 2, 100, 40000])]
                           for item, count in inventory.items() if count > 0])
    if tile is None:
        candidates.extend([["PLANT", crop] for crop, count in private["seeds"].items()
                           if count > 0])
        candidates.extend([["BUILD_COOP"], ["BUILD_PASTURE"]])
    if isinstance(tile, dict):
        if tile.get("kind") == "PLANT":
            candidates.extend([["WATER"], ["HARVEST"], ["FERTILIZE"], ["DIG"]])
        elif tile.get("kind") == "WEED":
            candidates.append(["DIG"])
        elif "animal" in tile:
            candidates.extend([["FEED"], ["CARE"], ["COLLECT_FERTILIZER"], ["HARVEST"]])
        else:
            candidates.append(["DIG"])
    return rng.choice(candidates)


def arbitrary_order(rng):
    op = rng.choice(["HIRE", "BUY_LAND", "BUY_SEED", "BUY_PRODUCT", "BUY_ANIMAL",
                     "SELL", "UNKNOWN"])
    if op in {"HIRE", "BUY_LAND"}:
        return [op]
    return [op, rng.choice([*ITEMS, "UNKNOWN"]), rng.choice(QUANTITIES)]


def market_order(rng, farm, private):
    if rng.random() < 0.35:
        return arbitrary_order(rng)
    candidates = [
        ["BUY_SEED", rng.choice(CROPS), rng.randint(1, 5)],
        ["BUY_PRODUCT", rng.choice(["WHEAT", "FERTILIZER"]), rng.randint(1, 5)],
        ["BUY_ANIMAL", rng.choice(ANIMALS), rng.randint(1, 2)],
    ]
    candidates.extend([["SELL", item, rng.randint(1, 8)]
                       for item in PRODUCTS if private["shed"][item] > 0])
    if len(farm["hands"]) < 15:
        candidates.append(["HIRE"])
    if len(farm["unlocked_quadrants"]) < 4:
        candidates.append(["BUY_LAND"])
    return rng.choice(candidates)


def make_action(rng, state, player, max_orders):
    obs = state[0].observation
    farm = obs.farms[player]
    private = state[player].observation.private
    actual_units = 1 + len(farm["hands"])
    submitted_units = actual_units + (rng.randrange(3) if rng.random() < 0.2 else 0)
    units = [unit_action(rng, farm, private, index) for index in range(submitted_units)]
    n_orders = rng.randrange(max_orders + 1)
    orders = [market_order(rng, farm, private) for _ in range(n_orders)]
    return {"farmer": units[0], "hands": units[1:], "market": orders}


def config_values(config):
    return [config[key] for key in CONFIG_KEYS]


def export_case(case_id, seed, config, out_dir, coverage):
    rng = random.Random((seed << 17) ^ 0xD1FF3A)
    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        env = make("kaggriculture", configuration=dict(config, seed=seed), debug=False)
    state = env.state
    turns = config["episodeSteps"] - 1
    action_lines = []
    truth_lines = []

    def record(frame_index):
        obs = state[0].observation
        money = [int(obs.farms[player]["money"]) for player in range(2)]
        inventory = [int(value) for value in obs.market["inventory"].values()]
        truth_lines.append(" ".join(map(str, [*money, *inventory,
                                                parity_hash(canonical_values(frame_index, state))])))
        coverage["shops"] = max(coverage["shops"], len(obs.town["unlocked_shops"]))
        for farm in obs.farms:
            coverage["units"] = max(coverage["units"], 1 + len(farm["hands"]))
            for row in farm["tiles"]:
                for tile in row:
                    if tile is None:
                        coverage["tiles"]["EMPTY"] += 1
                    elif tile == "LOCKED":
                        coverage["tiles"]["LOCKED"] += 1
                    else:
                        coverage["tiles"][tile["kind"]] += 1
                        if "crop" in tile:
                            coverage["crops"][tile["crop"]] += 1
                        if "animal" in tile:
                            coverage["animals"][tile["animal"]] += 1

    record(0)
    for step in range(turns):
        actions = [make_action(rng, state, player, config["maxMarketOrdersPerTurn"])
                   for player in range(2)]
        for action in actions:
            units = [action["farmer"], *action["hands"]]
            orders = action["market"]
            parts = [len(units), len(orders)]
            for unit in units:
                encoded = enc_unit(unit)
                coverage["unit_ops"][unit[0]] += 1
                parts.extend(encoded)
            for order in orders:
                encoded = enc_order(order)
                coverage["market_ops"][order[0]] += 1
                parts.extend(encoded)
            action_lines.append(" ".join(map(str, parts)))
        for player in range(2):
            state[player].action = actions[player]
            state[player].observation.step = step
        state = engine.interpreter(state, env)
        record(step + 1)

    lines = [f"{seed} {turns}", "CONFIG " + " ".join(map(str, config_values(config))),
             f"ENGINE {ENGINE_VERSION} {ENGINE_SOURCE_HASH}", *action_lines, "TRUTH", *truth_lines]
    path = Path(out_dir) / f"fuzz_{case_id:03d}_{seed}.txt"
    path.write_text("\n".join(lines) + "\n")
    return path


def cases():
    default = {
        "episodeSteps": 720, "boardSize": 10, "startingMoney": 3000,
        "maxMarketOrdersPerTurn": 10, "turnsPerDay": 24, "shedCapacity": 100,
        "weedSpawnChance": 0.005, "townShopUnlockInterval": 3,
        "townShopSellInterval": 4, "townCenterSellInterval": 24,
        "farmHandCostMult": 1,
    }
    result = [(10000 + index, dict(default)) for index in range(24)]
    for index in range(24):
        config = dict(default)
        config.update({
            "episodeSteps": [49, 97, 241, 720][index % 4],
            "startingMoney": [0, 3000, 12000][index % 3],
            "maxMarketOrdersPerTurn": [1, 4, 10, 16][index % 4],
            "turnsPerDay": [1, 7, 13, 24][index % 4],
            "shedCapacity": [1, 7, 100, 511][index % 4],
            "weedSpawnChance": [0.0, 0.005, 0.1, 1.0][index % 4],
            "townShopUnlockInterval": [1, 2, 3, 7][index % 4],
            "townShopSellInterval": [1, 2, 4, 11][index % 4],
            "townCenterSellInterval": [1, 5, 12, 24][index % 4],
            "farmHandCostMult": [0, 1, 3][index % 3],
        })
        result.append((20000 + index, config))
    return result


def main():
    out_dir = Path(sys.argv[1] if len(sys.argv) > 1 else "traces/fuzz")
    out_dir.mkdir(parents=True, exist_ok=True)
    coverage = {"unit_ops": Counter(), "market_ops": Counter(), "tiles": Counter(),
                "crops": Counter(), "animals": Counter(), "units": 0, "shops": 0}
    paths = [export_case(index, seed, config, out_dir, coverage)
             for index, (seed, config) in enumerate(cases())]
    print(f"wrote {len(paths)} traces to {out_dir}")
    for key in ("unit_ops", "market_ops", "tiles", "crops", "animals"):
        print(f"{key}:", " ".join(f"{name}={count}" for name, count in coverage[key].most_common()))
    print(f"max units={coverage['units']} shops={coverage['shops']}")


if __name__ == "__main__":
    main()
