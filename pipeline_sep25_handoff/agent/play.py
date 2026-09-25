"""Full Kaggriculture games between two agents in the official environment (kaggle-environments
1.32.7). Agent folders are loaded the way Kaggle loads a submission: main.py is exec()'d without
__file__ and its last callable is the agent.

    python tools/play.py AGENT_A AGENT_B [--seeds 1300-1303] [--both-seats] [--procs 4] [--out games.jsonl]

AGENT is an agent folder (e.g. agents/bc-v12-slots) or a built-in agent name (e.g. random).
Each game prints one JSON line: seed, agent per seat, rewards, statuses, the slowest call and the
overage seconds left per seat, and a SHA-256 of each seat's 720 actions (equal hashes = identical
play, e.g. to check a rebuilt bridge against the shipped one). The bc agents switch to a faster
search when less than 25 s of overage remains, so compare games on an idle machine.
"""
import argparse
import hashlib
import json
import multiprocessing
import time
from pathlib import Path

import kaggle_environments
from kaggle_environments import make
from kaggle_environments.agent import get_last_callable


def load(agent, times):
    path = Path(agent) / "main.py"
    if not path.is_file():
        return agent  # built-in agent name
    function = get_last_callable(path.read_text(), path=str(path))

    def timed(observation, configuration):
        start = time.perf_counter()
        action = function(observation, configuration)
        times.append(time.perf_counter() - start)
        return action

    return timed


def play(task):
    seed, agents = task
    times = [[], []]
    environment = make("kaggriculture", configuration={"seed": seed}, debug=True)
    start = time.monotonic()
    environment.run([load(agent, times[seat]) for seat, agent in enumerate(agents)])
    final = environment.steps[-1]
    actions = [[step[seat].action for step in environment.steps] for seat in range(2)]
    return {
        "seed": seed, "agents": list(agents),
        "rewards": [state.reward for state in final],
        "statuses": [str(state.status) for state in final],
        "max_call_seconds": [round(max(t), 3) if t else None for t in times],
        "overage_left": [state.observation.get("remainingOverageTime") for state in final],
        "action_sha256": [hashlib.sha256(json.dumps(a, sort_keys=True).encode()).hexdigest()[:16] for a in actions],
        "game_seconds": round(time.monotonic() - start, 1),
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("agent_a")
    parser.add_argument("agent_b")
    parser.add_argument("--seeds", default="1300-1300", help="inclusive range, e.g. 1300-1303")
    parser.add_argument("--both-seats", action="store_true", help="also play each seed with seats swapped")
    parser.add_argument("--procs", type=int, default=1)
    parser.add_argument("--out", type=Path, help="append results as JSON lines")
    args = parser.parse_args()
    if kaggle_environments.__version__ != "1.32.7":
        raise SystemExit(f"kaggle-environments {kaggle_environments.__version__}: the competition uses 1.32.7")
    first, last = map(int, args.seeds.split("-"))
    tasks = [(seed, (args.agent_a, args.agent_b)) for seed in range(first, last + 1)]
    if args.both_seats:
        tasks += [(seed, (args.agent_b, args.agent_a)) for seed in range(first, last + 1)]
    # One fresh process per game: every game loads its agents (and their C++ state) from scratch.
    with multiprocessing.get_context("spawn").Pool(args.procs, maxtasksperchild=1) as pool:
        for result in pool.imap_unordered(play, tasks):
            line = json.dumps(result)
            print(line, flush=True)
            if args.out:
                with args.out.open("a") as f:
                    f.write(line + "\n")


if __name__ == "__main__":
    main()
