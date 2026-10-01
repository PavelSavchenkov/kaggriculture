"""Full games of the bc_opus C++ agent against Local-LB Python agents in the official
kaggle-environments interpreter (direct interpreter loop, observation isolation as the
framework). The C++ agent runs through build/libopus_lb_bridge.so.

usage: lb_play.py out_dir opponent [opponent ...] --seeds 700-715 [--procs 24]
Opponents are loaded from data/localLB_3326cc6 (Local-LB origin/main 3326cc6) with the
Local-LB loader. Each game writes <out_dir>/<opponent>_<seed>_seat<k>.json.
"""
import os
for name in ("OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS", "MKL_NUM_THREADS", "NUMEXPR_NUM_THREADS"):
    os.environ[name] = "1"
import argparse
import ast
import contextlib
import copy
import ctypes
import io
import json
import multiprocessing
import sys
import time
import traceback
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[2]
LB = ROOT / "data" / os.environ.get("LB_SNAPSHOT", "localLB_3326cc6")  # LB_SNAPSHOT: another Local-LB snapshot
# Our model, read before any opponent is imported: some Local-LB agents (earlier versions of this
# agent) set BC_OPUS_MODEL at import or at game start, which our bridge reads in opus_new.
MODEL = os.environ.get("BC_OPUS_MODEL", "")
sys.path.insert(0, str(REPO / "fast_game_engine"))
from export_trace import ITEMS, ITI, OPS, MOPS, SHOPI, tile_values  # noqa: E402

CONFIG_KEYS = ("episodeSteps", "boardSize", "startingMoney", "maxMarketOrdersPerTurn", "turnsPerDay",
               "shedCapacity", "weedSpawnChance", "townShopUnlockInterval", "townShopSellInterval",
               "townCenterSellInterval", "farmHandCostMult")


def observation_fields(o):
    """Flatten one Kaggle observation in the order source/lb_bridge.cpp parses."""
    values = [o["player"], o["step"], o["day"], o["hour"]]
    for farm in o["farms"]:
        positions = [farm["farmer"], *farm["hands"]]
        values.extend([farm["money"], len(positions), len(farm["unlocked_quadrants"]), farm["hires_today"]])
        values.extend(coordinate for position in positions for coordinate in position)
        for row in farm["tiles"]:
            for tile in row:
                values.extend(tile_values(tile))
    private = o["private"]
    values.extend(private["shed"].get(item, 0) for item in ITEMS)
    values.extend(private["seeds"].get(item, 0) for item in ITEMS[:5])
    if len(private["inventories"]) != len(o["farms"][o["player"]]["hands"]) + 1:
        raise ValueError("private inventory count disagrees with visible workers")
    for inventory in private["inventories"]:
        items = [(ITI[item], quantity) for item, quantity in inventory.items() if quantity > 0]
        values.append(len(items))
        values.extend(value for pair in items for value in pair)
    values.extend(o["market"]["inventory"][item] for item in ITEMS[:9])
    values.extend(o["market"]["prices"][item] for item in ITEMS[:9])
    shops = o["town"]["unlocked_shops"]
    values.extend([len(shops), *(SHOPI[shop] for shop in shops)])
    return (ctypes.c_double * len(values))(*values)


def decode_action(values):
    units, orders = [], []
    for index in range(values[0]):
        op, item, count = values[2 + 3 * index:5 + 3 * index]
        value = [OPS[op]]
        if op in (5, 7):  # PICKUP, PLACE
            value.extend([ITEMS[item], count])
        elif op == 8:  # PLANT
            value.append(ITEMS[item])
        units.append(value)
    offset = 2 + 3 * values[0]
    for index in range(values[1]):
        op, item, count = values[offset + 3 * index:offset + 3 * index + 3]
        if op == 0:
            orders.append([])
        elif op in (1, 2):
            orders.append([MOPS[op]])
        else:
            orders.append([MOPS[op], ITEMS[item], count])
    return dict(farmer=units[0], hands=units[1:], market=orders)


class Native:
    def __init__(self, config, seat):
        self.lib = ctypes.CDLL(str(ROOT / os.environ.get("BUILD", "build") / "libopus_lb_bridge.so"))  # BUILD: build dir
        self.lib.opus_new.argtypes = [ctypes.POINTER(ctypes.c_double), ctypes.c_int, ctypes.c_int]
        self.lib.opus_new.restype = ctypes.c_void_p
        self.lib.opus_act.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_double), ctypes.c_int,
                                      ctypes.POINTER(ctypes.c_int32), ctypes.c_int]
        self.lib.opus_act.restype = ctypes.c_int
        self.lib.opus_stats.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_double)]
        self.lib.opus_delete.argtypes = [ctypes.c_void_p]
        self.lib.opus_last_day.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
        self.lib.opus_last_day.restype = ctypes.c_int
        values = (ctypes.c_double * len(CONFIG_KEYS))(*(config[key] for key in CONFIG_KEYS))
        other = os.environ.get("BC_OPUS_MODEL")  # the opponent's, if it set one at import
        os.environ["BC_OPUS_MODEL"] = MODEL
        self.pointer = self.lib.opus_new(values, len(values), seat)
        if other is not None:
            os.environ["BC_OPUS_MODEL"] = other
        self.buffer = (ctypes.c_int32 * 4096)()

    def act(self, observation, configuration=None):
        fields = observation_fields(observation)
        count = self.lib.opus_act(self.pointer, fields, len(fields), self.buffer, len(self.buffer))
        return decode_action(self.buffer[:count])

    def last_day(self):
        text = ctypes.create_string_buffer(2048)
        self.lib.opus_last_day(self.pointer, text, 2048)
        return text.value.decode()

    def stats(self):
        out = (ctypes.c_double * 4)()
        self.lib.opus_stats(self.pointer, out)
        return dict(uncompiled_days=out[0], fallback_days=out[1], invalid_intents=out[2], compile_ms_max=out[3])


def play(opponent_name, seed, seat):
    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        from kaggle_environments import make
        import kaggle_environments.envs.kaggriculture.kaggriculture as engine
        env = make("kaggriculture", configuration={"episodeSteps": 720, "seed": seed}, debug=False)
        env.reset(num_agents=2)
        sys.path.insert(0, str(LB / "src"))
        from lb.validate import load_agent_callable
        if opponent_name.startswith("extra:"):  # data/extra_opponents/<name>: Kaggle rule, last top-level function
            main_py = ROOT / "data/extra_opponents" / opponent_name[6:] / "main.py"
            entry = [n.name for n in ast.parse(main_py.read_text()).body if isinstance(n, ast.FunctionDef)][-1]
        else:
            main_py, entry = LB / "agents" / opponent_name / "main.py", "agent"
        opponent = load_agent_callable(main_py, module_label=opponent_name.replace(":", "_"), entry_callable=entry)
    policy = Native(env.configuration, seat)
    times = []

    def ours(o, configuration):
        started = time.perf_counter()
        action = policy.act(o, configuration)
        times.append(time.perf_counter() - started)
        return action

    agents = (ours, opponent) if seat == 0 else (opponent, ours)
    dawns = []
    hours = []  # LB_ACTIONS=1: per step both players' market orders, prices before the step and money

    def farm_summary(farm):
        plants, animals = {}, {}
        for row in farm["tiles"]:
            for tile in row:
                if isinstance(tile, dict) and tile.get("kind") == "PLANT":
                    plants[tile["crop"]] = plants.get(tile["crop"], 0) + 1
                elif isinstance(tile, dict) and "animal" in tile:
                    animals[tile["animal"]] = animals.get(tile["animal"], 0) + 1
        return dict(money=farm["money"], land=len(farm["unlocked_quadrants"]), workers=1 + len(farm["hands"]),
                    plants=plants, animals=animals)
    started = time.perf_counter()
    state = env.state
    for player in (0, 1):
        state[player].observation.step = 0
    for step in range(1, 720):
        observations = [copy.deepcopy(s.observation) for s in state]  # framework-style isolation
        for player in (0, 1):
            with contextlib.redirect_stdout(io.StringIO()):
                state[player].action = agents[player](observations[player], env.configuration)
        if observations[seat]["hour"] == 0:
            farms = observations[seat]["farms"]
            dawns.append(dict(day=observations[seat]["day"], ours=farm_summary(farms[seat]),
                              rival=farm_summary(farms[seat ^ 1]), plan=policy.last_day(),
                              shops=list(observations[seat]["town"]["unlocked_shops"]),
                              prices=dict(observations[seat]["market"]["prices"])))
        if os.environ.get("LB_ACTIONS"):
            hours.append(dict(step=step, prices=dict(observations[0]["market"]["prices"]),
                              market=[state[p].action.get("market", []) for p in (0, 1)],
                              money=[f["money"] for f in observations[0]["farms"]]))
        state = engine.interpreter(state, env)
        for player in (0, 1):
            state[player].observation.step = step
        if state[0].status == "DONE":
            break
    money = [float(f["money"]) for f in state[0].observation["farms"]]
    return dict(opponent=opponent_name, seed=seed, seat=seat, money=money, margin=money[seat] - money[seat ^ 1],
                statuses=[s.status for s in state], seconds=time.perf_counter() - started,
                max_action_seconds=max(times), mean_action_seconds=sum(times) / len(times), dawns=dawns, **policy.stats(),
                **(dict(hours=hours) if hours else {}))


def job(args):
    out_dir, opponent, seed, seat = args
    path = out_dir / f"{opponent}_{seed}_seat{seat}.json"
    if path.exists():
        return json.loads(path.read_text())
    if os.environ.get("LB_DUMP"):  # input buffers for build/lb_replay
        os.environ["OPUS_DUMP"] = str(out_dir / f"{opponent}_{seed}_seat{seat}.bin")
    try:
        result = play(opponent, seed, seat)
    except Exception:
        result = dict(opponent=opponent, seed=seed, seat=seat, error=traceback.format_exc())
    path.write_text(json.dumps(result, indent=1))
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("out_dir", type=Path)
    parser.add_argument("opponents", nargs="+")
    parser.add_argument("--seeds", default="700-715")
    parser.add_argument("--procs", type=int, default=24)
    args = parser.parse_args()
    first, last = map(int, args.seeds.split("-"))
    args.out_dir.mkdir(parents=True, exist_ok=True)
    jobs = [(args.out_dir.resolve(), o, s, k) for o in args.opponents for s in range(first, last + 1) for k in (0, 1)]
    results = []
    with multiprocessing.get_context("fork").Pool(args.procs, maxtasksperchild=1) as pool:
        for r in pool.imap_unordered(job, jobs):
            results.append(r)
            if "error" in r:
                print(f"error {r['opponent']} {r['seed']} seat{r['seat']}: {r['error'].splitlines()[-1]}", flush=True)
    for opponent in args.opponents:
        rs = [r for r in results if r["opponent"] == opponent and "error" not in r]
        if not rs:
            continue
        wins = sum(r["margin"] > 0 for r in rs)
        print(f"{opponent}: games {len(rs)} wins {wins} mean margin {sum(r['margin'] for r in rs) / len(rs):.1f} "
              f"uncompiled/game {sum(r['uncompiled_days'] for r in rs) / len(rs):.2f} "
              f"max action {max(r['max_action_seconds'] for r in rs):.2f}s", flush=True)


if __name__ == "__main__":
    main()
