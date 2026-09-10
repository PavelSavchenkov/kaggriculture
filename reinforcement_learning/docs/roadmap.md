# Delivery roadmap and gates

Each milestone must leave an executable artifact and evidence. Do not begin a
large training run while an earlier mechanics gate is open.

## M0 — foundation

- [x] Dedicated package and build.
- [x] Fast-engine vector container with actor-safe observations.
- [x] Streaming replay cursor with correct temporal alignment.
- [x] Initial spatial/entity actor and recurrent state.
- [x] PFSP probability primitive.
- [x] General batched joint-action `step` interface.
- [ ] Throughput benchmark including observation encoding and network inference.

Gate: C++ extension builds in `kaggle`; tests reproduce a replay and model forward.

Evidence: 18 tests pass; the first CUDA BC smoke run completed 100 updates over
12 episodes in 4.79 seconds and improved replay-disjoint validation loss from
5.37 to 2.09. This is connectivity evidence only.

## M1 — exact action codec (BC path complete)

- [x] Generate unit and market masks from actor-visible state.
- [x] Implement structured exact-quantity decoding shared with deployment.
- [x] Separate unit canonical quantities from observable PASS-opponent market labels.
- [ ] Fuzz sampled actions against engine diagnostics at corpus scale.
- [x] Verify that canonical and raw joint actions produce identical next parity hash.
- [x] Preserve inventory insertion rank across replay/live observations and exact
  capacity-limited DROP planning; audit all 439 selected pilot trajectories.

Gate: every canonicalized sample in a stratified replay set produces the recorded
next state, and sampled policies emit no unintended no-ops.

## M2 — replay dataset and behavioral clusters

- Stream replay sequences without materializing the 450 MB archive as JSON.
- [x] Build phase-conditioned episode behavioral fingerprints.
- [x] Split evident team/version mixtures; accept a reviewed manifest override.
- Persist immutable train/validation/test manifests by date and lineage.
- [x] Add recurrent BC entry point with exact effective labels, global replay-file
  split, lineage-balanced warm start, independent seed checkpoints, held-out loss,
  periodic atomic checkpoints, masked metrics, and recipe-safe resume.
- Compare full independent copies (current implementation) against adapters after
  closed-loop identity measurements exist.

Gate: each retained clone runs closed-loop on unseen seeds without collapse and
retains measurable behavioral identity. Report transition-effective accuracy, not
only raw token accuracy.

## M3 — value and opponent belief

- Train observation-history W/D/L and margin auxiliaries.
- Train future opponent-flow and hidden-reserve belief heads.
- Build separate privileged centralized critic.
- Report calibration and Brier/log loss by game day and held-out lineage.

Gate: value is calibrated out of sample and recurrent opponent features improve
prediction over a current-observation-only ablation.

## M4 — batched league learning

- [x] Exact C++ vector-engine rollouts with batched BF16 GPU inference.
- [x] Recurrent PPO learner with stored hidden states, exact structured action
  log probabilities, terminal W/D/L returns, privileged critic, gradient
  clipping, KL stopping, atomic checkpoints, and deterministic resume.
- [x] Frozen parent archive, independent descendants, mixed
  competitive/exploiter PFSP sampling, and bounded descendant snapshots.
- [x] Common-seed, seat-swapped deterministic evaluation against frozen parents;
  update-zero is retained unless a descendant improves on that panel.
- [ ] Add asynchronous rollout workers after profiling shows collection, rather
  than optimization, is the limiting stage.
- [ ] Add an explicit behavioral-novelty term; current diversity comes from
  independent parents and the opponent mixture.
- [x] Record generated matches atomically back to certified `.kagz` with seeds,
  seat, terminal money, full parity-hash anchors, and checkpoint ancestry.

Gate: at least two independent descendants improve over their clone ancestors on
paired unseen seeds, without converging to the same behavioral fingerprint.

## M5 — engine policy improvement

- Candidate-action sampling and exact short rollouts.
- Belief particles for hidden opponent inventory and RNG uncertainty.
- Observation-only bootstrap value.
- Distill search improvements back into the actor.

Gate: search improves paired match score after charging its full inference cost.

## M6 — deployment and evaluation

- [x] Export dependency-light NumPy inference behind `main.py`.
- [x] Use the same export path for report-selected self-play `best.pt` checkpoints.
- [x] Validate exported descendants through LocalLB before evaluation.
- [x] Run fresh full-horizon, seat-swapped parent and active-field evaluation and
  emit a machine-readable descendant comparison.
- [ ] Benchmark all 719 calls against the 1-second action and 900-second episode limits.
- Quantize only after W/D/L parity tests.
- [x] Run full `arena/arena.py` and LocalLB validation automatically after training.

Gate: zero forfeits, package below 100 MB, reproducible results, statistically
supported improvement in both field and top-agent evaluations.
