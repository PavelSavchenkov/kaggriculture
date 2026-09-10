"""Offline replay -> financial flows and independent verification truth.

Unit rules come from the installed official engine. No notebook code executes.
The C++ checker validates the resulting calendars and full original schedules.
"""
import argparse
import copy
import csv
import hashlib
import inspect
import json
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

from kaggle_environments.envs.kaggriculture import kaggriculture as game

EXP = Path(__file__).resolve().parents[1]
ITEMS = ["WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK", "WOOL", "FERTILIZER", "GOOSE", "COW", "SHEEP"]
OPS = ["PASS", "NORTH", "SOUTH", "EAST", "WEST", "PICKUP", "DROP", "PLACE", "PLANT", "WATER", "HARVEST", "FERTILIZE", "DIG", "BUILD_COOP", "BUILD_PASTURE", "FEED", "COLLECT_FERTILIZER", "CARE"]
MOPS = ["NONE", "HIRE", "BUY_LAND", "BUY_SEED", "BUY_PRODUCT", "BUY_ANIMAL", "SELL"]
SHOPS = ["BAKERY", "BRUNCH_SPOT", "FARMERS_MARKET", "ICE_CREAM_SHOP", "PET_CAFE", "PIZZA_SHOP", "SMOOTHIE_SHOP", "YARN_STORE"]
ITI = {name: i for i, name in enumerate(ITEMS)}
FLOW = {name: i for i, name in enumerate(["produce", "use", "withdraw", "deposit", "drop", "use_seed", "drop_all"])}
CONFIG = ["episodeSteps", "boardSize", "startingMoney", "maxMarketOrdersPerTurn", "turnsPerDay", "shedCapacity", "weedSpawnChance", "townShopUnlockInterval", "townShopSellInterval", "townCenterSellInterval", "farmHandCostMult"]


def account(observation, seat):
    farm = observation["farms"][seat]
    private = observation["private"]
    units = len(farm["hands"]) + 1
    assert units <= 40
    result = [farm["money"], sum(private["shed"].values()), units, farm["hires_today"], len(farm["unlocked_quadrants"])]
    result += [private["shed"].get(item, 0) for item in ITEMS]
    result += [private["seeds"].get(item, 0) for item in ITEMS[:5]]
    for inventory in private["inventories"]:
        result.append(len(inventory))
        for item, quantity in inventory.items():
            result.extend([ITI[item], quantity])
    return result


def unit(command):
    if not isinstance(command, list) or not command:
        return [0, 0, 1]
    return [OPS.index(command[0]) if command[0] in OPS else 18,
            ITI.get(command[1], 255) if len(command) > 1 else 255,
            int(command[2]) if len(command) > 2 else 1]


def order(command):
    parsed = game._parse_order(command)
    if parsed is None:
        return [0, 0, 0]
    return [MOPS.index(parsed["type"]), ITI.get(parsed.get("item"), 255), parsed.get("remaining", 1)]


def extract(path, output, allow_reserved=False):
    replay = json.loads(path.read_bytes())
    episode = replay["info"]["EpisodeId"]
    steps = replay["steps"]
    cfg = replay["configuration"]
    assert len(steps) == 720 and cfg["boardSize"] == 10 and not cfg["marketParams"]
    assert cfg["turnsPerDay"] == 24 and cfg["shedCapacity"] == 100
    names = replay["info"]["TeamNames"]
    manifest = json.loads((EXP / "data/MANIFEST.json").read_text())
    reserved = {c["episode"] for c in manifest["cases"] if c["status"] == "reserved"}
    if not allow_reserved and episode in reserved:
        return {"episode": episode, "status": "reserved_rival", "teams": names}
    lines = []

    def emit(values):
        lines.append(" ".join(map(str, values)))

    emit(["SPCAL1", episode, len(steps) - 1, replay["info"]["seed"]])
    emit([cfg[key] for key in CONFIG])
    emit([steps[0][0]["observation"]["market"]["inventory"][item] for item in ITEMS[:9]])
    for seat in range(2):
        emit(account(steps[0][seat]["observation"], seat))
    key_orders = [[list(inv) for inv in steps[0][seat]["observation"]["private"]["inventories"]] for seat in range(2)]
    counts = Counter()
    for turn in range(len(steps) - 1):
        obs = steps[turn][0]["observation"]
        shops = obs["town"]["unlocked_shops"]
        emit([len(shops), *(SHOPS.index(s) for s in shops)])
        for seat in range(2):
            farm = copy.deepcopy(obs["farms"][seat])
            private = copy.deepcopy(steps[turn][seat]["observation"]["private"])
            for b, inv in enumerate(private["inventories"]):
                previous = key_orders[seat][b] if b < len(key_orders[seat]) else []
                keys = [k for k in previous if k in inv] + [k for k in inv if k not in previous]
                private["inventories"][b] = {k: inv[k] for k in keys}
            action = steps[turn + 1][seat].get("action") or {}
            commands = [action.get("farmer") or ["PASS"], *(action.get("hands") or [])]
            assert len(commands) <= 40
            demand = Counter(c[1] for c in commands if isinstance(c, list) and len(c) > 1 and c[0] == "PLANT")
            blocked = {item for item, n in demand.items() if n > private["seeds"].get(item, 0)}
            before, after = [], []

            def event(flow, item, buffer, quantity):
                before.append([FLOW[flow], ITI[item], buffer, quantity])
                counts[flow] += 1

            for b, original in enumerate(commands):
                if b >= len(private["inventories"]):
                    continue
                command = ["PASS"] if len(original) > 1 and original[0] == "PLANT" and original[1] in blocked else original
                op = command[0] if command else "PASS"
                inv = dict(private["inventories"][b])
                seeds = dict(private["seeds"])
                position = farm["farmer"] if b == 0 else farm["hands"][b - 1]
                tile = farm["tiles"][position[1]][position[0]]
                adjacent = position[0] in (4, 5) and position[1] in (4, 5)
                animal_place = op == "PLACE" and len(command) > 1 and command[1] in game.ANIMALS and isinstance(tile, dict) and tile.get("kind") == game.ANIMALS[command[1]]["structure"] and "animal" not in tile
                game._apply_unit_action(farm, private, b, command, 10, obs["day"], 24, 100)
                current = private["inventories"][b]
                if op == "DROP" and adjacent:
                    event("drop_all", "WHEAT", b, 0)
                elif op == "PLACE" and adjacent and not animal_place and len(command) > 1 and command[1] in ITI:
                    n = int(command[2]) if len(command) > 2 else 1
                    if n > 0:
                        event("deposit", command[1], b, n)
                else:
                    for item in dict.fromkeys([*inv, *current]):
                        delta = current.get(item, 0) - inv.get(item, 0)
                        if delta > 0:
                            event("withdraw" if op == "PICKUP" else "produce", item, b, delta)
                        elif delta < 0:
                            event("use", item, b, -delta)
                for item in ITEMS[:5]:
                    used = seeds[item] - private["seeds"][item]
                    if used:
                        event("use_seed", item, b, used)
            if obs["hour"] == 23:
                after = [[FLOW["drop_all"], 0, b, 0] for b in range(len(private["inventories"]))]
                key_orders[seat] = [[]]
            else:
                key_orders[seat] = [list(inv) for inv in private["inventories"]]
            emit([len(before)])
            for row in before:
                emit(row)
            emit([len(after)])
            for row in after:
                emit(row)
            raw_orders = (action.get("market") or [])[:10]
            emit([len(raw_orders)])
            for raw in raw_orders:
                emit(order(raw))
            emit([len(commands)])
            for command in commands:
                emit(unit(command))
        for seat in range(2):
            emit(account(steps[turn + 1][seat]["observation"], seat))
        emit([steps[turn + 1][0]["observation"]["market"]["inventory"][item] for item in ITEMS[:9]])
    target = output / f"{episode}.calendar"
    target.write_text("\n".join(lines) + "\n")
    return {"episode": episode, "status": "extracted", "teams": names,
            "replay_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "calendar_sha256": hashlib.sha256(target.read_bytes()).hexdigest(),
            "counts": dict(counts), "calendar": str(target.relative_to(EXP))}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--limit", type=int, default=0)
    parser.add_argument("--output", type=Path, default=EXP / "research/calendars_v0")
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--split", type=Path, default=EXP / "data/MANIFEST.json")
    parser.add_argument("--allow-reserved", action="store_true", help="Explicitly expose reserved source families for discovery; record split change first.")
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=args.resume)
    split = json.loads(args.split.read_text())
    episodes = split["development_episodes"] if "development_episodes" in split else [
        c["episode"] for c in split["cases"] if c["status"] == "exposed"]
    if args.limit:
        episodes = episodes[:args.limit]
    results = []
    for episode in episodes:
        result = extract(EXP / "replays" / f"episode-{episode}-replay.json", args.output, args.allow_reserved)
        results.append(result)
        print(json.dumps(result), flush=True)
    record = {"completed_utc": datetime.now(timezone.utc).isoformat(), "cases": results,
              "allow_reserved": args.allow_reserved,
              "extractor_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "official_engine_sha256": hashlib.sha256(inspect.getsource(game).encode()).hexdigest(),
              "scope": "Conditional fixed accepted input withdrawals; original valid deposit limits; original production events. No route or biology certification from the financial model alone."}
    (args.output / "MANIFEST.json").write_text(json.dumps(record, indent=2) + "\n")


if __name__ == "__main__":
    main()
