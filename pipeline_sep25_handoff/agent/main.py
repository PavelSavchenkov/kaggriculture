"""bc_opus agent: a behaviour-cloned DayIntent network (v12, strength-conditioned; opening style
and compiler options from the sidecars in model/) compiled into each day's actions by a
deterministic C++ day compiler.

The C++ agent (libopus_lb_bridge.so, portable x86-64-v2 build with a static C++ runtime)
keeps its state per game; this module converts each Kaggle observation to the flat buffer
the bridge parses and its integer action back to the Kaggle action format.
"""
import ctypes
import os
import sys
from pathlib import Path


def _package_dir():
    """The directory with the bridge. Kaggle runs this file via exec() without __file__, after
    appending its directory to sys.path; the archive is unpacked in /kaggle_simulations/agent."""
    source = globals().get("__file__")
    candidates = [Path(source).resolve().parent] if source else []
    candidates += [Path(entry) for entry in reversed(sys.path) if entry]
    candidates += [Path.cwd(), Path("/kaggle_simulations/agent")]
    for directory in candidates:
        if (directory / "libopus_lb_bridge.so").is_file():
            return directory.resolve()
    raise FileNotFoundError("libopus_lb_bridge.so is not beside main.py")


HERE = _package_dir()
MODEL = str(HERE / "model" / "model.bin")

ITEMS = ["WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK", "WOOL",
         "FERTILIZER", "GOOSE", "COW", "SHEEP"]
ITEM_ID = {name: index for index, name in enumerate(ITEMS)}
UNIT_OPS = ["PASS", "NORTH", "SOUTH", "EAST", "WEST", "PICKUP", "DROP", "PLACE",
            "PLANT", "WATER", "HARVEST", "FERTILIZE", "DIG",
            "BUILD_COOP", "BUILD_PASTURE", "FEED", "COLLECT_FERTILIZER", "CARE"]
MARKET_OPS = ["NONE", "HIRE", "BUY_LAND", "BUY_SEED", "BUY_PRODUCT", "BUY_ANIMAL", "SELL"]
SHOP_ID = {name: index for index, name in enumerate([
    "BAKERY", "BRUNCH_SPOT", "FARMERS_MARKET", "ICE_CREAM_SHOP",
    "PET_CAFE", "PIZZA_SHOP", "SMOOTHIE_SHOP", "YARN_STORE"])}
CONFIG_KEYS = ("episodeSteps", "boardSize", "startingMoney", "maxMarketOrdersPerTurn", "turnsPerDay",
               "shedCapacity", "weedSpawnChance", "townShopUnlockInterval", "townShopSellInterval",
               "townCenterSellInterval", "farmHandCostMult")
CONFIG_DEFAULTS = (720, 10, 3000, 10, 24, 100, 0.005, 3, 4, 24, 1)

_library = None
_games = {}  # seat -> [bridge pointer, last step seen]


def _get(value, key, default=None):
    if isinstance(value, dict):
        return value.get(key, default)
    getter = getattr(value, "get", None)
    return getter(key, default) if callable(getter) else getattr(value, key, default)


def _bridge():
    global _library
    if _library is None:
        library = ctypes.CDLL(str(HERE / "libopus_lb_bridge.so"))
        library.opus_new.argtypes = [ctypes.POINTER(ctypes.c_double), ctypes.c_int, ctypes.c_int]
        library.opus_new.restype = ctypes.c_void_p
        library.opus_act.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_double), ctypes.c_int,
                                     ctypes.POINTER(ctypes.c_int32), ctypes.c_int]
        library.opus_act.restype = ctypes.c_int
        library.opus_delete.argtypes = [ctypes.c_void_p]
        _library = library
    return _library


def _tile_values(tile):
    values = [0] * 13
    if tile is None:
        return values
    if tile == "LOCKED":
        values[0] = 1
        return values
    kind = _get(tile, "kind")
    values[0] = {"WEED": 2, "COOP": 3, "PASTURE": 4, "PLANT": 5}.get(kind, 0)
    if kind == "PLANT":
        values[1] = ITEM_ID[_get(tile, "crop")]
    elif _get(tile, "animal") is not None:
        values[1] = ITEM_ID[_get(tile, "animal")]
        values[2] = 1
    values[3] = int(bool(_get(tile, "watered_today", False)))
    values[4] = int(bool(_get(tile, "fed_today", False)))
    values[5] = int(bool(_get(tile, "cared_today", False)))
    values[6] = int(bool(_get(tile, "fertilizer_available", False)))
    values[7] = int(_get(tile, "consecutive_unwatered", _get(tile, "consecutive_unfed", 0)) or 0)
    values[8] = int(_get(tile, "yield_units", 0) or 0)
    values[9] = int(_get(tile, "pending_care_bonus", 0) or 0)
    values[10] = int(_get(tile, "planted_day", _get(tile, "placed_day", 0)) or 0)
    lifespan = _get(tile, "max_lifespan_step", -1)
    values[11] = int(-1 if lifespan is None else lifespan)
    fertilized = _get(tile, "fertilized_until_day", -1)
    values[12] = int(-1 if fertilized is None else fertilized)
    return values


def _fields(o):
    """Flatten one Kaggle observation in the order the bridge parses (lb_bridge.cpp)."""
    player = int(_get(o, "player", 0))
    step = int(_get(o, "step", 0) or 0)
    values = [player, step, int(_get(o, "day", step // 24)), int(_get(o, "hour", step % 24))]
    farms = _get(o, "farms")
    for farm in farms:
        positions = [_get(farm, "farmer"), *(_get(farm, "hands") or [])]
        values.extend([_get(farm, "money"), len(positions), len(_get(farm, "unlocked_quadrants") or []),
                       _get(farm, "hires_today", 0) or 0])
        values.extend(coordinate for position in positions for coordinate in position)
        for row in _get(farm, "tiles"):
            for tile in row:
                values.extend(_tile_values(tile))
    private = _get(o, "private")
    shed, seeds = _get(private, "shed") or {}, _get(private, "seeds") or {}
    values.extend(shed.get(item, 0) for item in ITEMS)
    values.extend(seeds.get(item, 0) for item in ITEMS[:5])
    inventories = _get(private, "inventories") or []
    workers = len(_get(farms[player], "hands") or []) + 1
    inventories = (list(inventories) + [{}] * workers)[:workers]
    for inventory in inventories:
        items = [(ITEM_ID[item], quantity) for item, quantity in (inventory or {}).items() if quantity > 0]
        values.append(len(items))
        values.extend(value for pair in items for value in pair)
    market = _get(o, "market")
    values.extend(_get(market, "inventory")[item] for item in ITEMS[:9])
    values.extend(_get(market, "prices")[item] for item in ITEMS[:9])
    shops = _get(_get(o, "town"), "unlocked_shops") or []
    values.extend([len(shops), *(SHOP_ID[shop] for shop in shops)])
    overage = _get(o, "remainingOverageTime")
    if overage is not None:  # optional trailing field: the bridge's time-pressure valve
        values.append(float(overage))
    return (ctypes.c_double * len(values))(*values)


def _action(values):
    units, orders = [], []
    for index in range(values[0]):
        op, item, count = values[2 + 3 * index:5 + 3 * index]
        value = [UNIT_OPS[op]]
        if op in (5, 7):  # PICKUP, PLACE
            value.extend([ITEMS[item], count])
        elif op == 8:  # PLANT
            value.append(ITEMS[item])
        units.append(value)
    offset = 2 + 3 * values[0]
    for index in range(values[1]):
        op, item, count = values[offset + 3 * index:offset + 3 * index + 3]
        if op == 0:
            orders.append([])  # as evaluated (scripts/lb_play.py): keeps list positions
        else:
            orders.append([MARKET_OPS[op]] if op in (1, 2) else [MARKET_OPS[op], ITEMS[item], count])
    return {"farmer": units[0], "hands": units[1:], "market": orders}


def agent(observation, configuration=None):
    library = _bridge()
    seat = int(_get(observation, "player", 0))
    step = int(_get(observation, "step", 0) or 0)
    game = _games.get(seat)
    if game is None or step <= game[1]:  # first call or a new game
        if game is not None:
            library.opus_delete(game[0])
        config = [(_get(configuration, key, default) if configuration is not None else default)
                  for key, default in zip(CONFIG_KEYS, CONFIG_DEFAULTS)]
        values = (ctypes.c_double * len(config))(*[float(v) for v in config])
        # The bridge reads its model path when a game starts. Set it here, not at import: other
        # agents in the same process (e.g. another version of this one) set the same variable.
        os.environ["BC_OPUS_MODEL"] = MODEL
        game = [library.opus_new(values, len(values), seat), -1]
        _games[seat] = game
    game[1] = step
    fields = _fields(observation)
    buffer = (ctypes.c_int32 * 4096)()
    count = library.opus_act(game[0], fields, len(fields), buffer, len(buffer))
    return _action(buffer[:count])
