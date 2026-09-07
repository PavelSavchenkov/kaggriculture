"""Export ground truth from the real Python environment, for validating the port.

Runs a full episode with the built-in agents, records the actions they ACTUALLY
emitted, and dumps them alongside the resulting money and market-inventory
trajectory. Feeding those exact actions to the C++ simulator must reproduce the
trajectory step for step — that is what `validate` checks.

Recording emitted actions rather than replaying an agent matters: the reference
agents are not pure replays (they repair around weeds and reorder SELL slots),
so re-running one is not guaranteed to produce the same action sequence.

    python export_trace.py 70117 1 2 3        # writes replay_<agent>_<seed>.txt

Usage: export_trace.py [--agents A,B] seed [seed ...]
"""
import contextlib
import hashlib
import inspect
import io
import sys
from importlib.metadata import version
from pathlib import Path

# These orderings must match the enums in sim.hpp exactly.
OPS = ["PASS", "NORTH", "SOUTH", "EAST", "WEST", "PICKUP", "DROP", "PLACE",
       "PLANT", "WATER", "HARVEST", "FERTILIZE", "DIG",
       "BUILD_COOP", "BUILD_PASTURE", "FEED", "COLLECT_FERTILIZER", "CARE"]
ITEMS = ["WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK", "WOOL",
         "FERTILIZER", "GOOSE", "COW", "SHEEP"]
MOPS = ["NONE", "HIRE", "BUY_LAND", "BUY_SEED", "BUY_PRODUCT", "BUY_ANIMAL", "SELL"]
OPI = {o: i for i, o in enumerate(OPS)}
ITI = {o: i for i, o in enumerate(ITEMS)}
MOPI = {o: i for i, o in enumerate(MOPS)}
SHOPI = {name: i for i, name in enumerate([
    "BAKERY", "BRUNCH_SPOT", "FARMERS_MARKET", "ICE_CREAM_SHOP",
    "PET_CAFE", "PIZZA_SHOP", "SMOOTHIE_SHOP", "YARN_STORE",
])}
QUADI = {name: i for i, name in enumerate(["NW", "NE", "SW", "SE"])}
ENGINE_VERSION = "1.32.7"
ENGINE_SOURCE_HASH = "bc8a54879ef02c7ea64b8b333d6a976f0ea65c4949149d01f463f23bccee653e"
FNV_OFFSET = 14695981039346656037
FNV_PRIME = 1099511628211


def enc_unit(a):
    if not isinstance(a, list) or not a:
        return (0, 0, 1)
    op = OPI.get(a[0], len(OPS))
    arg = ITI.get(a[1], 255) if len(a) >= 2 and isinstance(a[1], str) else 0
    try:
        n = int(a[2]) if len(a) >= 3 else 1
    except (TypeError, ValueError):
        n = 1
    return (op, arg, n)


def enc_order(o):
    if not isinstance(o, list) or not o:
        return (0, 0, 0)
    op = MOPI.get(o[0], 0)
    if op in (1, 2):            # HIRE and BUY_LAND carry no item or quantity
        return (op, 0, 1)
    if len(o) < 3:
        return (0, 0, 0)
    return (op, ITI.get(o[1], 255), int(o[2]))


def tile_values(tile):
    values = [0] * 13
    if tile is None:
        return values
    if tile == "LOCKED":
        values[0] = 1
        return values
    kind = tile.get("kind")
    values[0] = {"WEED": 2, "COOP": 3, "PASTURE": 4, "PLANT": 5}[kind]
    if kind == "PLANT":
        values[1] = ITI[tile["crop"]]
    elif "animal" in tile:
        values[1] = ITI[tile["animal"]]
        values[2] = 1
    values[3] = int(tile.get("watered_today", False))
    values[4] = int(tile.get("fed_today", False))
    values[5] = int(tile.get("cared_today", False))
    values[6] = int(tile.get("fertilizer_available", False))
    values[7] = int(tile.get("consecutive_unwatered", tile.get("consecutive_unfed", 0)))
    values[8] = int(tile.get("yield_units", 0))
    values[9] = int(tile.get("pending_care_bonus", 0))
    values[10] = int(tile.get("planted_day", tile.get("placed_day", 0)))
    values[11] = int(tile.get("max_lifespan_step", -1))
    values[12] = int(tile.get("fertilized_until_day", -1))
    return values


def canonical_values(frame_index, frame):
    obs = frame[0].observation
    town = obs.town
    values = [frame_index, int(obs.day), int(obs.hour), int(frame[0].status == "DONE")]
    shops = town.get("unlocked_shops", [])
    values.extend([len(shops), *(SHOPI[s] for s in shops)])
    values.extend(int(v) for v in obs.market["inventory"].values())
    values.extend(int(v) for v in obs.market["prices"].values())
    for player, farm in enumerate(obs.farms):
        private = frame[player].observation.private
        positions = [farm["farmer"], *farm["hands"]]
        quadrants = farm["unlocked_quadrants"]
        values.extend([
            int(farm["money"]), len(positions), len(quadrants), int(farm["hires_today"]),
            *(QUADI[q] for q in quadrants),
            *(int(c) for pos in positions for c in pos),
        ])
        shed = private["shed"]
        values.append(sum(int(v) for v in shed.values()))
        values.extend(int(v) for v in shed.values())
        values.extend(int(v) for v in private["seeds"].values())
        inventories = private["inventories"]
        values.append(len(inventories))
        for inventory in inventories:
            values.append(len(inventory))
            for item, quantity in inventory.items():
                values.extend([ITI[item], int(quantity)])
        for row in farm["tiles"]:
            for tile in row:
                values.extend(tile_values(tile))
    return values


def parity_hash(values):
    result = FNV_OFFSET
    for value in values:
        word = value & ((1 << 64) - 1)
        for shift in range(0, 64, 8):
            result ^= (word >> shift) & 0xFF
            result = result * FNV_PRIME & ((1 << 64) - 1)
    return result


def export(seed, agents=("starter", "random"), out_dir="."):
    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        from kaggle_environments import make
        import kaggle_environments.envs.kaggriculture.kaggriculture as engine

        installed = version("kaggle-environments")
        if installed != ENGINE_VERSION:
            raise RuntimeError(f"expected kaggle-environments {ENGINE_VERSION}, found {installed}")
        source_hash = hashlib.sha256(inspect.getsource(engine).encode()).hexdigest()
        if source_hash != ENGINE_SOURCE_HASH:
            raise RuntimeError(f"unexpected {ENGINE_VERSION} engine source hash {source_hash}")
        env = make("kaggriculture", configuration={"episodeSteps": 720, "seed": seed},
                   debug=False)
        env.run(list(agents))

    n = len(env.steps) - 1                      # number of acting turns
    # Emit the environment's ACTUAL configuration rather than letting the C++
    # side assume defaults. Package versions disagree - a Kaggle notebook image
    # and a local install differed on startingMoney (2000 vs 3000) - and a
    # hardcoded constant turns that into a silent, total miscalibration.
    cfg = env.configuration
    conf = [cfg["episodeSteps"], cfg["boardSize"], cfg["startingMoney"],
            cfg["maxMarketOrdersPerTurn"], cfg["turnsPerDay"], cfg["shedCapacity"],
            cfg["weedSpawnChance"], cfg["townShopUnlockInterval"],
            cfg["townShopSellInterval"], cfg["townCenterSellInterval"],
            cfg["farmHandCostMult"]]
    lines = [f"{seed} {n}", "CONFIG " + " ".join(str(v) for v in conf),
             f"ENGINE {installed} {source_hash}"]
    for t in range(n):
        for seat in (0, 1):
            # The action that drives steps[t] -> steps[t+1] is recorded on the
            # RESULTING state, not the originating one.
            act = env.steps[t + 1][seat].action or {}
            units = [act.get("farmer") or ["PASS"]] + list(act.get("hands") or [])
            orders = list(act.get("market") or [])
            parts = [str(len(units)), str(len(orders))]
            for u in units:
                parts += [str(v) for v in enc_unit(u)]
            for o in orders:
                parts += [str(v) for v in enc_order(o)]
            lines.append(" ".join(parts))

    money = [[float(s[0].observation.farms[0]["money"]),
              float(s[0].observation.farms[1]["money"])] for s in env.steps]
    inv = [[int(v) for v in s[0].observation.market["inventory"].values()] for s in env.steps]
    hashes = [parity_hash(canonical_values(i, frame)) for i, frame in enumerate(env.steps)]
    lines.append("TRUTH")
    for i in range(len(money)):
        lines.append(" ".join([f"{money[i][0]:.0f}", f"{money[i][1]:.0f}"] +
                              [str(v) for v in inv[i]] + [str(hashes[i])]))

    agent_name = Path(str(agents[0])).stem
    p = Path(out_dir) / f"replay_{agent_name}_{seed}.txt"
    p.write_text("\n".join(lines) + "\n")
    print(f"seed {seed}: final money {money[-1]}  -> {p.name}")
    return p


def main():
    args = sys.argv[1:]
    agents = ("starter", "random")
    if args and args[0] == "--agents":
        args.pop(0)
        agents = tuple(args.pop(0).split(","))
    seeds = [int(x) for x in (args or [70117])]
    for s in seeds:
        export(s, agents)


if __name__ == "__main__":
    main()
