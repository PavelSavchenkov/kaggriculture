# Evaluation and validation pipeline

How the main session validated every change: the test beds, what each one measures and misses, the
order they run in, the promotion rule, the commands, and the traps that produced wrong conclusions.
Commands run from a working experiment folder made by `setup_experiment.sh`, with a build in
`build/` (all C++ beds are deterministic and replay bit-exactly).

## 1. The pipeline at a glance

| Stage | Bed | Opponents | Engine / harness | Games, time | Measures | Blind spot |
|---|---|---|---|---|---|---|
| 0 | Correctness gates | - | C++ | seconds-minutes | legality, exactness, coverage, parity, latency | strength |
| 1 | Self-play mirror | our current best (own sidecars) | C++ `search_games` | 8 traced games 35 s; 40/set 2-5 min | head to head vs our lineage | forecast changes; seat mirrors halve the sample |
| 2 | Local C++ league | 7 committed agents (varied styles) | C++ `full_games` | 224, ~15 min | robustness vs varied behaviour, opponent-model changes | these agents are weaker than the Kaggle top |
| 2 | Zoo league | BC clones of the Kaggle top-6 teams | C++ `full_games bc:` | 192 | network quality (both sides same compiler) | clones share our compiler |
| 3 | Local-LB panels | the team league's agents, unseen seeds | official Python engine (`lb_play.py`) | 240-480, 30-60 min | strength vs the league on seeds nobody tuned on | the league has one dominant plan family |
| 3 | Exact Local-LB replay | the current Local-LB roster, its own seeds | same | 140-400 | predicts the Local-LB table exactly | only as broad as the roster |
| 3 | Full repository roster | all 69 Local-LB agents | same | 966, hours | regressions vs any known agent | slow; run last |
| 3 | Head to head vs our own Local-LB agents / PPO | `pavel-bc-opus-v12-*`, JJ's `ppo-v6n-it60` | same | 40-80 | the matchup that decides the Local-LB #1 | model-path leak (section 6) |
| 4 | Frozen Kaggle top-10 replays | 145 held-out games of top-10 teams (Sep 18+), recorded seat frozen | C++ `replay_games` | 145, ~10 min | our play against real top-team plans and timing | the frozen seat does not react (its recorded orders can fail) |
| 4 | Frozen Kaggle games of our submission | the 64 real opponents our `robust` submission met, frozen | C++ `replay_games` | 64 | the closest local proxy of today's Kaggle opponents | same; opponent pool of one rating range |
| 4 | Replay continuations | recorded opponent from day 20/25/29 | C++ `continuation` | per perspective | diagnostics of late-game intents | ranked models wrongly vs games |
| 5 | Kaggle plays of our submission | real ladder | Kaggle | ~10-20 per hour | the true objective | slow, one submission slot costs a day; rating noise |

Stage 0 runs for every change; stages 1-2 screen; stage 3 confirms; stage 4 checks top-team play;
stage 5 is analysed after submitting (it drives the weakness list).

## 2. Stage by stage

**0. Correctness gates.**
- `ctest` (mechanics test); a refactor or a new option with its default off must reproduce recorded
  games bit-exactly: compare the C++ panel/mirror CSVs, or replay recorded Local-LB games with
  `tools/lb_replay` (record them with `LB_DUMP=1` in `lb_play.py`, or `OPUS_DUMP=<file>` in the
  bridge) and compare every action hash as `scripts/opt_check.sh` does (the 8 recorded games of the
  original are not included: record new ones). Every Sep 25 option was checked "defaults exact 8/8".
- New network: coverage gate (`tools/extract`, 0 failures), native parity (`tools/parity` +
  `scripts/parity.py`), teacher-forced decode check (`tools/decode_eval`).
- Latency: compile ms per dawn in every CSV (`compile_ms_max`), `DC10_PROFILE=1`; overage use through the
  real Local-LB runner (`scripts/lb_runner_games.py`): at most 0.7 s of 60 s per game.
- Package: the portable bridge plays the same actions as the dev build (`agent/play.py` action hashes).

**1. Self-play mirror.** `scripts/cand_mirror.sh models/<reference> build <threads> <seed_start>
models/<candidate> ...`: both sides are our agent with their own sidecars; 20 seeds x 2 seats per set;
the CSV row is from the candidate's side. Sets used: 1300, 1400, 1500, 1600 and 7000 (the Local-LB seeds
of the pairing that decides #1). Variants: `knob_mirror.sh` (opening, strength), `net_mirror.sh`
(networks under one compiler). A fast smoke is 8 games with traces (`BC_WRITE_TRACES=<dir>`,
`BC_GAME_TRACE=<seed>:<seat>` prints each dawn: cash, farms, intent, compile status and reason).

**2. Local C++ league.** `scripts/cpp_panel.sh models/<candidate> build <threads> <out>`: agent_sep23
(previous best BC), king_rc4, teammate_shoprouter, arlene_v4_m31, ahmed_v25,
investment_context_guarded_001_best, two_random_shop_league_v179 (all committed in `agents/`), seeds
700-715 both seats. Compare paired with a seed-clustered bootstrap (RESULTS.md uses it). Zoo league:
`scripts/zoo_league.sh` against six BC clones of DSM, DECEM, Mother-Goose, Majkel, Vadim, M&M (weights in
the team's Kaggle dataset `pavelsavchenkov/kaggriculture-agent-zoo-sep25`, put them in `models/zoo_*`).
Our older versions play as `bc:<model.bin>` opponents.

**3. Local-LB.** Official kaggle-environments interpreter with the Local-LB's own loader
(`scripts/lb_play.py`, one JSON per game with a per-dawn record). Snapshot: clone
[T3pp31/kaggriculture-localLB](https://github.com/T3pp31/kaggriculture-localLB) to
`data/localLB_main` and pass `LB_SNAPSHOT=localLB_main`.
- Broad unseen panel: `scripts/panel_lb10.sh <out> 1300-1311 [procs]` (select), then 1312-1323
  (confirm). Edit its agent list to the current active roster.
- Exact replay of the Local-LB evaluation (section 4): run before every push; it has predicted every
  result, e.g. `slots` 399/400 predicted and 399-1-0 on the table.
- Full roster: every agent in `data/localLB_main/agents` with the same seed rule (`roster_all` in
  `evidence/`): regressions vs anything known.
- Head to head vs our own merged agents and JJ's PPO agent (`lb_play.py` with their ids); valid only
  with the fixed harness (section 6).
- Extra Python agents (public-notebook candidates, teammates' PPO/RL): `scripts/gates.sh` expects them in
  `data/extra_opponents/` (not included).

**4. Kaggle replays (frozen).** `build/replay_games <list> <threads> out.csv`, list lines
`<trace> <replayed seat>`: we play the other seat from the same seed and config; the replayed seat
repeats its recorded orders. CSV columns include the original result for comparison.
- Top-10 bed: `tar -xzf <handoff>/traces/frozen_top10_145.tar.gz` in the experiment root (creates
  `data/traces/`), list `data/replay_opponents.txt` (145 games; `scripts/replay_list.py` built it from
  held-out episodes). `forecast`: 143/145.
- Our-submission bed: `tar -xzf <handoff>/traces/kaggle/robust_56553038_traces.tar.gz` (creates
  `data/kaggle_traces/`), list `<handoff>/traces/kaggle/frozen_kaggle_list.txt` (64 public games; the
  replayed seat is the opponent). The companion study used the 31 games available at the time together
  with the 145 (its `frozen_list.txt`, 176 games).
- Replay continuations: `build/continuation corpus.txt out.csv 20 25 29` (diagnostic only).

**5. Kaggle plays of our submission.**
- Before submitting: `agent/package.sh` -> `agent/play.py <folder> starter --seeds 1-2` in
  kaggle-environments 1.32.7; optionally a Kaggle kernel check with 1.32.7 installed (the kernel image
  has 1.29.3 with old rules; our agents score $2k-22k there).
- After Kaggle validates: download the validation episode and run
  `agent/reproduce_validation.py <folder> episode-<id>-replay.json` (must reproduce every action;
  `robust`: 0 of 1,440 differ).
- While it plays: `analysis/kaggle/fetch.py <submission id>` (Kaggle API; `BC_EXPERIMENT` = the working
  experiment) downloads every episode, converts replays to engine traces (exact rewards) and writes
  `meta_<id>.csv` (opponent team, ranks, rewards). Then per game and product, ours minus theirs: build
  `analysis/` and run `ledger <list> <prefix>` (list lines `trace label0 label1`), and the per-loss
  tables (`analysis/queries/q48.py`, `q58.py`). The frozen-Kaggle bed above turns these games into a
  local test.

## 3. Promotion rule used for each Local-LB push

1. Screen in the mirror vs the current best on one seed set; stop ideas that show nothing.
2. Confirm on >= 4 mirror seed sets including the Local-LB seeds of the deciding pairing (160-200 games).
3. No regression on the C++ league (paired), and for market/forecast changes a gain there, because the
   mirror cannot show it.
4. Frozen top-10 replays: no farm collapses, win count not lower.
5. Exact replay of the Local-LB evaluation predicts the table position; head to head vs the current #1.
6. Package, check identical play of the portable bridge, overage use in the Local-LB runner, validator.

## 4. Local-LB rules and submitting

- Rules since main 490c63d (Sep 26 PRs #185-#188, `config/leaderboard.yaml`): 7 seeds per pair, top-30
  roster by rating, act timeout 1 s + 60 s overage, 20 parallel games. A challenger plays every active
  agent of `data/rankings.json` in alphabetical order (challenger excluded); the k-th (from 0) gets
  seeds 1000*(k+1) .. +6, both seats (`src/lb/match.py` `_pair_seeds`). Before: 10 agents x 20 seeds.
- Exact replay: write the active ids (from `data/rankings.json` of the current main, after the rebuild)
  to a roster file, then
  `BUILD=build BC_OPUS_MODEL=$PWD/models/forecast/model.bin scripts/lb_seeds_roster.sh roster.txt reports/roster/forecast 8`.
- Submitting: branch `submit/<agent id>` from origin/main, copy the package (`agent/package.sh`) to
  `agents/<id>/`, write `AGENT.toml` (display_name, author), run the validator from the Local-LB root with
  `PYTHONPATH=src` (`validate_agent_dir`), play a couple of games, push, open a PR. Agent id pattern
  `^[a-z0-9][a-z0-9_-]{1,39}$`, <= 100 MB, smoke call <= 5 s.
- `latest-yannik-suffix-5d-28s-v1` is nondeterministic late in the game; exclude it from determinism
  checks.

## 5. Statistics and seeds

- Report win rate first (the ladders count wins), then margin, then own and opponent money separately.
- Paired comparisons on identical seeds and seats; seed-clustered bootstrap CIs (both seats of a seed
  are one cluster; they are often exact mirrors, so 40 games carry ~20-30 outcomes). Comparison
  scripts: `scripts/lb_compare.py A B` (Local-LB folders), `scripts/panel_compare.py A B` (panels).
- Unseen seeds for selection. Seeds 700-731 selected the Vadim opening (94% there, 88% on fresh seeds).
- One 40-game set can flip: DSM vs Vadim went 25-15, 31-9, 26-14, then 17-23 on the Local-LB seeds.
- Seeds used: 600-631 (v1-v9 selection), 700-731 (opening selection; avoid for opening decisions),
  1200-1231 (fresh check), 1300-1323 (broad panel), mirror sets 1300/1400/1500/1600, 7000-7019, and the
  Local-LB rule 1000*(k+1)..+6.
- Screen small, confirm big: kill runs several times slower per game than the baseline.
- `evidence/` holds every result of the experiment in merged CSVs (`summary.csv`: one row per run).

## 6. Traps we hit (each cost hours)

- **Model-path leak between agents in one process.** Older packages of this agent set `BC_OPUS_MODEL`
  at import; our bridge reads it when a game starts, so our challenger silently played their model
  folder. Every sidecar head-to-head before Sep 25 15:25 was invalid. `lb_play.py` now sets our path
  only around our `opus_new`; `main.py` sets it just before `opus_new`. Check which model and sidecars
  each side really loaded.
- **`DC10_*` environment knobs leak into opponents' bridges** in the same process. Use sidecars
  (`model.bin.compiler`) for candidates.
- **Stale binaries**: a panel ran with a `full_games` built by a partial `--target` build. Build all
  targets before copying binaries; check that an old configuration reproduces a recorded game.
- **Wall-clock limits make results machine-dependent** (2 of 8 games differed on a rerun); the compiler
  uses fixed counts. Under heavy load the overage valve (< 25 s left) can still change play: compare on
  an idle machine.
- **Shop draws share the night RNG with weed spawns** (one draw per empty tile on both farms). A change in
  the number of empty tiles reshuffles later shops for both players, so paired games are paired only
  until then; margins stay comparable, own/opponent money splits do not.
- **Wrong evaluation roster**: two Local-LB agents were dropped from a panel as "identical" to others;
  they were our weakest matchups. Check identity by playing.
- **Machine limits** (i7-14700K 8P+12E, 125 GB RAM, one RTX 5090): CPU time per game is flat from 4 to 16
  threads, so oversubscription only delays results and starves GPU training; keep ~16 game threads, one
  training process at a time.
- **`pkill -f` / `pgrep -f`** from a shell whose own command line contains the pattern kills that shell.
  Kill by exact PID.
