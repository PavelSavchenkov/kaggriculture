# Calendar-aware call model, development v2

This model predicts the success and CPU of one proposed cold compiler call. It does not predict minimum labor cost and is not a feasibility test. It is not yet adopted. The accepted ordinary cold-cost component remains unchanged.

The input is the unscheduled physical contract, 23 or 24 real work phases, an explicit menu of mandatory and optional hire slots, and the proposed workforce. `include/planning_context.hpp` represents the menu, and `include/context_query_features.hpp` constructs 320 call features. Unselected slots and the mandatory designation of an identical selected hire set cannot affect the call forecast.

The backend target is unchanged V30 with a nominal three-second cold budget. H23 uses the separate explicit-phase-22-deadline wrapper. It is not the original warm caller's visit reuse plus four-thread repair plus cold fallback. CPU estimates are exponentiated mean log CPU, not expected CPU.

## Evidence

There are 1,739 ordinary development contracts with 31,110 logged calls. After excluding shared-family inputs and applying necessary screens, 25,289 calls are eligible for fitting. The calendar study has 160 contracts from 32 physical obligations, with five hire-menu profiles and both horizons. Its 5,379 calls yield 72 short-budget certificates and 4,442 eligible call observations. All failures remain bounded failures.

The terminal round has thirteen certificates, compared with ten under the first wrapper. Source transfer and earlier rounds improve four physical upper counts, including 25 to 13 workers. They do not alter any short-budget labels. The combined physical evidence is in `runs/calendar_complete_physical_v2`.

All comparisons exclude the complete test family from every fit. They are development comparisons, not a fresh prospective test.

| Training / inputs | Call Brier score | Signed success-probability error |
| --- | ---: | ---: |
| Ordinary examples only, physical features + H + k, boost | 0.1891 | +0.1945 |
| Ordinary examples only, calendar features, boost | 0.3211 | +0.3461 |
| Ordinary + other families' calendars, physical features + H + k, boost | 0.1747 | +0.1603 |
| Ordinary + other families' calendars, calendar features, boost | 0.0710 | +0.0138 |

Adding calendar descriptors without varied calendar examples makes transfer worse. With calendar examples, the boosted model improves Brier score in every held family. H23 score/bias is 0.0425/-0.0117; H24 is 0.0823/+0.0240. CPU MAE remains 1.061 seconds with bias -0.461 seconds, so predicted-CPU query ordering still needs caution and direct measurement.

## C++ checks and cost

`models/context_candidate_dev_v2/context_model.hpp` stores 160 boosted success trees, 120 CPU trees and a logistic control in fixed arrays. `build/predict_context` accepts rows `id problem_path H fixed_count optional_count [fixed h slot pairs] [optional h slot pairs]`, followed by output path and repeat count as CLI arguments.

The check covers 1,875 distinct contexts and 225,000 queries in mixed input order. Largest C++/Python error is below 1.3e-14; the unchanged 1e-7 mixed tolerance passes. Context construction, physical features and bounds cost 51.74 microseconds mean and 89.70 microseconds p95. Constructing a query and evaluating all three models costs 3.62 microseconds mean and 6.80 microseconds p95. These are thread-CPU measurements; JSON parsing and CSV output are excluded.

Files: `runs/context_calendar_transfer_v2`, `runs/context_calendar_augmented_v2`, `runs/context_cpp_check_v2/CHECK.json`. Parity and calibration do not establish an iteration-speed gain. Test query decisions and fresh data before adoption.

## Failure exposed by the old query model

On the second wave's novel layouts, the old boosted query model predicts 96.65% success for 33–40 workers, but only 1.32% of those 1,592 calls succeed. In contrast, its 9–12-worker calibration is much closer. The actual success rate declines gradually: 159/199 at 24 workers, 69/199 at 31, 19/199 at 32, and zero from 37 through 40. Unsuccessful large calls generally use the full budget. The API validation supports up to forty workers; no 32-worker limit was found in that validation. This pattern is consistent with bounded solver scaling, not a physical impossibility claim.

The new fit includes full-range calls from the addition sets. A check on the now-exposed second wave is a post hoc diagnostic only. Keep holdout_b unused for a later prospective test.
