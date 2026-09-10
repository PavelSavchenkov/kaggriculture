# arena — offline evaluation calibrated against LocalLB

`~/Kaggle/kaggriculture-localLB` is the calibration target for every offline
claim the strategy pipeline makes. This is a faster runner for the same
protocol, and evidence that it measures the same thing.

## Calibration

A clean round-robin over the 16 ranked agents against LocalLB's published table:

**Spearman 0.982**, 7/16 exact ranks, max shift 2 positions, 0 forfeits.

The residual disagreement is LocalLB's, not ours: its published ratings
accumulated from incremental challenger runs with uneven game counts
(600 / 678 / 198), so Elo there is path-dependent. Ours is what its own
`rebuild` would produce.

Underneath that, game outcomes are identical *by construction*:

```bash
python3 arena/fastmatch.py ~/Kaggle/kaggriculture-localLB \
        pavel-empty-sale-slots-m2 pavel-observed-sale-lead-216 1000,1001,1002
```

requires identical terminal money from both paths. 15/15 across the strongest
agents. Any divergence is a bug here, not an acceptable approximation.

## Speed, honestly

Full 16-agent round-robin, 4,800 games: **184 s = 26.1 games/s**, against
LocalLB's 0.34. A full rebuild is **3 minutes instead of ~4 hours**; a challenger
evaluation is **23 seconds instead of 29 minutes**.

Quote the full round-robin number, not a subset: games against *weak* opponents
are the expensive ones, because the strong agent grows a far bigger farm
(148k vs ~100k in peer games) and searches more per turn.

## Where the speed comes from

Two independent fixes, both of which had to be found by measurement:

1. **The deepcopy.** LocalLB spends 2.94 s/game of which the interpreter is
   ~51 ms. Almost all the rest is `Environment.__get_shared_state` deepcopying
   the whole state for every agent on every step - 1,440 deep copies of a
   200-tile structure per game. Worth ~7x on its own.

2. **Idle spinner threads.** `kaggle_environments` transitively loads an OpenMP
   runtime that starts one spin-waiting thread per CPU in the affinity mask, at
   import, before any agent exists. They do no work - a game takes 0.46 s with
   32 of them and 0.46 s with one - but they burn CPU, and under fork the
   children inherit the parent's pool size, so workers pinned to 4 cores still
   spawned 32 spinners apiece. Setting `OMP_NUM_THREADS=1` and friends before
   the first import is worth another ~20x under parallelism.

I initially misdiagnosed (2) as "the agents are internally parallel". They are
not: the agent `.so` files contain no `pthread_create`, and a game takes the
same 0.46 s and returns identical results at 1, 4 or 32 cores.

## Reproducibility

Two runs of an identical configuration must agree on every game. They did not:
29 of 4800 differed (0.6%), all involving `pavel-bohann-opening-v1`, and that
was enough to move it a rank. The pair reproduces perfectly in isolation - fresh
processes, 1/4/32 cores, ASLR off, under load, after warm-up - so the carryover
lives in a reused Pool worker. `maxtasksperchild=1` gives every pair a clean
address space and makes runs bit-identical (0 of 4800 differ) at no measurable
throughput cost.

The runner records every game and its forfeit count so this can be re-checked:

```bash
python3 -c "import json;a={tuple(r[:4]):r[4] for r in json.load(open('arena/out/c1.json'))['results']};\
b={tuple(r[:4]):r[4] for r in json.load(open('arena/out/c2.json'))['results']};\
print(sum(a[k]!=b[k] for k in a),'of',len(a),'differ')"
```

## Usage

```bash
python3 arena/arena.py --procs 8 --cores-per-worker 4      # full round robin
python3 arena/arena.py --agents a b c --seeds-per-pair 20  # a subset
```

Protocol matches LocalLB exactly: pairs in `itertools.combinations` order over
sorted ids, lexicographically smaller id as challenger, seeds
`1000 + pair_index*1000 + k` for k in 0..19, seat swap, Elo K=32 scale=400
initial=1500 applied in (opponent, seed, seat) order.

## Two traps

Both of these silently corrupt offline results and both cost me real time:

- `observation.step` is delivered to **both** seats at runtime even though the
  serialized replay omits seat 1's copy, and several agents key their entire
  policy off it. Omitting it produced a constant 1,416 for every seed.
- Profiling an agent against a PASS opponent measures nothing: no search runs,
  so the agent looks single-threaded and cheap. It is neither.
