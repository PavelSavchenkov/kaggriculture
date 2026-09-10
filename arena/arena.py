"""Fast offline leaderboard: LocalLB's protocol, ~100x its throughput.

LocalLB is the calibration target for every offline claim, but at ~3 s per game
a single challenger evaluation costs ~29 minutes and a full rebuild ~4 hours.
That is too slow to search over hypotheses, which is what the strategy pipeline
needs it for.

This runner reproduces LocalLB's protocol exactly - same pairing order, same
seeds, same seat swap, same Elo - while replacing `env.run` with a direct
interpreter loop (see fastmatch.py, which verifies identical terminal money) and
running pairs across processes.

Correctness rests on game outcomes being identical, not similar. Verify first:

    python3 arena/fastmatch.py <lb_root> <agent_a> <agent_b> 1000,1001,1002
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


import argparse
import itertools
import json
import os
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fastmatch                                              # noqa: E402

_CACHE: dict[str, object] = {}
_LB_ROOT: Path | None = None
_STEPS = 720


def _agent(agent_id: str):
    if agent_id not in _CACHE:
        sys.path.insert(0, str(_LB_ROOT / "src"))
        from lb.config import load_config
        from lb.match import load_agent
        cwd = Path.cwd()
        os.chdir(_LB_ROOT)
        try:
            _CACHE[agent_id] = load_agent("agents", agent_id, load_config())
        finally:
            os.chdir(cwd)
    return _CACHE[agent_id]


def _init(lb_root: str, steps: int, cores_per_worker: int, slots) -> None:
    """Pin this worker to a disjoint set of cores.

    Not because the agents are parallel - they are not; there is no
    pthread_create in either .so and they time identically on 1, 4 and 32 cores.
    The threads are OpenMP spinners that kaggle_environments' import chain
    starts, one per CPU in the affinity mask, before any agent is constructed.
    Children inherit the parent's pool size across fork, so N unpinned workers
    each spin up a full-width pool and fight over the same cores - measured at
    93 core-seconds per game against a true cost of ~6, and a single game went
    6.22 s pinned to one core with a 32-wide pool against 0.46 s otherwise.
    The OMP_NUM_THREADS block at the top of this file is the actual fix;
    disjoint pinning then keeps workers off each other's cores.
    """
    global _LB_ROOT, _STEPS
    _LB_ROOT = Path(lb_root)
    _STEPS = steps
    try:
        with slots.get_lock():
            index = slots.value
            slots.value += 1
        total = os.cpu_count() or 1
        cores = {(index * cores_per_worker + k) % total for k in range(cores_per_worker)}
        os.sched_setaffinity(0, cores)
    except (AttributeError, OSError, ValueError):
        pass


def run_pair(job) -> list[dict]:
    """All seeds and seat swaps for one pair. Mirrors lb.match.run_pair."""
    pair_index, challenger, opponent, seeds_per_pair, seed_base, seat_swap = job
    games = []
    fn_c, fn_o = _agent(challenger), _agent(opponent)
    base = seed_base + pair_index * 1000
    for seed in (base + k for k in range(seeds_per_pair)):
        for seat in ((0, 1) if seat_swap else (0,)):
            a0, a1 = (fn_c, fn_o) if seat == 0 else (fn_o, fn_c)
            try:
                money = fastmatch.play(a0, a1, seed, _STEPS)
                res = fastmatch.result(money, seat)
                forfeit = False
            except BaseException as exc:                        # noqa: BLE001
                money, res, forfeit = [None, None], "loss", True
                print(f"  forfeit {challenger} vs {opponent} seed {seed}: "
                      f"{type(exc).__name__}: {exc}", file=sys.stderr)
            games.append({"challenger": challenger, "opponent": opponent, "seed": seed,
                          "seat": seat, "result": res, "forfeit": forfeit, "money": money})
    return games


def elo(games, ids, initial=1500.0, k=32.0, scale=400.0, draw=0.5) -> dict[str, float]:
    """Replicates lb.elo.apply_games: sequential over (opponent, seed, seat)."""
    ratings = {i: float(initial) for i in ids}
    for g in sorted(games, key=lambda x: (str(x["opponent"]), int(x["seed"]), int(x["seat"]))):
        sa = draw if g["result"] == "draw" else (1.0 if g["result"] == "win" else 0.0)
        ra, rb = ratings[g["challenger"]], ratings[g["opponent"]]
        ea = 1.0 / (1.0 + 10.0 ** ((rb - ra) / scale))
        ratings[g["challenger"]] = ra + k * (sa - ea)
        ratings[g["opponent"]] = rb + k * ((1.0 - sa) - (1.0 - ea))
    return ratings


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--lb-root", default=str(Path.home() / "Kaggle/kaggriculture-localLB"))
    ap.add_argument("--agents", nargs="*", help="default: every directory under agents/")
    ap.add_argument("--seeds-per-pair", type=int, default=20)
    ap.add_argument("--seed-base", type=int, default=1000)
    ap.add_argument("--episode-steps", type=int, default=720)
    ap.add_argument("--procs", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--cores-per-worker", type=int, default=1,
                    help="disjoint cores pinned per worker; agents size their thread "
                         "pool from this, so keep procs*cores <= ncpu")
    ap.add_argument("--out", default="arena/out/rankings.json")
    args = ap.parse_args()

    lb_root = Path(args.lb_root)
    ids = sorted(args.agents) if args.agents else sorted(
        p.name for p in (lb_root / "agents").iterdir() if (p / "main.py").exists())
    pairs = list(itertools.combinations(ids, 2))
    jobs = [(i, *sorted((a, b)), args.seeds_per_pair, args.seed_base, True)
            for i, (a, b) in enumerate(pairs)]
    per_pair = args.seeds_per_pair * 2
    print(f"{len(ids)} agents, {len(pairs)} pairs, {len(pairs) * per_pair} games, "
          f"{args.procs} processes x {args.cores_per_worker} core(s) "
          f"= {args.procs * args.cores_per_worker} of {os.cpu_count()} cpus")

    import multiprocessing as mp
    t0 = time.time()
    games: list[dict] = []
    slots = mp.Value("i", 0)
    # One fresh forked worker per pair. Without this, two runs of the identical
    # configuration disagreed on 29 of 4800 games (0.6%), all involving
    # pavel-bohann-opening-v1, and the disagreement moved it a rank. The pair is
    # perfectly reproducible in isolation - fresh processes, 1/4/32 cores, under
    # load, after warm-up - so the carryover is something that survives inside a
    # reused worker. Rather than ship a known-irreproducible measurement, give
    # every pair a clean address space; the cost is one agent load per pair.
    with mp.Pool(args.procs, maxtasksperchild=1, initializer=_init,
                 initargs=(str(lb_root), args.episode_steps,
                           args.cores_per_worker, slots)) as pool:
        progress_every = max(1, len(jobs) // 20)
        for n, batch in enumerate(pool.imap_unordered(run_pair, jobs), 1):
            games.extend(batch)
            if n == 1 or n % progress_every == 0 or n == len(jobs):
                el = time.time() - t0
                print(f"  {n}/{len(jobs)} pairs  {el:.0f}s elapsed, "
                      f"{el / n * (len(jobs) - n):.0f}s left", flush=True)
    elapsed = time.time() - t0

    ratings = elo(games, ids)
    stats = {i: [0, 0, 0] for i in ids}
    for g in games:
        w, l = (0, 1) if g["result"] == "win" else ((1, 0) if g["result"] == "loss" else (2, 2))
        if g["result"] == "draw":
            stats[g["challenger"]][2] += 1
            stats[g["opponent"]][2] += 1
        else:
            stats[g["challenger"]][w] += 1
            stats[g["opponent"]][l] += 1

    forfeits = [g for g in games if g["forfeit"]]
    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    with open(args.out, "w") as f:
        json.dump({"ratings": ratings, "stats": stats, "games": len(games),
                   "forfeits": len(forfeits), "seconds": elapsed, "agents": ids,
                   "game_records": sorted(games, key=lambda g: (
                       g["challenger"], g["opponent"], g["seed"], g["seat"])),
                   # Full per-game record so two runs can be diffed directly.
                   # Results must be identical across process counts; anything
                   # else means a game silently failed or an agent is not
                   # deterministic, and the arena must not hide that.
                   "results": sorted((g["challenger"], g["opponent"], g["seed"],
                                      g["seat"], g["result"]) for g in games)},
                  f, indent=1)
    if forfeits:
        print(f"\n!! {len(forfeits)} FORFEITS - these are scored as losses and "
              f"make the run non-reproducible:")
        import collections
        for (c, o), n in collections.Counter(
                (g["challenger"], g["opponent"]) for g in forfeits).most_common(10):
            print(f"     {n:>4}  {c} vs {o}")

    print(f"\n{len(games)} games in {elapsed:.0f}s "
          f"({len(games) / elapsed:.1f} games/s; LocalLB does ~0.33)\n")
    print(f"{'rank':>4}  {'agent':<40} {'rating':>8}  {'W-L-D':>14}")
    for r, (i, v) in enumerate(sorted(ratings.items(), key=lambda kv: -kv[1]), 1):
        w, l, d = stats[i]
        print(f"{r:>4}  {i:<40} {v:>8.1f}  {f'{w}-{l}-{d}':>14}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
