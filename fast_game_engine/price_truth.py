"""Emit exhaustive official market prices for price_validate.cpp."""
from importlib.metadata import version
import hashlib
import inspect

from kaggle_environments.envs.kaggriculture import kaggriculture as engine

from export_trace import ENGINE_SOURCE_HASH, ENGINE_VERSION, ITEMS


assert version("kaggle-environments") == ENGINE_VERSION
assert hashlib.sha256(inspect.getsource(engine).encode()).hexdigest() == ENGINE_SOURCE_HASH
for item_index, item in enumerate(ITEMS[:9]):
    for inventory in range(-20000, 40001):
        print(item_index, inventory, engine.market_price(item, inventory))
