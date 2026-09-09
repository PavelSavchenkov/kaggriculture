# Review 024 — scheduled 2026-09-09 08:56 UTC, recorded 08:57

Decision: continue with the accepted cold-cost component unchanged. Calendar training now has a clear development calibration benefit and a fast, verified C++ implementation. Test its decision benefit and large-workforce calibration before adoption or another prospective freeze. Keep the general estimator and original warm pipeline unaccepted.

Both calendar sweeps are complete: 160 contracts, 32 obligations, 5,379 calls and schedules for 72 contracts. There are 4,442 call observations after necessary screening. H23 has thirteen short-budget certificates and H24 has fifty-nine. Earlier rounds and source certificates improve four terminal physical upper bounds without altering the latest operational outcomes, including one 25-to-13 reduction.

Calendar features alone do not generalize from earliest-hire examples: boosted Brier score on held families worsens from 0.1891 with physical features plus horizon/workforce to 0.3211 with calendar descriptors. With other families' calendar examples added, the calendar-aware boosted score is 0.0710 versus 0.1747 for the same physical-feature control. Every held family improves. Signed success-probability error is +0.0138 overall, -0.0117 at H23 and +0.0240 at H24. CPU prediction remains weaker, with MAE 1.061 seconds and bias -0.461 seconds.

The new development header exports 160 boosted success trees, 120 CPU trees and a logistic control. Full C++ feature/model parity passes 225,000 queries across 1,875 distinct contexts; largest error is below 1.3e-14. Mean/p95 context construction and physical-feature cost is 51.74/89.70 microseconds. Query features plus all three models cost 3.62/6.80 microseconds. Parsing and output are excluded. These checks do not establish a search-speed gain.

The old frozen query model has a severe large-workforce extrapolation failure on novel second-wave layouts: 96.65% mean predicted success at 33–40 workers versus 1.32% observed. The actual rate declines gradually with workforce, and unsuccessful calls generally consume the budget. API validation allows forty workers; do not mislabel this as physical infeasibility or an established 32-worker hard limit. The new training corpus includes full-range addition calls.

Predictions from the new model are now saved for all 34,840 second-wave calls as a post hoc diagnostic. That wave is already exposed and is not a new prospective test. Holdout_b remains unopened. The owned-addition thirty-second round is 452/479 complete and the larger-addition round 436/587; analyze each as soon as complete.

Next priorities: compare new versus old call calibration and decision behavior on the exposed wave; analyze the stronger addition references; then choose a compact set of model/utility candidates for fresh testing. No more generators until these results are handled. Next review 09:16:39 UTC; deadline September 10 00:56:39 UTC.
