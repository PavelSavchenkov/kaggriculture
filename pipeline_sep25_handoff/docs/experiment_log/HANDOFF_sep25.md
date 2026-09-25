# Handoff: sep24_BC_opus (Sep 25, ~11:00 BST)

For the next session continuing this experiment. Read AGENTS.md, prompts/local_agent.md,
prompts/experiments_pipeline.md and designs/day_intent.md first. PROGRESS.md (audits),
IDEAS_LEDGER.md and ../../../BC_issues_sep25.md hold the full history.

## Standing rules from the user

- 24 h goal (started ~02:05 Sep 25): the strongest day-compiler + BC agent against the
  Local-LB (main gate: full games; whole Local-LB, not only the top 3; don't overfit one
  opponent family). Frozen top-team replays are a secondary test.
- Audit every 30 min (PROGRESS.md) and keep IDEAS_LEDGER.md prioritised.
- RAM: at most ONE training process at a time (~24-32 GB each) and <= ~20 game workers.
  Three concurrent trainings killed the previous session (out of memory).
- Win rate is the target (the ladder counts only wins), margin second.
- Everything needed is allowed (git push to the Local-LB repo was explicitly requested).
- Many responses got cut off by a safety classifier (false positives). Keep commands
  short; re-issue a cut-off command.

## Current best agent

`models/cand_v12_vadim6/` = v12_cond (BC trained on 1.45M top-player days, strength and
recency conditioning, no style slot) + `model.bin.opening` "6 8" (Vadim Vasilenko's style
for days 0-5). Full panel (reports/panel/v12_open6_vadim, 865 games): Local-LB 94% wins
(v12 alone 88%, v11 79%); +12.5 pts vs ahmed and arsgorynich. Compare panels with
`scripts/panel_compare.py A B` (paired, seed-clustered CIs, win-rate change).

## Task in progress: optimize the day compiler, then push to the Local-LB

The user asked (before pushing): optimize compile time hard, properly (algorithms, not
just limits), with no meaningful quality drop.

Measured on 8 recorded games (`reports/lb_dump_cand/*.bin`; `scripts/opt_check.sh
<label>` replays them with build_dev/lb_replay, checks decisions against
reports/opt/base_*.txt, prints compile time): ~5-6 s compile per game, worst day ~1.5 s.

Profile (DC10_PROFILE=1 prints per-day lines "profile/solver/routes"):
- ~94% of time is the return ladder's re-solves in source/compiler.cpp (the
  market+capacity attempt; 98 failing solves = 75% of ladder time).
- Inside the solver (day_policy_local/source/policy.cpp) ~88% is route preparation;
  inside it (routes.cpp construct_routes) job reordering `reorder_jobs` ~75% of all
  compile time, and within that the EXCHANGE_ROUTE_TAILS block (families >= 2) ~4.4 of
  5.1 s. It moves spans of 2-4 jobs and swaps route tails between worker pairs, re-scoring
  whole routes (copy + score) for each candidate; bounds prune some candidates.
- Route executions themselves are cheap (~0.4 s per game).

Next step: exact speedups in the tail-exchange block (e.g. incremental scoring, cheaper
lower bounds before copy+score, caching `summarize` per route, avoiding copies), verified
by `scripts/opt_check.sh` = 8/8 identical decisions. Then re-measure.

Already done in this task:
- Local copy of the route solver: `day_policy_local/` (from REPO/day_policy/source,
  hashes in SOURCE_COPY.sha256); compiler.hpp includes it. Added SolveOptions::min_hires
  (warm start for ladder re-solves; decisions identical, ~6% faster) and
  max_executions / SolveResult::executions.
- Time-pressure valve: the bridge reads an optional trailing field = remaining overage
  seconds (the package main.py must append `remainingOverageTime`); below 25 s the
  market attempt uses SearchEffort::Fast (~4x faster), below 10 s capacity returns only +
  fast + execution cap (~7x faster). Normal hosts: identical decisions.
- Tested and rejected as a default: fast market attempt always (-1.4 pts overall,
  -3.4 pts Local-LB, reports/panel/cand_fast).
- Profiling instrumentation (DC10_PROFILE, SolverProfile, route stage timers) is in the
  code; keep it or strip it before packaging.

## Local-LB packaging (prepared, not pushed)

- `submissions/sep25-bc-opus-v12-vadim/main.py` (standalone: observation -> buffer,
  per-seat game state, new game when step goes backwards). TODO: append
  `remainingOverageTime` to the buffer for the valve.
- `scripts/package_localLB.sh <model_dir> <submission_dir> <display_name>` builds the
  bridge with conda env `kagbuild` (conda-forge GCC 14.3, glibc 2.28 sysroot,
  -march=x86-64, static libstdc++), copies model + sidecars + sources, writes
  AGENT.toml and BUILD.txt. Verified: needs GLIBC <= 2.27, no AVX, identical decisions
  to the native build. Local-LB image: python:3.11-slim-bookworm (glibc 2.36); Kaggle 2.35.
- Local-LB rules (config/leaderboard.yaml): agent id `^[a-z0-9][a-z0-9_-]{1,39}$`,
  <= 100 MB, entry `main.py:agent`, smoke call <= 5 s (all-None tiles), actTimeout 1 s
  + default 60 s overage, 20 parallel games per host.
- Push steps: in external/kaggriculture-localLB (origin git@github.com:T3pp31/
  kaggriculture-localLB.git) fetch origin, `git worktree add
  ../../work/local_lb_prs/<id> -b submit/<id> origin/main` (pattern used before),
  copy the package to agents/<id>/, run the validator (src/lb/validate.py
  validate_agent_dir) and a couple of games, commit, push, give the user
  https://github.com/T3pp31/kaggriculture-localLB/pull/new/submit/<id>.
  Suggested id: `pavel-bc-opus-v12-vadim`.

## Background state

- Expert-iteration generator (scripts/expert_iter.sh, v12 searching days 0-6 against 6
  opponent pairs with a mismatched rollout model) is PAUSED: `pgrep -x search_games`
  shows a stopped process; resume with `kill -CONT <pid>` or kill it. Traces so far:
  reports/expert_iter/*/traces (extraction verified: scripts/build_search_corpus.py).
- v13_w384b (width 384, style v2, valid ~16.3) finished training, NOT yet screened.
  v13_w384 at 29.5k was neutral in games.
- arrays_v6x (17.7k extra lower-rated perspectives) extracted; v14 (all data) not trained.
- Replay compression of REPO/replays is done (lossless zstd; README there).

## Main findings of the session (details in PROGRESS.md)

- Compiler fixes (tile limit, determinism, trims, funding): +2.0k/game, 13.7% -> 4.9%
  trimmed days. Next-dawn cash reserve is needed mid-game (without it one seed collapses).
- 13x data + strength conditioning (v12) > v11; filtering data (top/recent/winners) is
  worse than using all data; training length matters (10k vs 40k steps: -4.8k).
- Style index 0 now means "unknown team" (bad); use no style or train with --style-v2.
- Opening style pins (days 0-5) are a large lever (Vadim/DSM good, M&M bad); 6 days best.
- Per-dawn search over 7 day-level pushes: exact opponent copy triples the margin; a
  wrong opponent model keeps ~1/4 of the gain (+9.4k): large RL / expert-iteration room.
- Closed-loop deviations vs top players: fewer animals (16 vs 20) and never a 4th quadrant.

## Update at session stop (~10:55)

- Tail-exchange speedup started: exact skip of worker pairs whose routes are unchanged
  since they last found no improving move (routes.cpp, EXCHANGE_ROUTE_TAILS block,
  `quiet`/`version` arrays). Decisions identical on all 8 recorded games; the timing
  comparison was not clean (machine load) - re-time `scripts/opt_check.sh` against the
  build without it before keeping it.
- Expert-iteration job stopped: pair ahmed_v25__king_rc4 complete (games.csv + traces);
  pair king_rc4__ahmed_v25 partial traces only; other pairs not started.
- Nothing is running in the background.
