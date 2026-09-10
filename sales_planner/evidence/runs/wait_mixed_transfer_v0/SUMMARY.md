# Guarded one-turn waiting across other farm plans

The frozen extension covers 138 player calendars from 69 already exposed episodes, excluding the three Otter study episodes. It keeps the same one-turn rule and public-ready guard. No thresholds are tuned on this result.

Both financial calendars remain feasible in all 138 guarded cases. The full engine reproduces every financial cash result. Mean margin change is +$13.48, with a 95% episode-cluster bootstrap interval of -$5.92 to +$32.46. There are 83 positive, 51 negative and four unchanged results; worst margin -$528, best +$661. Own cash averages +$14.50. The interval does not establish a mean margin improvement, and related source families limit generalization further.

The strict noncash-state check fails in two cases. Episode 107270694 seat 1 first differs in seed stock after turn 164, while production, work outcomes and terminal shed stock still match. Episode 107275575 seat 0 finishes with one extra wheat in the shed; its original terminal shed is empty. Keep these results in the record, but do not call all 138 cases identical-state finance improvements. Other production, discards and action outcomes match. See ENGINE_CHECKS.json and state_diagnosis files.

Four original episodes request actions for inactive workers. The game accepts these raw replay actions, while the local C++ agent API requires exactly one action per active worker. The initial strict run stopped in the original baseline. The recorded-source rerun preserves every original worker action and checks that no new worker-count discrepancy appears. These are explicitly counted source-format exceptions, not validation of a deployable agent. See SOURCE_ACTIONS_NOTE.md.

Without the public-ready guard, eight of 138 cases violate inputs or commitments. Its all-case financial mean is -$362.45 and is not a valid quality estimate over executable schedules. Do not report a filtered success-only average as promotion evidence.

## Main negative learning

Visible ready yield is not an inventory estimate. Episode 107201424 seat 1 delays 12 melon at turn 264. Its no-rival projection expects +$23 cash; the actual two turns give own -$295 and rival +$297. The rival can sell previously harvested product despite no recent ready crop. Episode 107197409 seat 0 similarly delays six strawberry at turn 668: prediction +$22, actual own -$183 and rival +$205.

Next hypothesis: retain public evidence of past harvests and infer rival sales from market history, with uncertainty where flows cannot be identified. Do not infer private inventory from current ready yield alone. Keep hidden inventory solely for evaluating and diagnosing that belief.

Decision: preserve the strong agents; no broad promotion of waiting and no arbitrary hold-duration tuning. Otter’s local gains remain valid for its own three checked histories. Reusable finance still needs better rival sale-risk information and continuation checks.
