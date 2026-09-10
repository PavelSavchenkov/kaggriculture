"""Stable tensor and action constants shared by loaders, models, and trainers."""

from enum import IntEnum

BOARD = 10
MAX_UNITS = 40
MAX_ORDERS = 10
N_ITEMS = 12
N_PRODUCTS = 9
N_CROPS = 5
N_SHOPS = 8
N_UNIT_OPS = 18  # includes PASS, excludes engine sentinel INVALID
N_MARKET_OPS = 8  # engine NONE plus six ops and decoder-only END
MARKET_END = 7
N_QUANTITIES = 102  # exact 0..100 plus ALL_AVAILABLE
ALL_AVAILABLE = 101


class UnitOp(IntEnum):
    PASS = 0
    NORTH = 1
    SOUTH = 2
    EAST = 3
    WEST = 4
    PICKUP = 5
    DROP = 6
    PLACE = 7
    PLANT = 8
    WATER = 9
    HARVEST = 10
    FERTILIZE = 11
    DIG = 12
    BUILD_COOP = 13
    BUILD_PASTURE = 14
    FEED = 15
    COLLECT_FERTILIZER = 16
    CARE = 17


class MarketOp(IntEnum):
    NONE = 0
    HIRE = 1
    BUY_LAND = 2
    BUY_SEED = 3
    BUY_PRODUCT = 4
    BUY_ANIMAL = 5
    SELL = 6
