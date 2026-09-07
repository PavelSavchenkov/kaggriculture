"""Benchmark the official Python environment on a recorded C++ benchmark trace."""
import contextlib
import io
from pathlib import Path
import statistics
import sys
import time

from kaggle_environments import make
from kaggle_environments.envs.kaggriculture import kaggriculture as engine


OPS = ["PASS", "NORTH", "SOUTH", "EAST", "WEST", "PICKUP", "DROP", "PLACE",
       "PLANT", "WATER", "HARVEST", "FERTILIZE", "DIG", "BUILD_COOP",
       "BUILD_PASTURE", "FEED", "COLLECT_FERTILIZER", "CARE"]
ITEMS = ["WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK",
         "WOOL", "FERTILIZER", "GOOSE", "COW", "SHEEP"]
MOPS = ["NONE", "HIRE", "BUY_LAND", "BUY_SEED", "BUY_PRODUCT", "BUY_ANIMAL", "SELL"]
CONFIG_KEYS = ["episodeSteps", "boardSize", "startingMoney", "maxMarketOrdersPerTurn",
               "turnsPerDay", "shedCapacity", "weedSpawnChance", "townShopUnlockInterval",
               "townShopSellInterval", "townCenterSellInterval", "farmHandCostMult"]


def decode_unit(op, item, count):
    name = OPS[op]
    if name in {"PICKUP", "PLACE"}:
        return [name, ITEMS[item], count]
    if name == "PLANT":
        return [name, ITEMS[item]]
    return [name]


def decode_order(op, item, count):
    name = MOPS[op]
    if name in {"HIRE", "BUY_LAND"}:
        return [name]
    return [name, ITEMS[item], count]


def load_trace(path):
    lines = Path(path).read_text().splitlines()
    seed, turns = (int(value) for value in lines[0].split())
    config_values = lines[1].split()[1:]
    config = dict(zip(CONFIG_KEYS, (float(value) if "." in value else int(value)
                                    for value in config_values)))
    cursor = 2 + int(lines[2].startswith("ENGINE "))
    actions = [[], []]
    for turn in range(turns):
        for player in range(2):
            values = [int(value) for value in lines[cursor + 2 * turn + player].split()]
            n_units, n_orders = values[:2]
            offset = 2
            units = []
            for _ in range(n_units):
                units.append(decode_unit(*values[offset:offset + 3]))
                offset += 3
            orders = []
            for _ in range(n_orders):
                orders.append(decode_order(*values[offset:offset + 3]))
                offset += 3
            actions[player].append({"farmer": units[0], "hands": units[1:], "market": orders})
    return seed, config, actions


def replay_agent(actions):
    def agent(observation, configuration):
        return actions[observation.step]
    return agent


def framework_episode(config, agents, actions):
    env = make("kaggriculture", configuration=config, debug=False)
    env.run(agents)
    return int(env.steps[-1][0].reward)


def interpreter_episode(config, agents, actions):
    env = make("kaggriculture", configuration=config, debug=False)
    state = env.state
    for step in range(len(actions[0])):
        for player in range(2):
            state[player].action = actions[player][step]
            state[player].observation.step = step
        state = engine.interpreter(state, env)
    return int(state[0].reward)


def benchmark(name, runner, reps, rounds, seed, base_config, agents, actions):
    timings = []
    sink = 0
    for round_index in range(rounds):
        started = time.perf_counter()
        for repetition in range(reps):
            config = dict(base_config, seed=seed + round_index * reps + repetition)
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                sink += runner(config, agents, actions)
        timings.append(time.perf_counter() - started)
    median = statistics.median(timings)
    best = min(timings)
    print(f"{name}: {reps} episodes x {rounds}: median {reps / median:.2f} eps/s "
          f"({median / reps * 1000:.1f} ms/ep), best {reps / best:.2f} eps/s [sink {sink}]")


def main():
    path = sys.argv[1]
    reps = int(sys.argv[2]) if len(sys.argv) > 2 else 3
    rounds = int(sys.argv[3]) if len(sys.argv) > 3 else 3
    seed, base_config, actions = load_trace(path)
    agents = [replay_agent(player_actions) for player_actions in actions]
    benchmark("interpreter only", interpreter_episode, reps, rounds,
              seed, base_config, agents, actions)
    benchmark("full framework", framework_episode, reps, rounds,
              seed, base_config, agents, actions)


if __name__ == "__main__":
    main()
