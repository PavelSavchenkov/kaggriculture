"""Fast, LocalLB-protocol-compatible match runner.

LocalLB runs matches through `env.run`, which costs ~2.94 s per episode. Almost
none of that is the game: the engine interpreter itself is ~51 ms. The rest is
framework overhead, dominated by `Environment.__get_shared_state`, which does a
`copy.deepcopy` of the whole state for every agent on every step - 1,440 deep
copies of a 200-tile nested structure per episode.

This runner calls the official interpreter directly and hands each agent the
observation the interpreter already maintains for it. Two properties make that
safe, and both were checked against the LocalLB agent set rather than assumed:

  * no agent mutates the observation it is given, so the defensive deepcopy is
    pure overhead; and
  * no agent reads `remainingOverageTime`, which the framework decrements from
    measured wall-clock time and which is therefore not reproducible anyway.

Correctness is not argued, it is tested: `verify` replays the same seeds through
both paths and requires identical terminal money. Any divergence is a bug here,
not an acceptable approximation.
"""
from __future__ import annotations

import os as _os

# kaggle_environments transitively loads an OpenMP runtime that starts one
# spin-waiting thread per CPU in the affinity mask, at import, before any agent
# exists. Those threads do no work: a game takes 0.46 s with 32 of them and
# 0.46 s with one. They only steal CPU - and under fork, children inherit the
# parent's pool size, so 8 workers pinned to 4 cores each still spawned 32
# spinners apiece and thrashed. Pinning one core with the pool at 32 made a
# single game 14x slower (6.22 s vs 0.46 s).
# Must be set before the first import of anything that pulls the runtime in.
for _v in ("OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS", "MKL_NUM_THREADS",
           "NUMEXPR_NUM_THREADS", "VECLIB_MAXIMUM_THREADS"):
    _os.environ.setdefault(_v, "1")


import contextlib
import io
import sys
from pathlib import Path
from typing import Any, Callable

from kaggle_environments import make
import kaggle_environments.envs.kaggriculture.kaggriculture as engine

DEFAULT_STEPS = 720


def play(agent0: Callable, agent1: Callable, seed: int,
         episode_steps: int = DEFAULT_STEPS) -> list[float]:
    """Run one episode and return terminal money per seat."""
    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        env = make("kaggriculture",
                   configuration={"episodeSteps": int(episode_steps), "seed": int(seed)},
                   debug=False)
        env.reset(num_agents=2)
        state = env.state
        agents = (agent0, agent1)
        cfg = env.configuration
        for index in range(1, int(episode_steps)):
            for seat in (0, 1):
                state[seat].action = agents[seat](state[seat].observation, cfg)
            state = engine.interpreter(state, env)
            # The framework owns the step counter; the interpreter reads it but
            # never advances it. Both seats get it at runtime - the serialized
            # replay omits seat 1's copy, but a live agent does receive it, and
            # several LocalLB agents key their whole policy off it. Verified by
            # instrumenting env.run rather than read off the replay format.
            for seat in (0, 1):
                state[seat].observation.step = index
            if state[0].status == "DONE":
                break
    farms = state[0].observation["farms"]
    return [float(farms[0]["money"]), float(farms[1]["money"])]


def result(money: list[float], challenger_seat: int) -> str:
    if money[0] == money[1]:
        return "draw"
    return "win" if (0 if money[0] > money[1] else 1) == challenger_seat else "loss"


def _load_localbb_agent(lb_root: Path, agent_id: str) -> Callable:
    sys.path.insert(0, str(lb_root / "src"))
    from lb.config import load_config
    from lb.match import load_agent
    cwd = Path.cwd()
    import os
    os.chdir(lb_root)
    try:
        return load_agent("agents", agent_id, load_config())
    finally:
        os.chdir(cwd)


def verify(lb_root: Path, a: str, b: str, seeds: list[int]) -> int:
    """Require identical terminal money from the fast path and LocalLB's own."""
    sys.path.insert(0, str(lb_root / "src"))
    from lb.config import load_config
    from lb.match import run_single_game
    import os
    import time
    cfg = load_config.__wrapped__() if hasattr(load_config, "__wrapped__") else None
    cwd = Path.cwd()
    os.chdir(lb_root)
    try:
        from lb.config import load_config as lc
        cfg = lc()
        from lb.match import load_agent
        fa, fb = load_agent("agents", a, cfg), load_agent("agents", b, cfg)
        bad = 0
        print(f"{'seed':>6} {'LocalLB money':>28} {'fast money':>28} {'ref s':>7} {'fast s':>7}  match")
        for seed in seeds:
            t0 = time.time()
            ref = run_single_game(fa, fb, config=cfg, seed=seed, challenger_seat=0)
            t1 = time.time()
            fast = play(fa, fb, seed)
            t2 = time.time()
            same = [float(x) for x in ref["money"]] == fast
            bad += 0 if same else 1
            print(f"{seed:>6} {str(ref['money']):>28} {str(fast):>28} "
                  f"{t1-t0:>7.2f} {t2-t1:>7.2f}  {'OK' if same else 'DIFFER'}")
        return bad
    finally:
        os.chdir(cwd)


if __name__ == "__main__":
    lb = Path(sys.argv[1] if len(sys.argv) > 1 else Path.home() / "Kaggle/kaggriculture-localLB")
    a = sys.argv[2] if len(sys.argv) > 2 else "baseline-random"
    b = sys.argv[3] if len(sys.argv) > 3 else "pavel-sixday-0"
    seeds = [int(x) for x in sys.argv[4].split(",")] if len(sys.argv) > 4 else [1000, 1001, 1002]
    bad = verify(lb, a, b, seeds)
    print(f"\n{len(seeds) - bad}/{len(seeds)} identical")
    sys.exit(1 if bad else 0)
