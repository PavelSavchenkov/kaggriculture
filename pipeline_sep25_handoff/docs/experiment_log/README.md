# sep24_BC_opus: BC DayIntent policy + day compiler

A behavior-cloning policy that emits one DayIntent per dawn
(designs/day_intent.md), executed by the day compiler (designs/day_compiler.md)
copied from experiments/v10/sep24_day_compiler_opus (hashes in
data/COMPILER_SOURCE.sha256) with one new fallback (trim new entities).

Details: PROGRESS.md (timeline), LEARNINGS.md (what worked and what did not).
A self-contained write-up for outside advice: `BC_issues_sep25.md` in the repo root.

## Layout

- `source/`: compiler copy, `features.hpp` (actor inputs, shared by extraction and
  agent), `dataset.hpp` (targets and fixed-value masks).
- `agent/bc_opus/`: local agent (C++ inference, decoding, compiler, executor).
- `tools/extract.cpp`: dataset generation = interface-coverage gate.
- `tools/continuation.cpp`, `tools/full_games.cpp`, `tools/decode_eval.cpp`,
  `tools/parity.cpp`: evaluation (traces: BC_TRACE, BC_HOUR_TRACE, BC_GAME_TRACE).
- `scripts/train.py` (PyTorch, strict FP32), `scripts/parity.py`,
  `scripts/collect_replays.py` (fresh Kaggle replays), `scripts/build_corpus.py`.
- `opponents/`: agent_sep23 and in-house league agents behind isolated adapters.
- `models/`: all trained versions; `models/selected/model.bin` = v9.

## Selected model (v9)

Width 256 MLP encoders (global, crop groups, animal groups), mean/max/sum pooling,
10x10 tile CNN for both farms, context MLP; whole-farm counts as 101-way
categoricals plus total x type shares; group fields as independent 101-way count
heads decoded by exact MAP DP under the partition constraints; group heads see
the decoded whole-farm fields. Trained on 3,021 top-20 perspectives (~90k dawns,
including 1,221 fresh perspectives downloaded Sep 24), 12k steps.

## Results (main gate: full games)

| Opponent | Seeds | Wins | Mean margin |
|---|---|---|---|
| agent_sep23 (previous strongest BC agent) | 700-731, both seats (fresh) | 61/64 | +19,216 |
| agent_sep23 | 600-631 (used for selection) | 64/64 | +20,585 |
| two_random_shop_league_v179 | 700-731 | 62/64 | +49,948 |
| one_shop_no_geese_league_winner_v1 | 700-731 | 64/64 | +75,934 |
| PASS | 900-901 | 4/4 | +168,672 |

Replay continuations on the untouched test split (348 perspectives; fixed
opponent, diagnostic): -2,342 from day 20, -1,839 from day 25, -223 on day 29.

Gates: dataset coverage passes (112,017 of 112,020 perspective-days represented,
3 ignored for a discarded animal, 0 failures); native inference matches PyTorch
(100/100 identical decisions per checked model); full games pass against local
opponents; not yet run against Local-LB or public agents. Compile latency tail
up to about 3.7 s on a heavy day.

Build: `conda run -n kaggriculture cmake -S . -B build && conda run -n kaggriculture cmake --build build -j 28`.
