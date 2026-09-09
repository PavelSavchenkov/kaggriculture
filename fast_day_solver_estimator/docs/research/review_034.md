# Review 034 - scheduled 12:16 UTC, recorded 12:25

Continue the frozen third-wave and two warm-seed tests. Process inspection at 12:23 confirms all three reference runners, both paired warm runners and both report queues remain live. No partial outcome aggregates have been read for model selection. The previous review time was missed during the retry study; retain the original twenty-minute schedule, with the next review at 12:36:39 UTC.

The conditional retry audit is complete: 1,333 exact matched three-to-thirty-second attempts, 440 physical contracts, 1,300 exact query inputs and no conflicting outcomes among 33 repeated inputs. There are 543 separate contract/round decision states, including 365 with a prior certificate and 178 without one. Historical rounds remain separate; they are not independent samples of an online trajectory.

Whole-family exclusion gives retry Brier 0.05832 for the physical/calendar/prior-certificate forest, versus 0.09248 for the old three-second probability control. However, the main gain is in cases without a prior certificate. On the 799 calls with a prior certificate, the new forest is slightly worse: 0.08578 versus 0.08318. A post hoc rule that multiplies the old probability by 0.1 only when no certificate is known reaches 0.05680 overall, better than the new forest. This rule is development evidence, not a prospective calibration result.

Decision: do not export another model merely for its overall Brier improvement. First compare choices under one, two and three long-call allowances, with recorded backend CPU and verified bills reported separately. Include cheap workforce-order controls and the corrected old probability. Selection must use only frozen predictions and the caller's currently known certificate. Preserve losses from probability cutoffs explicitly. New expensive retry calls remain deferred until there is decision evidence and free reference capacity.

The accepted component is still limited to the second-wave cold H24 direct-cost search. No general-calendar, addition, warm or conditional-retry improvement is accepted yet.

Next review 12:36:39 UTC. Deadline September 10 00:56:39 UTC.
