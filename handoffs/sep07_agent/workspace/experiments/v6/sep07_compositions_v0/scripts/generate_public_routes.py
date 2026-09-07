"""Decode public notebook route data into immutable C++; no policy execution."""
import ast
import base64
import hashlib
import json
import shutil
import zlib
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
SOURCE = EXP / "research/notebooks/thomastschinkel/kaggriculture-public-state-router-74-5-win-rate/extracted_main.py"
TARGET = EXP / "league/public_router"
ITEMS = "WHEAT CARROT TOMATO STRAWBERRY MELON EGG MILK WOOL FERTILIZER GOOSE COW SHEEP".split()
OPS = "PASS NORTH SOUTH EAST WEST PICKUP DROP PLACE PLANT WATER HARVEST FERTILIZE DIG BUILD_COOP BUILD_PASTURE FEED COLLECT_FERTILIZER CARE".split()
MARKET = "PASS HIRE BUY_LAND BUY_SEED BUY_PRODUCT BUY_ANIMAL SELL".split()


def triple(value, market=False):
    op = (MARKET if market else OPS).index(value[0])
    # The engine ignores arguments on PASS/HIRE/BUY_LAND. Some recent public
    # agents attach arbitrary numeric HIRE arguments; they are not item IDs.
    if market and op <= 2:return [op,0,0]
    item = ITEMS.index(value[1]) if len(value) > 1 else 0
    count = int(value[2]) if len(value) > 2 else (0 if market and op <= 2 else 1)
    return [op, item, count]


def main():
    tree = ast.parse(SOURCE.read_text())
    blob = next(ast.literal_eval(node.value) for node in tree.body if isinstance(node, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == "_BLOB" for t in node.targets))
    data = json.loads(zlib.decompress(base64.b64decode(blob)))
    routes = {data["main"]: data["full"]}
    for tail in data["tails"]:
        routes[tail["h"]] = routes[tail["parent"]][:tail["at"]] + tail["suffix"]
    values, offsets = [], []
    for route in routes.values():
        assert len(route) == 720
        row = []
        for action in route:
            row.append(len(values))
            units = [action["farmer"], *action["hands"]]
            orders = action["market"]
            values += [len(units), len(orders)]
            for unit in units:
                values.extend(triple(unit))
            for order in orders:
                values.extend(triple(order, True))
        offsets.append(row)
    code = "// Generated from the archived public notebook; see IMPORT.json.\n"
    code += "inline constexpr int route_offsets[4][720] = {\n"
    code += ",\n".join("{" + ",".join(map(str, row)) + "}" for row in offsets) + "\n};\n"
    code += "inline constexpr int route_data[] = {\n"
    code += ",\n".join(",".join(map(str, values[i:i+100])) for i in range(0, len(values), 100)) + "\n};\n"
    prefix = []
    for a in routes.values():
        prefix.append([next((t for t in range(720) if a[t] != b[t]), 720) for b in routes.values()])
    code += "inline constexpr int route_prefix[4][4] = {" + ",".join("{" + ",".join(map(str, row)) + "}" for row in prefix) + "};\n"
    (TARGET / "source").mkdir(parents=True, exist_ok=True)
    (TARGET / "source/routes.inc").write_text(code)
    shutil.copy2(SOURCE, TARGET / "upstream_reference.py.txt")
    shutil.copy2(SOURCE.parent / "kernel-metadata.json", TARGET / "notebook-metadata.json")
    (TARGET / "IMPORT.json").write_text(json.dumps({
        "source": "https://www.kaggle.com/code/thomastschinkel/kaggriculture-public-state-router-74-5-win-rate",
        "source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
        "route_labels": list(routes), "route_prefix": prefix,
        "changes": ["Python policy ported to typed C++", "Immutable routes and future sell totals decoded once",
                    "Normalize emitted worker action count to active workers as required by local API"],
        "route_original_episode_ids": "Not supplied in notebook; retain route hashes, do not invent player attribution",
    }, indent=2) + "\n")
    print(json.dumps({"prefix": prefix, "values": len(values)}))


if __name__ == "__main__":
    main()
