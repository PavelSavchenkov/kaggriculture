from collections import Counter
from pathlib import Path
import sys


OPS = [
    "PASS", "NORTH", "SOUTH", "EAST", "WEST", "PICKUP", "DROP", "PLACE",
    "PLANT", "WATER", "HARVEST", "FERTILIZE", "DIG", "BUILD_COOP",
    "BUILD_PASTURE", "FEED", "COLLECT_FERTILIZER", "CARE", "INVALID",
]
MOPS = ["NONE", "HIRE", "BUY_LAND", "BUY_SEED", "BUY_PRODUCT", "BUY_ANIMAL", "SELL"]


def coverage(path):
    lines = Path(path).read_text().splitlines()
    turns = int(lines[0].split()[1])
    cursor = 2 + int(lines[2].startswith("ENGINE "))
    unit_ops = Counter()
    market_ops = Counter()
    max_units = 0
    max_orders = 0
    for line in lines[cursor:cursor + turns * 2]:
        values = [int(value) for value in line.split()]
        n_units, n_orders = values[:2]
        max_units = max(max_units, n_units)
        max_orders = max(max_orders, n_orders)
        offset = 2
        for _ in range(n_units):
            unit_ops[OPS[values[offset]]] += 1
            offset += 3
        for _ in range(n_orders):
            market_ops[MOPS[values[offset]]] += 1
            offset += 3
    print(Path(path).name)
    print(f"  max units/orders: {max_units}/{max_orders}")
    print("  unit:", " ".join(f"{name}={count}" for name, count in unit_ops.most_common()))
    print("  market:", " ".join(f"{name}={count}" for name, count in market_ops.most_common()))


for argument in sys.argv[1:]:
    coverage(argument)
