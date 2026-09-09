# Review008 — 2026-09-09 03:36 UTC

Decision: continue, with more emphasis on structural routing estimates, difficult-input detection and better reference intervals. Direct pair learning has not beaten the best augmented absolute model. Do not select a larger model merely because it is larger. Keep v1 frozen and finish its complete report.

Evidence:

- The first source validation sweep is complete:579 physical contracts,566 verified upper bounds after normalized-source merging,13 unresolved and no analytical unreachable flags. Novelty filtering and frozen accuracy evaluation are next; no v1 fit has changed.
- The primary exact-overlap-free layout stratum has44 pools. At60 seconds of recorded query CPU, the primary timing_extra method and both original baselines each have one no-certificate pool. Mean time to the reference-best cost is13.285 seconds for timing_extra,30.095 for original flat and13.143 for original geometry. Thus timing_extra saves55.86% against original flat but is1.07% slower than original geometry. Family-bootstrap saved-seconds intervals are[10.83,23.26] and[-6.33,5.73] respectively. This is a frozen cold-backend proxy with physical inputs already constructed, not the full original optimization pipeline.
- The source-only timing formula saves62.06% against original flat and13.13% against original geometry on that same unseen stratum. The latter interval includes zero. Ridge has the fastest observed mean time7.923 seconds but was a secondary frozen comparison, not the primary candidate. Avoid replacing the failed primary retrospectively.
- Novel layout marginal errors are mostly underestimates: timing_extra cold MAE91.04/cash bias-73.10 versus original flat98.73/-91.11 on113 certified endpoint pairs. Original geometry cold MAE1238.20 is very poorly calibrated in cash. Warm anchoring reduces that to121.59. Keep63 censored original-to-changed pairs visible.
- Direct antisymmetric worker/cash pair models were trained from226–258 other-family unordered pairs per fold. Best cold development result among these is pair_compact_ridge_workers126.72 with one failure; full-feature pair cash trees128.20 with two failures. Neither beats augmented absolute extra trees119.35 with zero failures. Keep the negative comparison and test pairs again only when changed-work or tighter references add new evidence.
- Independent C++ local biology construction produced289 cold contracts across17 profiles, including17 five-plan adjacent asset-count pools. All pass global item/seed conservation; they provide no global route/workforce witness. Coverage includes sparse/dense near/far/shuffled farms, all crops/animals, establishment and service, late inputs and early withdrawals. The ordinary v3 limitations on cash/capacity remain explicit. All frozen v1 methods were applied before the2988 reference queries started.
- Frozen C++ parity passes2890 synthetic predictions within8e-9 workers. Mixed feature+bound+three-model p50/p95 is30.407/149.270 microseconds, p99 188.218; large dense farms raise the tail. This is outside the initial100-us tier but is not a reason to reject future useful adaptive models under the user's average-search-speed objective.
- Terminal extraction adapter is built. It uses23 real phases and a forbidden-work bookkeeping phase23 solely to express relative tile endpoints and remaining total goods in the public24-phase contract. It must be validated before use. Solver certificates that do work or trade in phase23 must never count as terminal certificates. Current v1 domain remains ordinary24-phase days.

Next20 minutes:

1. Complete v1 source and layout reports, including exact-overlap-free results, signed errors, failure rates and comparisons against both pinned originals.
2. Validate terminal extraction and23-phase certificate filtering, then add explicit horizon-aware features/bounds.
3. Evaluate completed changed-work/deeper-reference evidence as it arrives.
4. Start a cheap route-improvement feature (optimized task-chain order and route splitting) and measure its average and tail cost. Consider an input-driven gate only after out-of-fold benefit exists.

Next review03:56:39 UTC. DeadlineSeptember10 00:56:39 UTC. No accepted final estimator or claim of faster full SOTA agent development yet.
