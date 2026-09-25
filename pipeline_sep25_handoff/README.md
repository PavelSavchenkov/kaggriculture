# Sep 25 BC + day-compiler pipeline: handoff

This folder hands over the work of `experiments/v10/sep24_BC_opus` (Sep 24 22:10 to Sep 25 ~20:00 BST,
not committed) so the team can reproduce it, learn from it and improve it. It holds the complete code,
the weights of the best agent, every candidate configuration, the evidence behind each decision,
traces, the weakness analysis and a plan. It depends only on files already committed to this
repository (`fast_game_engine/`, `agents/common/`, `agents/external/*`, `agents/inhouse/*`,
`agent_sep23/`) and on public or team data sources named in [TRAINING.md](TRAINING.md).

## The agent in one paragraph

Every dawn a behaviour-cloned (BC) network reads the game state and emits one **DayIntent**: how
many of each crop and animal to add, whether to buy land, and for every group of identical crops or
animals how many to water, fertilize, harvest, clear, feed, care for and collect. A deterministic
C++ **day compiler** turns that intent into every hourly worker action and market order: tiles,
routes, hires, purchases, shed returns, and an hourly sale plan against a forecast of the opponent.
The network never picks tiles, paths or prices; the compiler never decides what the farm should
grow. The network is `v12_cond` (2.3M parameters, trained on 1.45M days of top Kaggle players,
conditioned on player strength). Most of the Sep 25 gains came from the compiler and from
inference settings, not from new networks.

## Headline results (best agent = `forecast`)

| Test | Result |
|---|---|
| Local-LB (team league), exact replay of the current 10-agent roster's games | 137/140 wins, +$13.8k per game (3 losses, all vs our own `slots`) |
| Every agent in the Local-LB repository (69 agents, 7 seeds x 2 seats each) | 962/966 wins (losses: `slots` 2, `wages` 1, JJ's `ppo-v6n-it60` 1) |
| Head to head vs `wages` (previous best, C++ mirror, 5 seed sets) | 164-36 (82%), +$3.2k per game |
| 7 C++ opponents (224 games) | 224/224, +$25.9k to +$73.4k per opponent |
| 145 frozen games of the Kaggle top-10 (we replace one seat) | 143/145, +$54.4k |
| Earlier lineage on the Local-LB table | `slots` #1 at 399-1-0 (rating 2802); `herd` was #1 before it, `robust` reached #2 |
| Kaggle (older `robust` agent, submission 56553038) | score 2795 after 64 games (50 wins) on Sep 25 19:35; top 10 is 2917-3065 |
| Newest candidate `z_rs` (not yet on the Local-LB) | vs `forecast`: 19-1 and 18-2 (one-seat mirror sets), C++ panel +$1.4k; fixes a herd-reach fragility of `forecast` (WEAKNESSES 0.5) |

Details and every other number: [RESULTS.md](RESULTS.md). The Kaggle games show what still loses
against top-50 players: [WEAKNESSES.md](WEAKNESSES.md).

## How the results were achieved (largest levers first)

1. **Compiler fixes found by tracing lost full games**, not by offline metrics: trimming an unfunded
   day's new crops/animals one unit at a time instead of dropping all (14/64 -> 42/64 wins vs the
   previous best BC agent); day 0 drops crops before animals (the third sheep was lost in 100% of
   games; +$6.6k, wins 86% -> 97.5% on unseen seeds); a recovery level instead of survival-only
   mode (3 farm collapses in 145 replays -> 0).
2. **Market order slots.** The engine fills both players' orders slot by slot at the same quote, so a
   sale listed earlier gets the higher price. Listing sales by revenue at stake won 39/40 mirror games
   (+$11.6k) against the old product-index order and made `slots` Local-LB #1 at 399-1-0.
3. **Output design of the network**: 101-way count categoricals instead of rounded per-member
   probabilities, marginal count heads decoded by exact MAP dynamic programming, whole-farm totals
   decoded as total x type shares. Against the previous best BC agent (`agent_sep23`), v1 won 0/32
   and v9 64/64; the trim fix above and fresh data are part of that jump, and a teacher-forced decode
   check isolated each output bug.
4. **13x more data with strength/recency conditioning instead of filtering** (v12): Local-LB 79% ->
   88% wins; filtering to the strongest players lost $11.3k per game; 10k instead of 40k steps lost
   $4.8k.
5. **Opening style pin**: days 0-5 decoded in one strong team's style (DSM), pooled style after:
   82-38 vs the Vadim pin, which itself was +5.7 win points over no pin.
6. **Opponent forecast from the opponent's visible farm** (ripe crops, held animal products), not only
   its trailing sales: 101-19 in the mirror; collecting every animal's fertilizer 88-32; wage-aware
   same-day returns +$0.9k; larger herds on fully funded days 6-14 86-28.
7. **Evaluation discipline**: exact replay of the Local-LB's own seeds (predicted every merge result
   exactly, e.g. 399/400 for `slots`), unseen seeds for selection, >= 4 seed sets before adopting a
   change (single 40-game sets produced false winners).

The full story with evidence and rejected ideas: [LINEAGE.md](LINEAGE.md).

## Reading order

1. This README, then [RESULTS.md](RESULTS.md) and [LINEAGE.md](LINEAGE.md).
2. [DESIGN.md](DESIGN.md): the philosophy of the DayIntent + day-compiler split, the design contracts
   (`docs/design/`), the exact C++/Python API, and where the implementation differs from the design.
3. [ARCHITECTURE.md](ARCHITECTURE.md): how the network, decoding, compiler and executor work; every
   sidecar and option with its evidence.
4. [EVALUATION.md](EVALUATION.md): the validation pipeline (self-play mirror, local C++ league, zoo
   league, Local-LB panels and exact replay, frozen Kaggle replays, our Kaggle games), the promotion rule
   and the traps. Read it before running any comparison.
5. [WEAKNESSES.md](WEAKNESSES.md) and [PLAN.md](PLAN.md): what to work on next.
6. [TRAINING.md](TRAINING.md) to retrain or fine-tune the network.
7. [LEARNINGS.md](LEARNINGS.md): what worked and what failed, in short.
8. `docs/`: design contracts, the original experiment logs, the companion weakness study.

## Layout

```
README.md, RESULTS.md, LINEAGE.md, DESIGN.md, ARCHITECTURE.md, EVALUATION.md, TRAINING.md,
LEARNINGS.md, WEAKNESSES.md, PLAN.md
setup_experiment.sh   copy experiment/ + weights + candidates into a new experiments/<v>/<name>/
agent/                the best agent (forecast): main.py, sidecars, package.sh, play.py,
                      reproduce_validation.py
candidates/           sidecar sets of every lineage agent and of the untested-at-scale candidates
weights/v12_cond/     the network: model.bin (C++), model.pt (PyTorch), training history and log
experiment/           full source snapshot: compiler, agent, tools, scripts, tests, small data
analysis/             ledger/audit tools and Kaggle-replay pipeline of the companion weakness study
evidence/             every game result of the experiment as merged CSVs + summary.csv
traces/               traced games of the best agent, its exact Local-LB replay, the frozen
                      top-10 replay bed (145 games) and our 64 Kaggle games as a frozen bed
docs/                 design contracts, original PROGRESS/IDEAS/HANDOFF logs, weakness study
MANIFEST.sha256       hashes of every file in this folder
```

## Quickstart

All commands from the repository root, Linux x86-64, conda env `kaggriculture` (Python with
PyTorch and `kaggle-environments==1.32.7`, CMake, a C++20 compiler).

```bash
# 1. Build in place (1 minute with -j 8) and run the unit tests
conda run -n kaggriculture cmake -S pipeline_sep25_handoff/experiment -B work/p25_build -DCMAKE_BUILD_TYPE=Release
conda run -n kaggriculture cmake --build work/p25_build -j 8
conda run -n kaggriculture ctest --test-dir work/p25_build

# 2. Full C++ games of the best agent vs a committed opponent (seed 700, both seats)
mkdir -p work/p25_model && cp pipeline_sep25_handoff/weights/v12_cond/model.bin pipeline_sep25_handoff/agent/model/* work/p25_model/
BC_OPUS_MODEL=$PWD/work/p25_model/model.bin work/p25_build/full_games king_rc4 700 1 2 work/p25_games.csv
# expected: 700,0,115310,79848,35462 and 700,1,115310,79848,35462 (bit-exact with the recorded panel)

# 3. A working experiment folder (scripts expect experiments/<v>/<name>/ depth, models/, data/)
pipeline_sep25_handoff/setup_experiment.sh experiments/v11/sep26_myidea

# 4. A submission folder + Kaggle archive (needs conda env kagbuild, see agent/README.md)
pipeline_sep25_handoff/agent/package.sh work/my_submission
conda run -n kaggriculture python pipeline_sep25_handoff/agent/play.py work/my_submission starter --seeds 1-2
```

## Snapshot and provenance

- Code snapshot taken Sep 25 18:55 UTC (19:55 BST) from `experiments/v10/sep24_BC_opus` while that
  session was still running (it was screening 4th-quadrant probes). Verified from this folder: the build
  reproduces the recorded games of `forecast`, `anticipate`, `z_rs`, `wages`, `slots`, `robust`, `herd`
  and `vadim` bit-exactly (C++ panel, mirror, one-seat screen and Python Local-LB harness); the frozen
  Kaggle beds reproduce the recorded replay gate; the packaged agent plays the same actions as the
  shipped `forecast` bridge in kaggle-environments 1.32.7 and reproduces Kaggle's validation episode.
- Logs and evidence were refreshed at ~20:05 BST; they may mention later work of that session.
- The companion analysis (`work/sep25_bc_weakness`, another session on the same agents) is in
  `docs/weakness_analysis/` and `analysis/`, copied at ~20:05 BST (its 24-hour cycle log `CYCLES.md` had
  just started).
- Not included (size): replay traces (5-7 GB), extracted training arrays (22-26 GB per version), the
  Local-LB snapshot (658 MB), the 150+ experimental model folders. [TRAINING.md](TRAINING.md) and
  [EVALUATION.md](EVALUATION.md) say where each comes from. The team's private Kaggle dataset
  `pavelsavchenkov/kaggriculture-agent-zoo-sep25` holds the training traces (539 MB compressed) and
  the six top-team clone networks used as sparring partners.
- Names: agents are named after their Local-LB id `pavel-bc-opus-v12-<name>`; `fin_*` / `cand_*` are
  the experiment's model-folder names (mapping in [LINEAGE.md](LINEAGE.md)).
