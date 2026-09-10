# Continuation handoff

Last updated: 2026-09-10.

## Read first

1. `README.md` for build commands and invariants.
2. `docs/architecture.md` for the actor/critic and information boundary.
3. `docs/roadmap.md` for the current gate.
4. `corpus/README.md` for `.kagz` temporal alignment.
5. `fast_game_engine/README.md` and `sim.hpp` for transition semantics.
6. `arena/README.md` for promotion methodology.

## Current working point

The corrected BC path through M2 is implemented. `action_codec.py` is the
NumPy-only source of structured legality and deployment decoding. Unit labels use
joint canonicalization only for accepted quantities; compact market labels use
`solo_effective_action`, because active-opponent resolution is unavailable to a
live actor.

`tools/train_bc_smoke.py` has completed one bounded CUDA run. Its JSON report is
under `artifacts/`; the `.pt` checkpoint is intentionally ignored. The production
entry point is now `tools/train_seed_models.py`. It prefers submission IDs and
otherwise splits team histories with behavioral fingerprints, makes a global
replay-level split, trains a
lineage-balanced shared warm start, then writes independent recurrent seed models.
It supervises legal conditional unit arguments, autoregressive market lines,
W/D/L, margin, and opponent flow. Checkpoints are periodic, atomic, and reject
recipe or data changes during `--resume`.

On 2026-09-09 the production pilot exposed a strict-codec failure at
`106106169.kagz:seat1:step718`. The engine DROPs worker inventory dictionaries in
insertion order, but the planner had reconstructed them in item-ID order; at a
full shed this reduced the simulated fertilizer deposit from 7 to 5. Observation
contract v2 now carries per-item insertion ranks through C++ replay observations,
the live NumPy encoder, the actor token, and the shared legality planner. The
original failure and all 439 selected train/validation trajectories pass. Nine
tests and a three-update CUDA training smoke also pass with the new input.

A tiny CUDA integration run (three trajectories, four total updates) completed on
2026-09-09. The corrected v2 architecture has also completed training, validation,
resume, export, structured PyTorch/NumPy action-parity, LocalLB validation, and
live-decoder smoke checks. These are execution evidence, not policy-quality results.

The first 1,000-replay pilot was deliberately rejected by its new deployment
gate: all four exports passed LocalLB validation with zero forfeits, but spent down
to 0 and lost both seats to the always-PASS 3,000-money baseline. The replacement
recipe is `run_pilot_v3.sh`; `run_pilot_v2.sh` remains only for reproducing the
older team-level experiment. The current recipe scans all replays, filters weak
lineages, mixes full-game recurrent streams with stratified
windows, and exports best held-out checkpoints. Do not promote based on replay loss.

The actor in `model.py` is an initial testable architecture, not a frozen format.
Preserve tensor key names unless a versioned migration is added. Always keep a
small CNN+GRU baseline on the same action codec so architecture comparisons are
meaningful.

Earlier correction evidence from 2026-09-09: CUDA training and
held-out checkpoint selection run on the RTX PRO 6000; export checked 68 complete
structured actions with maximum PyTorch/NumPy error `1.43e-6`; the 15.4 MB package
passed LocalLB validation; and a fail-fast 720-step live run completed in 4.19 s.
A full-corpus dry run selected A Poor Vul, HowardLeeTW, Thomas Tschinkel behavior
cluster 2, and 我都先道歉, all with replay score above 0.75. That older dry run is
superseded by the exact-rating pilot results below.

The canonical `run_pilot_v3.sh` now delegates to the exact-rating pipeline. Its
pinned population is ymg_aq submission `56065395` (88 local trajectories,
latest observed rating 2900.0), 3정훈 `56064002` (99, 2877.3), THUNDER THUNDER
`56078126` (40, 2827.7), and SpaTaro `56064371` (81, 2819.0). Selection writes a
submission-specific cluster manifest; it never passes team names back to the
trainer. `corpus/index/episode_agents.csv` and `crawl_state.json` contain the
checkpointed four-submission API probe and can be expanded with
`corpus/kaggle_index.py --resume`.

The first exact-rating run completed on 2026-09-10. All four exports passed
LocalLB validation with zero forfeits. In 40 paired games, ymg_aq ranked first
(14-2), THUNDER THUNDER second (11-5), SpaTaro third (5-11), and 3정훈 fourth
(5-11), just above PASS. Against PASS alone, ymg_aq and THUNDER scored 4/4,
3정훈 scored 3/4, and SpaTaro scored 0/4 with terminal money only 1--27.
The cause is identifiable in the corpus: the first three submissions each have
one canonical opening, while SpaTaro has 36 openings across 81 trajectories.
Greedy BC synthesized an unseen five-hire/two-wheat/two-cow/four-sheep opening
and immediately spent down to seven. Promotion is therefore per seed now; the
strict score threshold is unchanged and the default pilot requires three of
four seeds. Do not put rejected SpaTaro into league play without a behavior-mode
split, stronger sequence decoding, or closed-loop RL repair.

M4 self-play infrastructure was added on 2026-09-10. `policy.py` is the shared
structured stochastic sampler/PPO evaluator: it records the exact path-dependent
legality masks and an invariant test requires rollout and recomputed joint log
probabilities to match. `selfplay.py` provides the privileged critic, complete
vector-engine rollout collector, recurrent PPO, mixed PFSP pool, bounded
descendant snapshots, and deterministic parent-panel evaluation.
`tools/train_selfplay.py` trains/resumes one independent descendant and
`run_selfplay.sh` runs all promoted lineages sequentially. The launcher performs
a visible engine-build, regression-test, LocalLB-roster, and CUDA preflight;
`PREFLIGHT_ONLY=1 ./reinforcement_learning/run_selfplay.sh` exercises that path
without beginning training. Runtime settings are environment overrides, and
`./reinforcement_learning/run_selfplay.sh --resume` restores each lineage's
optimizer, RNG, league, and update state.

On successful population completion, `run_selfplay.sh` invokes
`evaluate_selfplay.sh`. That stage exports only each report-selected `best.pt`,
validates the NumPy-only LocalLB packages, constructs a de-duplicated roster of
active agents, frozen parents, and descendants, and runs fresh 720-step paired
games with seat swaps. A forfeit invalidates the evaluation. The atomic
`localLB_evaluation.json` reports field rank/rating and direct score/money margin
against the corresponding parent. It can be rerun separately after an
interrupted arena without repeating training.

Evidence: 18 tests pass, including a real short-engine rollout and PPO parameter
update. A complete RTX PRO 6000 integration traversed all 719 decisions for two
games at about 102 vector steps/s with finite log probabilities. A full-horizon
rollout, PPO update, six common-seed/seat-swapped evaluations, atomic checkpoint,
export, and LocalLB package validation also completed. That deliberately
aggressive first trial exposed excessive KL and regression, so production
defaults now use larger minibatches, a `1e-6` actor LR, one PPO epoch, a hard KL
stop, and an immutable evaluated update-zero best checkpoint. No self-play
checkpoint is promoted without fresh LocalLB evaluation.

Self-play rollouts are not disposable: both requested action streams, seed,
absolute seat assignment, terminal money, checkpoint ancestry, and every
engine parity hash are atomically stored under each descendant's `rollouts/`
directory. Every newly written file is immediately replay-verified through
`ReplayCursor`; a failed certificate aborts training before optimization.

## Known constraints

- Engine/runtime pin: Kaggriculture 1.32.7.
- Replays: 15,657 compatible `.kagz` files under `corpus/data_all`.
- Most historical replay headers have empty submission-ID lists; automatic
  fingerprint clustering is therefore required.
- Training GPU in `conda run -n kaggle`: RTX PRO 6000 Blackwell, ~96 GB, BF16.
- The actor may not consume hidden seed or opponent-private state.
- LocalLB: 1 second/action, 900 seconds/episode, 100 MB package, seat swap.
- The repository worktree contains unrelated user changes; do not rewrite them.

## Definition of done

The project is not complete when loss decreases. It is complete when a packaged,
runtime-legal learned agent passes LocalLB validation and shows a reproducible,
paired improvement against the intended field and strongest direct opponent.
