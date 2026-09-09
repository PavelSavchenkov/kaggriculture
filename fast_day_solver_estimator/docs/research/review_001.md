# Review 001 — 2026-09-09 01:17 UTC

Scheduled 01:16:39 UTC. Original goal and latest user directions reviewed: a distinct useful measurable component, 24-hour sustained work, any effective estimator method allowed, repeated unseen tests, and global review every 20 minutes.

## Decision

Continue with fast marginal labor estimation. The user accepted this revised target and explicitly allowed pattern matching, rules, learned models, formulas, ad hoc methods, small optimization problems and combinations. No reason yet to pivot. Do not return to workforce minimization as the claimed contribution.

## Evidence so far

- Imported 2,024 distinct archive problems and 571 root-solver problems. Across both, 2,595 unique full problems and 1,837 contracts after removing hires/workforce and canonicalizing tile references.
- Strictly replayed 1,578 unique saved schedule/problem pairs after reconstructing the physical purchase-only contract: 1,568 valid witnesses for 1,008 problems; 10 rejected. These are unoptimized feasible upper bounds, not minimum workforce labels.
- Initial C++ extractor emits 186 physical features without source worker count/hire events. Uncontrolled single-pass feature extraction over 2,595 problems: median 24.263 us, p95 34.415 us, maximum 172.382 us. This excludes parsing and currently includes no fitted model. It establishes plausible prediction speed, not useful accuracy.
- Fresh leaderboard metadata acquired read-only. A deterministic team-level split was written before replay downloads; development intake running. Final groups remain undownloaded and unanalyzed. Historical-family overlap still requires an audit.

## Reprioritization

1. Fix the target labels and hiring-policy normalization before fitting. A source answer count must not leak through the allowed hire-slot menu.
2. Inspect the 10 invalid witnesses and retain explicit rejection reasons. Do not silently call malformed/unsupported cases infeasible.
3. Build offline reference searches with unchanged V30, strict replay, incumbent retention and bounded-search status. Record intrinsic lower bounds separately.
4. Reproduce/tune existing formulas, then compare cheap geometry and deadline models. Fit only after source grouping and answer-leakage checks.
5. Prepare fresh-replay extraction and independent/synthetic farm families. Reserve enough of the 24-hour window for multiple frozen unseen evaluations and downstream compile-budget tests.

## Risks

The principal risk is teaching the estimator current compiler inefficiency. Keep reference intervals and solve budgets visible. The historical source-support correlation changed land and labor together and must not be used as a clean causal labor result. Team identity alone does not eliminate copied-policy similarity; check exact and structural overlap.

Next review due 01:36:39 UTC. Research deadline remains September 10 00:56:39 UTC.
