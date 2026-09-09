# Frozen forecasts on new asset additions

These are new physical variants from eight exposed development families. Forecasts preceded the reference calls. They are not an untouched-family test. Every reference call used the unchanged V30 solver with a three-second budget and strict replay. Unknown results remain censored.

Owned-tile additions: 228 distinct contracts, 204 verified upper bounds. There are 189 distinct parent/candidate pairs, of which 168 have both short-budget certificates; 21 remain censored. Equivalent variant names are retained without counting them twice.

| Model | Marginal MAE | Signed bias | Error at most -500 |
| --- | ---: | ---: | ---: |
| Original flat10 | 82.86 | -9.70 | 1.79% |
| Frozen direct-cost forest | 64.47 | -51.23 | 1.79% |
| Frozen timing + boost | 65.21 | -39.30 | 1.79% |
| Original geometry | 554.91 | +482.23 | 1.19% |

The direct forest reduces mean absolute marginal error by 22.19%; the paired family-bootstrap interval for saved absolute error is [12.74, 23.37]. However, all three reference increases of at least 500 are badly underestimated: direct-forest MAE 1,924.05 versus flat10 1,852.67. Two are incorrectly scored as savings. Those three examples come from only two parents.

Larger additions, mostly new land: 245 distinct contracts, 180 verified upper bounds. Of 197 parent/candidate pairs, 134 have both short-budget certificates and 63 remain censored.

| Model | Marginal MAE | Signed bias | Error at most -500 |
| --- | ---: | ---: | ---: |
| Original flat10 | 210.17 | +35.96 | 3.73% |
| Frozen direct-cost forest | 149.39 | -143.84 | 8.96% |
| Frozen timing + boost | 81.04 | -51.27 | 2.24% |
| Frozen timing + extra trees | 95.25 | -72.02 | 3.73% |
| Original geometry | 2,469.80 | +2,469.68 | 0% |

Timing + boost reduces marginal MAE by 61.44%, with a family-bootstrap saved-error interval [104.22, 161.74]. On the 17 increases of at least 500 from eight parents, its MAE is 319.86 versus flat10 583.06 and direct forest 702.65. Underestimation remains its main error direction.

These targets are differences between cheapest short-budget certificates, not proven minimum bills. Three-count, 30-second adjudication rounds were selected uniformly for all eligible cases after these reports. They can tighten physical evidence but cannot revise the original operational outcomes or frozen forecasts.

Decision: do not accept the direct forest from its average gain. Keep the formula-plus-residual approach prominent; compare physical and fixed-budget targets, calendar context, and expensive-error slices before another freeze. No full-pipeline speedup follows from these accuracy results.

Evidence:

- `runs/expansion_owned_dataset_v2/DATASET.json`
- `runs/expansion_owned_pairs_v2_complete/RESULTS.json`, `ERRORS.jsonl`, `SLICES.json`, `CENSORED.json`
- `runs/expansion_owned_evaluation_v2_complete/RESULTS.json`
- `runs/expansion_dataset_v1/DATASET.json`
- `runs/expansion_pairs_v1/RESULTS.json`, `ERRORS.jsonl`, `SLICES.json`, `CENSORED.json`
- `runs/expansion_evaluation_v1/RESULTS.json`

Two earlier owned-report directories lack results because their invocation passed a directory instead of `PREDICTIONS.jsonl`. The completed report directories above contain the successful runs. Forecasts and source data were unaffected.

## Stronger owned-addition physical evidence

The completed 479-call, thirty-second round lowers 53 workforce upper bounds and adds no new certificates. Against these stronger physical bounds, frozen direct-cost marginal MAE is 42.51, bias -22.30, versus flat10 MAE 67.73 and bias +19.23. Timing + boost has MAE 40.41 and bias -10.37. These changes reflect better reference schedules; the estimator forecasts and original short-budget outcomes are unchanged.

Three direct-cost errors still exceed 500. For `2a22298396122ce9ecad`, the parent upper falls from 13 to 12 workers and candidate upper from 18 to 16, reducing the reference increment from 3,804 to 1,364; prediction remains -0.36. Two candidates of parent `0af7724e39909de27388` retain increments of 987 and predictions -3.14 and +9.35. These few cases remain important despite the improved mean.

Evidence: `runs/expansion_owned_dataset_deeper_v3`, `runs/expansion_owned_deeper_pairs_v3`, and `runs/expansion_owned_deeper_evaluation_v3`. Keep these physical-evidence reports separate from the initial operational-target reports above.

The larger-addition thirty-second round also completed: 587 calls lower 49 upper bounds and add no new certificates. On the same 134 known pairs, frozen timing + extra trees has MAE 49.65 and bias -9.75; timing + boost has MAE 59.39 and bias +11.00; direct cost has MAE 90.77 and bias -81.57; original flat10 has MAE 175.56 and bias +98.23. Evidence: `runs/expansion_dataset_deeper_v2`, `runs/expansion_deeper_pairs_v2_complete`, and `runs/expansion_deeper_evaluation_v2`. The earlier `expansion_deeper_pairs_v2` invocation failed because it named a nonexistent index file; its corrected successor contains the results.

A guided offline round then chose three previously untried longer-budget workforce counts for each of 24 unresolved owned-addition cases, ranked by the new query model and spread by at least three workers where possible. Its 72 calls added three verified upper bounds. These are reference-allocation gains, not estimator-accuracy or online-search gains; the selector used an exposed development model. Original short-call records stay unchanged in `runs/expansion_owned_dataset_guided_v4`.

Including the newly certified owned cases produces 170 labeled marginal pairs and nineteen censored pairs. Direct-cost MAE rises to 122.12, bias -101.15, versus flat10 MAE 147.23. One newly labeled crop addition has a reference-bill increment of 13,530, predicted as -4.14 by the direct model. Its interval from necessary lower bounds and feasible upper bounds is [-4,168, 17,698]; the minimum marginal cost is not identified. This case must not disappear when reporting the earlier improved mean. See `runs/expansion_owned_guided_pairs_v4`.

The corresponding guided larger-addition round completes 195 calls on 65 unresolved contracts, adding three certificates. Two new labeled pairs share a 16-worker parent with reference bill 1,596. The eight-crop addition has a 21-worker upper and reference increment 16,114, predicted as 268.91. The ten-crop addition has a 28-worker upper and reference increment 512,632, predicted as 269.97. Their physical marginal intervals are [-1,576, 17,698] and [-1,563, 514,216], respectively. These very loose upper bounds do not establish minimum labor costs.

With those pairs included, the larger panel has 136 labeled pairs and 61 censored. Direct-cost MAE is 3,973.31 and bias -3,964.25; flat10 MAE is 4,056.85; timing-plus-extra MAE is 3,930.11. The 512,632 upper-bill increment dominates every method's average. Preserve the full results, the previously labeled cohort and these new hard cases as distinct slices. Evidence: `runs/expansion_dataset_guided_v3`, `runs/expansion_guided_physical_pairs_v4`, and `runs/expansion_guided_evaluation_v3`.

The first invocation named `expansion_guided_pairs_v3` omitted the physical target flag and correctly recomputed the unchanged short-sweep target. Its scope string was too broad; `CORRECTION.json` points to the physical report. No forecasts or solver records changed.

The unchanged family-exclusion difficulty probe catches seven of eight per-contract underestimates of at least 500 on the updated larger panel while flagging 66/245 contracts (26.94%). Fifty flagged contracts still lack a verified upper bound. This uses probability below 0.2 at the direct point workforce; peak below 0.5 alone catches only four of eight. The probe catches the newly resolved extreme cases. The unflagged labeled cash MAE is 45.39. This is a development per-contract diagnostic, not marginal-pair recall or a new-family acceptance result. Evidence: `runs/expansion_guided_probe_risk_v4`.
