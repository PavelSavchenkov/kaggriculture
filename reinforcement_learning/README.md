# Kaggriculture reinforcement learning

This directory is the implementation home for the learned two-player policy. It
is intentionally separate from `strategy/`: the latter contains hand-written
policies and economic investigations, while this package owns replay pretraining,
batched engine rollouts, population training, and learned-policy deployment.

The target is **match outcome**, not terminal cash in isolation:

```text
z = sign(my final money - opponent final money)  # win/draw/loss
```

The engine is known and parity checked, so the project does not learn dynamics.
It learns a deployable observation-history policy, an opponent belief, and a
continuation value. See [architecture.md](docs/architecture.md) for the model and
[roadmap.md](docs/roadmap.md) for the staged delivery plan.

## Current status

This first vertical slice provides:

- a pybind11 `VectorEnv` backed directly by `fast_game_engine/sim.hpp`;
- actor-safe observations that omit opponent-private state and hidden seed, while
  retaining own inventory insertion ranks for exact capacity-limited DROP behavior;
- a streaming `.kagz` `ReplayCursor` with the documented `state[t] -> action[t+1]`
  alignment;
- a compact masked Perceiver-style actor with spatial encoding and hourly/daily memory;
- one NumPy structured action codec shared by training and submitted agents;
- legality-aware recurrent BC with state-carrying replay streams and stratified windows;
- PFSP opponent sampling utilities;
- exact-engine recurrent PPO self-play with frozen parents and bounded snapshots;
- best-checkpoint export, LocalLB validation, and fresh-field evaluation;
- CPU regression tests and GPU/model/rollout integration tests.

The full BC-to-league pipeline is implemented, but no learned checkpoint is
claimed competitive until its exported package passes the fresh LocalLB gate.
Action legality, portable structured decoding, behavior fingerprints, exact replay
certification, checkpoint recovery, and deployment export are implemented.

Architecture v2 reduces the default actor from 8.9M parameters to a 192-wide,
four-block model. Exported smoke packages are about 15.4 MB. Rebenchmark throughput
on the production checkpoint; old v1 timing numbers no longer apply.

The first 100-update BC smoke run used 12 replay episodes with a replay-disjoint
validation split. Validation loss moved 5.37 -> 2.09, effective unit-operation
accuracy reached 46.7%, first market-line accuracy 76.1%, and W/D/L accuracy 50%.
See `artifacts/bc_smoke.json`. This proves the gradient path; it is not a policy
quality claim.

## Environment

Use the repository's `kaggle` Conda environment. It contains PyTorch with CUDA
13, pybind11, CMake, and the Kaggriculture 1.32.7 runtime.

```bash
conda run --no-capture-output -n kaggle cmake -S reinforcement_learning \
  -B reinforcement_learning/build -G Ninja
conda run --no-capture-output -n kaggle cmake --build reinforcement_learning/build

PYTHONPATH=reinforcement_learning/python \
  conda run --no-capture-output -n kaggle python -m pytest -q reinforcement_learning/tests

PYTHONPATH=reinforcement_learning/python \
  conda run --no-capture-output -n kaggle python reinforcement_learning/tools/smoke_model.py

# Bounded gradient/update smoke test (not a competitive checkpoint)
PYTHONPATH=reinforcement_learning/python \
  conda run --no-capture-output -n kaggle python \
    reinforcement_learning/tools/train_bc_smoke.py

# Recurrent seed training (defaults: eight lineages, shared warm start, then
# independently materialized descendants). Checkpoints are intentionally ignored.
PYTHONPATH=reinforcement_learning/python \
  conda run --no-capture-output -n kaggle python \
    reinforcement_learning/tools/train_seed_models.py

# Continue an interrupted run to the requested total update counts.
PYTHONPATH=reinforcement_learning/python \
  conda run --no-capture-output -n kaggle python \
    reinforcement_learning/tools/train_seed_models.py --resume
```

Use `--no-capture-output` for training. The tools emit line-oriented progress with
counts, percentage, elapsed time, throughput, and ETA; allowing Conda to capture
stdout would delay those updates until the process exits.

For the reviewed closed-loop pilot recipe, run the single orchestration script:

```bash
./reinforcement_learning/run_pilot_v3.sh
```

It joins the corpus to Kaggle's episode ratings and selects four exact
submission-ID lineages rather than mixing every policy uploaded under one team
name. The pinned pilot submissions are `56065395`, `56064002`, `56078126`, and
`56064371`; override the comma-separated `SUBMISSION_IDS` environment variable
only for an intentional new population. Training uses up to 128 trajectories per
lineage, mixes three state-carrying 48-step chunks with one strategic/endgame
window, uses a 168-step validation burn-in, batch size 32, 5,000 balanced
warm-start updates, and 2,000 lower-LR lineage updates. It then exports the four
checkpoints into the sibling LocalLB checkout, validates the exact submission
directories, and plays full 720-step paired games. Seeds qualify independently:
each must score strictly above 0.5 against the always-PASS baseline, and the pilot
requires three qualifiers by default (`MIN_PROMOTED` overrides this). The accepted
IDs and their money margins are written to `promoted_agents.json`; a rejected seed
does not invalidate stronger, behaviorally distinct seeds. If interrupted, rerun
the same script with `--resume`; it restores optimizer and RNG state.

To measure only the promoted roster against the full active LocalLB field:

```bash
./reinforcement_learning/rerun_locallb.sh 20 rl-elo2 \
  reinforcement_learning/artifacts/seeds-pilot-elo-v2/promoted_agents.json
```

## Self-play descendants

Train one independent recurrent PPO descendant from each promoted seed:

```bash
./reinforcement_learning/run_selfplay.sh
```

The launcher shows progress for preflight, population members, rollout
collection, PPO updates, evaluation, snapshots, and replay certification. It
builds the exact engine, runs the regression suite, checks the promoted LocalLB
packages, and verifies CUDA before training. After all lineages finish it calls
`evaluate_selfplay.sh`: each selected `best.pt` is exported and validated, then
the descendants, their parents, and the active LocalLB field play fresh 720-step
seat-swapped games. The final comparison is written to
`artifacts/selfplay-v1/localLB_evaluation.json`. To check the training launch
path without starting an update:

```bash
PREFLIGHT_ONLY=1 ./reinforcement_learning/run_selfplay.sh
```

The final evaluation is independently rerunnable without retraining:

```bash
./reinforcement_learning/evaluate_selfplay.sh
```

While later lineages are still training, export and test the first completed
descendant against the active LocalLB field plus its parent with one command:

```bash
./reinforcement_learning/evaluate_selfplay_one.sh
```

Pass a promoted parent agent ID or a self-play output directory to select a
specific completed lineage. The launcher uses challenger-only arena mode, so it
does not replay unrelated LocalLB-vs-LocalLB pairs.

It defaults to 20 fresh seeds per pair. Use `LOCAL_LB_SEEDS_PER_PAIR` to change
that number, or `RUN_FINAL_EVALUATION=0` to intentionally omit the automatic
post-training tournament.

The production defaults use 32 complete 719-decision games per update, alternate
the learner's seat, sample one opponent per rollout with a competitive/exploiter
PFSP mixture, and optimize 24-step recurrent chunks. The actor uses terminal
win/draw/loss returns with `gamma=1`; intermediate cash is not a reward. A
separate centralized critic sees both private inventories, but those features
never enter the actor. Frozen behavior clones and bounded descendant snapshots
remain in the opponent pool.

The initial seed is evaluated and saved as update zero. PPO uses a conservative
`1e-6` actor learning rate, larger sequence minibatches, gradient clipping, and
a joint-action KL stop. Thus `best.pt` remains the parent unless a deterministic,
common-seed, seat-swapped parent-panel evaluation improves. `latest.pt` is the
resumable exploration state and can be worse; never export it as the best model
by hand.

Useful bounded runs:

```bash
# One learner and ten real-horizon updates.
PYTHONPATH=reinforcement_learning/python \
  conda run --no-capture-output -n kaggle python \
    reinforcement_learning/tools/train_selfplay.py \
    --promotion-manifest reinforcement_learning/artifacts/seeds-pilot-elo-v2/promoted_agents.json \
    --lb-root ../kaggriculture-localLB \
    --learner-agent rl-elo2-ymg-aq-submission-56065-d8b594f7 \
    --output reinforcement_learning/artifacts/selfplay-v1/ymg-aq \
    --updates 10

# Continue every population member to the configured total update count.
./reinforcement_learning/run_selfplay.sh --resume

# Override bounded settings without editing the launcher.
UPDATES=10 ENVIRONMENTS=32 ./reinforcement_learning/run_selfplay.sh
```

Every output directory contains an export-compatible `training_report.json`,
`best.pt`, resumable `latest.pt`, evaluation records, and a bounded opponent
snapshot archive. By default every rollout is also written atomically as
engine-verified `.kagz` under `rollouts/update_*`; set
`RECORD_KAGZ_EVERY=0` only when that replay archive is intentionally unwanted.
Export a descendant with `tools/export_seed_agents.py`, then
use the normal LocalLB validation and arena gates on fresh seeds. Parent-panel
selection prevents a known local regression; it is not a substitute for that
independent promotion test.

`train_seed_models.py` uses submission ID when a replay contains it. Older corpus
files have empty submission-ID lists, so large team histories are split by
phase-conditioned action fingerprints.
Use `--cluster-manifest reviewed.json` to supply reviewed lineage assignments.
The generated `manifest.json` records the exact membership and a global
replay-file split: neither seat of a match can leak across train and validation.

`corpus/kaggle_index.py` obtains exact per-seat `submissionId`, `initialScore`,
and `updatedScore` rows from Kaggle's Episode API. It checkpoints
`corpus/index/episode_agents.csv` plus its crawl queue and can continue with
`--resume`. `tools/select_lineages.py` ranks exact submissions by their latest
observed `updatedScore` and writes the episode/seat cluster manifest consumed by
training. Team-only rating files are rejected unless `--allow-team-fallback` is
explicitly requested.

The v2 observation contract includes `own_inventory_order`, a 1-based insertion
rank for every present item in every worker inventory. This is not cosmetic:
Kaggriculture iterates inventory dictionaries in insertion order during DROP, so
the order determines which goods survive when the 100-item shed cap binds. The
shared codec and actor both consume this feature. Checkpoints made before
`observation_version = 2` are intentionally not resumable.

The loss applies deployment legality masks, teacher-forces a compact market
sequence with a decoder END token, conditions item/quantity losses on the selected
operation, and adds terminal W/D/L, margin, and opponent-flow auxiliaries. The global
warm start samples lineages uniformly; each seed is then a complete independent
checkpoint suitable for later league descendants. `--resume` restores model,
optimizer, Python sampling RNG, and Torch CPU/CUDA RNG state and rejects changed
model/data lineage.

`tools/export_seed_agents.py` removes optimizer state and emits a NumPy-only
submission. Export is rejected unless model outputs and complete decoded actions
match PyTorch on fresh and replay-derived states. The generated `main.py` is the
exact agent used by LocalLB.

The extension is written into `python/kaggriculture_rl/` by CMake. It is ignored
by the repository-wide `*.so` rule and must be rebuilt on a new checkout.

## Non-negotiable invariants

1. The actor receives only information available to a live agent. Privileged
   engine state belongs in a separate training critic.
2. Replay supervision pairs state `t` with recorded action `t+1`.
3. Policy reward is terminal W/D/L. Cash and production are auxiliary targets.
4. Both seats and common seeds are used for evaluation.
5. A checkpoint is promoted through the LocalLB-compatible arena, not by replay
   likelihood or mean cash.
6. Training imports engine mechanics from the C++ binding; deployment injects the
   official environment's market-pricing function into the shared codec.

## Directory map

```text
reinforcement_learning/
  cpp/core.cpp                    batched engine and replay bindings
  python/kaggriculture_rl/
    model.py                      actor architecture
    action_codec.py               shared masks and structured action decoder
    league.py                     population/PFSP utilities
    schema.py                     stable tensor/action schema
  tests/                          executable interface tests
  tools/smoke_model.py            CPU/CUDA forward-pass check
  tools/train_seed_models.py      recurrent lineage seed trainer
  docs/architecture.md            model and learning design
  docs/roadmap.md                 gates and implementation order
  docs/handoff.md                 concise continuation record
```
