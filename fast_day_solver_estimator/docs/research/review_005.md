# Review005 — 2026-09-09 02:36 UTC

Decision: continue. Prioritize comparisons with the original pipeline and changed-plan generalization over further source-day model fitting. Keep the first frozen candidate's failures visible.

Evidence:

- C++ candidate parity passes685 physical inputs, with maximum difference below8e-9 workers for all three models.13,700 shuffled-input predictions cost60.551 us median/81.179 us p95, including features, bounds and all three models. Feature extraction accounts for most of the cost.
- First validation wave:32 selected player-games, all replay parity checks pass.909 ordinary source days pass strict physical extraction;19 capacity-loss cases and32 terminal days remain explicit exclusions. There are579 physical contracts/591 full hashes across8 new teams. Frozen C++ predictions were saved before reference searches started.5,192 fixed reference queries are running.335/591 normalized source schedules pass under the new hire timing; other source schedules remain rejected under that timing.
- Development layout reference is complete:240 candidates in48 pools,172 have solver certificates and47 have analytically unreachable deadlines. The remaining21 are unresolved at the declared budget, not labeled infeasible.
- First46-pool layout study exposes a model-selection problem: the timing formula's top-one cold choice has mean verified cost113.83 and zero no-certificate selections. The best source-day worker-count model, timing+residual trees, costs145.93 conditionally and has one no-certificate selection. Old fitted geometry costs132.22 conditionally with five failures. A model's low worker MAE does not establish useful proposal ranking.
- With a verified original-layout incumbent available, all methods keep that cost if their new candidate fails or is worse. Report this separately from cold selection; it is a counterfactual layout-design proxy, not a claim of legally moving existing crops during a game.
- Original baseline code/evidence is pinned locally. The latest general model uses additive operation coefficients0,10,25 in its published sweep;10 gives the historical2520 estimate for252 added operations.13,728 archived comparisons match this term within0.05 cash, the published rounding precision. The earlier geometry/work estimator's labor arithmetic matches the copied original implementation on15,000 generated composition-days. Its DayProblem adapter preserves that arithmetic but bypasses original biology/placement forecasting; do not call it full-pipeline parity.

User steering retained:

1. Keep new, diverse data unseen until a version's evaluation; track error direction and specific hard cases.
2. Compare every version with the original SOTA development-pipeline estimator as well as tuned baselines.
3. Optimize average complete search speed. Rare difficult cases may use more compute;100 us is a benchmark tier, not a universal per-case ceiling. Judge heavier processing by improved decisions and reduced total search cost.

Next20 minutes:

- Add exact original baseline rows and deltas to layout/marginal reports.
- Freeze predictions for validation layout variants before their reference outcomes; keep later family reserves unused.
- Train/evaluate changed-layout augmentation within complete parent-family splits. Diagnose where trees over/underestimate alternatives; consider routing disagreement as an input-driven deferral signal.
- Add time-to-quality and compilation-effort measures with prediction overhead included. Preserve UNKNOWN and reference-interval uncertainty.

Next review02:56:39 UTC. DeadlineSeptember10 00:56:39 UTC. No accepted model or whole-agent gain claim.
