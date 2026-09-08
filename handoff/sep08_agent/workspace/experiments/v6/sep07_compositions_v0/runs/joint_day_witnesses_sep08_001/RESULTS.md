# Exact day scheduling results

45/48 cases solved within a2-second allowance; every returned schedule passed full-engine checks. Median solved time 0.185s. All12 augmented cases with two fewer workers solved.

| Farm/controller | Day | Source moves | Solved moves, more work/two fewer workers | Extra field actions | Hire saving | Tiles served by multiple workers, source → solved |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| joint_routes_p355_m0 | 14 | 138 | 61 | 4 | 89 | 18 → 1 |
| joint_routes_p355_m0 | 15 | 132 | 83 | 3 | 89 | 15 → 1 |
| joint_routes_p355_m0 | 16 | 172 | 72 | 24 | 89 | 7 → 0 |
| joint_routes_p355_m1 | 14 | 146 | 85 | 24 | 89 | 0 → 5 |
| joint_routes_p355_m1 | 15 | 143 | 71 | 13 | 89 | 2 → 3 |
| joint_routes_p355_m1 | 16 | 141 | 89 | 19 | 89 | 0 → 6 |
| joint_routes_p362_m0 | 14 | 153 | 75 | 0 | 233 | 18 → 0 |
| joint_routes_p362_m0 | 15 | 157 | 70 | 2 | 233 | 12 → 0 |
| joint_routes_p362_m0 | 16 | 172 | 84 | 5 | 233 | 10 → 2 |
| joint_routes_p362_m1 | 14 | 152 | 88 | 8 | 233 | 1 → 3 |
| joint_routes_p362_m1 | 15 | 176 | 129 | 2 | 233 | 3 → 7 |
| joint_routes_p362_m1 | 16 | 154 | 122 | 7 | 233 | 0 → 2 |

The p355 joint-route day14 contract recovers20wheat,3milk and2fertilizer plus service while saving89 in hires. Existing transactions stay fixed, so extra products are retained inventory, not extra sale revenue. p362 joint-route day14 recovers8wheat and2fertilizer plus service and saves233.

The three timeouts are all p362 joint-route day16; its augmented/two-fewer-worker case solves. This shows search failure does not establish infeasibility. Retry only those three. New construction is outside the added-work contract.

Position and task-assignment counts from schedules already verified in the full engine. Source schedules contain sanitized accepted actions. Multiple workers on a tile establish cooperation in the solution, not that cooperation alone caused the gain.
