"""Games through the Local-LB's own runner (kaggle-environments env.run with actTimeout 1 s and
the overage accounting), with a packaged agent directory, to see how much overage our agent
uses under load and whether the time-pressure valve changes results.

For each game writes <out>/<opponent>_<seed>_seat<k>.json: margin, statuses and our remaining
overage at every dawn (hour 0) and at the end.
usage: lb_runner_games.py <agents_root> <agent_id> <out> <opponent> <first_seed> <n_seeds> [procs]
"""
import json
import multiprocessing
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "data/localLB_main/src"))
from lb.config import load_config  # noqa: E402
from lb.match import load_agent  # noqa: E402


def play(job):
    agents_root, agent_id, out, opponent, seed, seat = job
    path = Path(out) / f"{opponent}_{seed}_seat{seat}.json"
    if path.exists():
        return
    from kaggle_environments import make
    config = load_config(ROOT / "data/localLB_main/config/leaderboard.yaml")
    me = load_agent(agents_root, agent_id, config)
    rival = load_agent(ROOT / "data/localLB_main/agents", opponent, config)
    env = make("kaggriculture", configuration={"episodeSteps": 720, "seed": seed, "actTimeout": 1}, debug=True)
    start = time.time()
    env.run([me, rival] if seat == 0 else [rival, me])
    last = env.steps[-1]
    money = [last[0].observation["farms"][p]["money"] for p in range(2)]
    overage = [step[seat].observation.get("remainingOverageTime") for step in env.steps]
    path.write_text(json.dumps({
        "opponent": opponent, "seed": seed, "seat": seat, "money": money, "margin": money[seat] - money[1 - seat],
        "statuses": [s.status for s in last], "seconds": time.time() - start,
        "dawn_overage": overage[::24], "final_overage": overage[-1]}))


def main():
    agents_root, agent_id, out, opponent, first, count = sys.argv[1:7]
    procs = int(sys.argv[7]) if len(sys.argv) > 7 else 20
    Path(out).mkdir(parents=True, exist_ok=True)
    jobs = [(agents_root, agent_id, out, opponent, s, seat) for s in range(int(first), int(first) + int(count)) for seat in (0, 1)]
    with multiprocessing.Pool(procs) as pool:
        pool.map(play, jobs, chunksize=1)


if __name__ == "__main__":
    main()
