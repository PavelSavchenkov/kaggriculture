"""Differential port verification on recorded observations, not Python gameplay."""
import importlib.util
import json
import subprocess
from copy import deepcopy
from collections import Counter
from pathlib import Path
from generate_public_routes import ITEMS, OPS, MARKET, triple

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


def main():
    source = EXP / "research/notebooks/thomastschinkel/kaggriculture-public-state-router-74-5-win-rate/extracted_main.py"
    spec = importlib.util.spec_from_file_location("public_router_reference", source)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    fixture = EXP / "tests/public_router_cases.txt"
    routes = Counter()
    with fixture.open("w") as out:
        paths = sorted((EXP / "replays").glob("*replay.json"))[:4]
        for path in paths:
            replay = json.loads(path.read_text())
            for seat in range(2):
                agent = module.Agent()
                for step, pair in enumerate(replay["steps"][:719]):
                    obs = pair[seat]["observation"]
                    action = agent.act(obs)  # Direct method: never suppress reference errors.
                    routes[agent.cur] += 1
                    out.write(pack(obs, action, step == 0))
        # Exercise every branch, including milk when the natural replay sample
        # does not cross its threshold. Observations remain exogenous fixtures.
        replay = json.loads(paths[0].read_text())
        for branch in range(4):
            agent = module.Agent()
            for step, pair in enumerate(replay["steps"][:719]):
                obs = deepcopy(pair[0]["observation"])
                if step == 226:
                    obs["town"]["unlocked_shops"] = ["YARN_STORE" if branch in (1, 2) else "BAKERY"]
                if step == 360:
                    obs["market"]["prices"]["CARROT"] = 42 if branch == 2 else 41
                if step == 433:
                    obs["market"]["inventory"]["MILK"] = 10067 if branch == 3 else 10066
                action = agent.act(obs)
                routes[agent.cur] += 1
                out.write(pack(obs, action, step == 0))
        assert len(routes) == 4, routes
    binary = EXP / "build/public_router_parity"
    command = ["conda", "run", "-n", "kaggriculture", "g++", "-std=c++20", "-O2", "-I", str(ROOT),
               str(EXP / "tests/public_router_parity.cpp"), str(EXP / "league/public_router/source/agent.cpp"), "-o", str(binary)]
    subprocess.run(command, check=True)
    result = subprocess.run([str(binary), str(fixture)], check=True, capture_output=True, text=True)
    report = {"result": result.stdout.strip(), "reference_route_actions": dict(routes),
              "scope": "8 full recorded observation sequences plus 4 full forced-branch fixtures; normalized active worker count; inventory key order as supplied",
              "command": command}
    (EXP / "results/public_router_parity.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
