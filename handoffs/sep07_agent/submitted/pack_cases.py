"""Differential port verification on recorded observations, not Python gameplay."""
import importlib.util
import json
import subprocess
from copy import deepcopy
from collections import Counter
from pathlib import Path
ITEMS = "WHEAT CARROT TOMATO STRAWBERRY MELON EGG MILK WOOL FERTILIZER GOOSE COW SHEEP".split()
OPS = "PASS NORTH SOUTH EAST WEST PICKUP DROP PLACE PLANT WATER HARVEST FERTILIZE DIG BUILD_COOP BUILD_PASTURE FEED COLLECT_FERTILIZER CARE".split()
MARKET = "PASS HIRE BUY_LAND BUY_SEED BUY_PRODUCT BUY_ANIMAL SELL".split()
def triple(value, market=False):
    op = (MARKET if market else OPS).index(value[0])
    if market and op <= 2: return [op, 0, 0]
    item = ITEMS.index(value[1]) if len(value) > 1 else 0
    count = int(value[2]) if len(value) > 2 else 1
    return [op, item, count]


EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]
SHOPS = "BAKERY BRUNCH_SPOT FARMERS_MARKET ICE_CREAM_SHOP PET_CAFE PIZZA_SHOP SMOOTHIE_SHOP YARN_STORE".split()
KINDS = {None: 0, "LOCKED": 1, "WEED": 2, "COOP": 3, "PASTURE": 4, "PLANT": 5}


def pack(obs, action, reset, full_tiles=False):
    values = [int(reset), obs["player"], obs.get("step", obs["day"] * 24 + obs["hour"]), obs["day"], obs["hour"]]
    for farm in obs["farms"]:
        positions = [farm["farmer"], *farm["hands"]]
        values += [farm["money"], len(positions), len(farm["unlocked_quadrants"]), farm["hires_today"]]
        for xy in positions:
            values.extend(xy)
        for row in farm["tiles"]:
            for tile in row:
                values.append(KINDS[tile.get("kind") if isinstance(tile, dict) else tile])
                if full_tiles:
                    t=tile if isinstance(tile,dict) else {}
                    animal=t.get("animal")
                    values += [ITEMS.index(t.get("crop") or animal) if t.get("crop") or animal else 0,
                        int(bool(animal)),int(t.get("watered_today",False)),int(t.get("fed_today",False)),
                        int(t.get("cared_today",False)),int(t.get("fertilizer_available",False)),
                        t.get("consecutive_unwatered",t.get("consecutive_unfed",0)),t.get("yield_units",0),
                        t.get("pending_care_bonus",0),t.get("planted_day",t.get("placed_day",0)),
                        t.get("max_lifespan_step",-1),t.get("fertilized_until_day",-1)]
    own = obs["private"]
    values += [own["shed"].get(i, 0) for i in ITEMS]
    values += [own["seeds"].get(i, 0) for i in ITEMS[:5]]
    n_units = len(obs["farms"][obs["player"]]["hands"]) + 1
    for u in range(n_units):
        inv = own["inventories"][u] if u < len(own["inventories"]) else {}
        values.append(len(inv))
        for item, count in inv.items():
            values += [ITEMS.index(item), count]
    values += [obs["market"]["prices"][i] for i in ITEMS[:9]]
    values += [obs["market"]["inventory"][i] for i in ITEMS[:9]]
    shops = obs["town"]["unlocked_shops"]
    values += [len(shops), *[SHOPS.index(s) for s in shops]]
    units = [action["farmer"], *action["hands"]]
    units = (units + [["PASS"]] * n_units)[:n_units]
    orders = action["market"]
    values += [n_units, len(orders)]
    for unit in units:
        values.extend(triple(unit))
    for order in orders:
        values.extend(triple(order, True))
    return " ".join(map(str, values)) + "\n"

