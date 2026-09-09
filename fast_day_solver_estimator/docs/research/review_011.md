# Review 011 — 2026-09-09 04:36 UTC

Decision: continue, but keep routing optimization behind an evidence gate. Its added cost is measurable and its held-family workforce accuracy is not consistently better. Test its effect on layout choices and marginal costs before making it part of the default estimator.

Evidence:

- The 244-feature routing study is complete. Routing residual models improve a few family folds slightly but regress others; the routing formula is essentially unchanged from the timing formula. Layout and changed-work decision studies are now running.
- The independent synthetic sweep is complete: 289 contracts, 170 verified upper bounds, 35 proven unreachable by the existing output-deadline screen, and 84 unresolved. The frozen v1 predictions were stored before this sweep; evaluate those first. Late input releases are an important missing screen and may account for some unresolved cases, but this has not yet been established.
- Terminal-day sweep is complete: 31 contracts, 25 verified upper bounds, six unresolved. These use 23 real phases and a separate verification wrapper; they are not ordinary-day labels.
- Longer layout adjudication improved eight existing upper bounds and supplied two new ones. The particularly expensive Tarang original layout still has no improved certificate; one formerly unresolved sibling now has a 15-worker certificate. Keep original three-second operational outcomes separate from these improved reference labels.
- The new route feature passed answer-workforce and tile-index invariance checks on 2,595 historical inputs. Its cost remains roughly 0.5–0.65 ms on the current repeated-input measurements.

Priorities for the next 20 minutes:

1. Inspect the independent synthetic signed errors and failures, not only aggregate accuracy.
2. Add explicit baseline-cost, work-size and direction slices to marginal reports.
3. Prove and test a cheap necessary input-release condition before using it to screen or train. Purchases enter the shed after worker actions; carried inputs cannot be transferred between workers away from the shed. Count all possible local production as immediately available to keep the bound conservative.
4. Use routing decision results to decide whether to retain, gate or drop the heavier feature. Do not confuse improving source-day prediction with improving the original optimization pipeline.

Holdout_a and holdout_b remain unopened. The original pipeline end-to-end comparison is still outstanding. No estimator is accepted yet. Next review 04:56:39 UTC; deadline September 10 00:56:39 UTC.
