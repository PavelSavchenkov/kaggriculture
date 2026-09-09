# Final results and their limits

Results compare the relevant original pipeline estimator: geometry for absolute workforce/cold ordering and flat10 per operation for marginal work cost. They are different controls and must remain separate.

## Accepted ordinary cold ordering

Frozen before the second new-family dataset was downloaded, the direct-cost forest plus nearby-count ordering reduces mean time to the best reference bill from 13.205633 to 1.615992 CPU seconds on 46 novel layout pools: 87.7629% less. The family-bootstrap 95% interval for saved mean CPU is [5.4541, 18.4340] seconds. p95 falls from 40.0017 to 4.6439 seconds. Actual C++ scoring/selection costs are included.

Within 30 CPU seconds, best-reference-bill coverage improves 43/46 to 46/46; any-certificate coverage improves 45/46 to 46/46. Mean verified bill is 392.29 on the original's 45 certified pools versus 153.98 on all 46 guided pools. Do not compare these means without their coverage. Time-to-best is retrospective, not an implementable stopping rule or complete warm-course runtime.

On 558 labeled novel source contracts, worker-equivalent MAE falls 3.623656 to 0.497262; family-macro MAE falls 3.820867 to 0.450596. Signed bias is -0.079 workers. Cash MAE falls 1421.896 to 91.894; signed cash bias changes +1321.943 to -63.830. Fourteen of 558 forecasts underestimate the upper bill by at least 500; none overestimate it by 500. Two additional source contracts remain unresolved.

On 166 labeled novel removal pairs, marginal cash MAE falls 214.795 to 26.489 (87.6676% lower), with signed bias changing -212.00 to +7.24. The family-bootstrap saved-error interval is [145.96, 227.96]. Twenty-two pairs remain censored. These are removal savings, not symmetric evidence that large additions are solved.

## Accepted warm seed-transfer scope

The unchanged `.02` deferral candidate was evaluated in 56 paired complete compiler cases: eight new seeds and seven specifications, retaining the same source, rival, and shop sequence. Mean complete child CPU falls 163.439797 to 125.853622 seconds, a 22.9970% reduction. The seed-cluster 95% interval for saved mean CPU is [26.7334, 45.3555] seconds. Wall means are 127.95 and 92.34 seconds under the measured concurrent workload.

Original and guided complete 30 and 34 courses respectively; no original certificate is lost and four are gained. All 64 successful outputs pass independent full-game verification over 719 transitions. All 30 common-success pairs have equal production. Mean hire-bill change is -4.8, with seed-cluster interval [-64.85, 48.84]. Nine bills increase, eight decrease, thirteen are equal; worst increase is 466. This is a finite-sample scoped pass, not a guarantee of individual non-regression.

Complete timing includes contract generation, reuse, four-thread repair, cold fallback, endpoint checks, launcher and artifact output. Independent post-run verification is outside both timed invocations. The 1,979 prediction records total 0.250 wall seconds. All 1,720 common executed full query inputs match exactly, yet 83 outcomes differ. Bounded parallel search and subsequent schedule effects are variable.

The earlier 14-pair exposed `.02` benchmark saved 23.89% CPU but lost one certificate and raised common-success bills by 21.18 on average. It failed quality and remains on record. A necessary-only screen was 4.35% slower and raised bills by 58.25; it is rejected.

## Costly additions remain difficult

After deeper evidence, the owned-addition panel has 170 labeled pairs and 19 censored pairs. Direct-cost marginal MAE is 122.12 versus original flat10 147.23; signed bias is -101.15, with four underestimates of at least 500. Its family-macro gain is only 17.87%, below the later 20% addition gate. The new crop case with a 13,530 verified upper-bill increment is predicted as -4.14.

The larger-addition panel has 136 labeled pairs and 61 censored pairs. Direct-cost MAE is 3973.31 versus flat10 4056.85. A newly certified 512,632 upper-bill increment dominates all methods; prediction is 269.97 and its physical interval is [-1563, 514216]. The minimum marginal cost is not identified. On the previous 134-label cohort, stronger references gave timing-plus-extra MAE 49.65 versus flat10 175.56. Preserve both cohorts; do not discard newly labeled hard cases.

The old forest has only eight of 925 training examples at 15+ workers and none above 17. No tree leaf exceeds 1537.75 cash, so neither can the raw forest average. The worst owned addition leaves 125/128 trees unchanged. Individual feature-range checks miss three of four severe owned marginal errors and both extreme larger additions. See `docs/research/addition_forest_diagnosis_v1.md`.

A development difficulty probe captures seven of eight severe per-contract underestimates while flagging 66/245 inputs (26.94%), including 50 unresolved ones. This is not fresh-family acceptance or marginal-pair recall.

## Experimental and incomplete work

- Adaptive inference: ordinary mean/p95/p99 69.69/135.15/160.89 microseconds; other calendars 86.56/132.65/139.62. Numerical export checks pass. Accuracy/transfer of the broader API is unaccepted.
- A cheap known-certificate longer-retry cutoff saves 28.31% development retry CPU with the same best recorded bills. C++ policy selection averages 0.513 microseconds excluding features/model scoring. Its prospective test was queued and not run to completion.
- The conditional retry forest improved aggregate calibration but failed useful decision gains and is set aside.
- The `.10` after-repair policy, new-shop transfer panel, and third new-family reference wave were interrupted at the user's finalization request. Completed records are retained; partial aggregates are not promotion evidence. An earlier native assertion in the stage benchmark remains a recorded failure.
- The final seed-suffix lower-bound prototype passes 240 independently engine-verified constructed exact-count cases (2..40 workers) and 1,200 controls. It is not integrated or promoted; broader corpus validation remains required.

See `evidence/FINAL_CHECKPOINT.json` for exact stopped counts. Primary original report files are copied into `evidence/reports/`; full inputs, attempts, schedules and frozen predictions are in the research checkpoint.
