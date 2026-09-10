# Waiting within the same day

Experimental extension of ../history_course with the same external Atakan fixed farm, compiler and purchase repair. Fresh confirmation on 9,216 paired games gives +$336.04 mean margin over purchase repair and +$305.80 over one-turn timing, with no sampled losses or checked noncash changes. See ../../runs/history_course_day_confirmation_v1. A separate frozen public confirmation gains $154.14 on 42 player plans with fixed rival orders. The default API agent uses course 0; the evaluator also tests courses 1 and 2.

When public history rules out stored rival output and none is harvestable now, that output cannot be newly produced before the next night under standard game rules. The planner tests later sale turns immediately following known consumption within the same day. It checks the supplied resource calendar and other stock, seeds, hires and carried resources at every intervening turn, plus all ending private stock and shared market inventory. Other rival products are not simulated, so full-game funding and state checks remain necessary.

Accepted changes update an explicit future market-order plan; later decisions retain those commitments. The planner evaluates at most 1,000 financial turns per call. This is a computation budget, not an arbitrary holding-duration parameter. The horizon ends before the next day because the no-rival-output argument ends then. Previous one-turn agents remain separate controls. No root change or submission.

Generic and typed debug pair agree on 16 full-budget PASS games; full self-play and zero-budget matches pass. See ../../runs/history_course_checks_v0/DAY_CHECKS.json.
