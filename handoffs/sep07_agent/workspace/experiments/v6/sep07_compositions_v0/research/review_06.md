# Review 6 — 2026-09-07 02:47 UTC

Original wording reread at 02:46. Goal active; next review 03:07. The pipeline
now compiles a dated composition from an empty farm, but realization is weak.
The strongest complete reference remains public_router. No new promotion.

## Common-seed causal evidence

All rows use seeds 1000–1003, both seats, public_router opponent, exact 719-turn
games and validated action metadata. Cash alone is not the league objective.

| Compiler configuration | Mean cash | Mean margin | Wins |
| --- | ---: | ---: | ---: |
| Initial productive service | 65,112 | -31,019 | 0/8 |
| Source service calendar | 31,327 | -118,397 | 0/8 |
| Consistent expansion wheat reserve | 66,607 | -32,683 | 0/8 |
| Three-day surplus reserve | 64,041 | -36,306 | 0/8 |
| Source calendar + consistent reserve | 23,631 | -131,735 | 0/8 |
| Source calendar + reserve + flexible hiring | 63,883 | -32,809 | 0/8 |
| Productive default + reserve + flexible hiring | 64,566 | -31,296 | 0/8 |
| Exact source action trace | 80,315 | -7,693 | 2/8 |

The original default and source-calendar configurations are exactly reproducible
in the new generic runner. Their full game results match the earlier binaries.
The source trace matches all 719 original actions, with worker count normalized.
It reconstructs one observed course, not the source player's branching agent.

Two implementation faults were exposed. Buying feed for unplaced animals but
reserving only the placed herd caused same-turn sell/buy churn. Fixing that
accounting did not by itself improve margin. Also, a $60 hiring reserve blocked
even $1 workers when the opening left less than $60. Removing that reserve
rescued the source-calendar variant and restored all 319 recorded hires, but
does not establish a universally best cash reserve. Keep both ablations.

## Global and same-composition invariants

The independent global reference remains the 72 current top-12 games, not this
local opponent. The source-calendar compiler with flexible hiring has 1,481.8
crop-days versus global 1,517.7 and same-composition trace 1,656.4. Watering is
61.9% versus global 72.2% and trace 65.1%; productive water+fertilizer 49.7%
versus global 34.9% and trace 53.1%. Its wheat/carrot/tomato/strawberry/melon
output is 418.3/63.3/67.9/276.3/60, while the trace gives
555/111.6/85.9/310/72. Thus more fertilizer coverage recovers some output,
but missed placements, deadlines and routes still dominate several crops.

Compiler cow/sheep/goose days: 149/122.6/57.1, versus global 186.1/159.8/35.8
and trace 146.6/124/58.3. Five opening cows are established, but one game loses
one later. Sheep reach five by day 11 instead of trace day 8; geese reach three
by day 11. Compiler feed+care shares: 52.4%/45.5%/47.9%, versus global
76.8%/79.5%/80.3% and trace 59.1%/58.9%/70.9%. Fertilizer collection:
79.1%/78.9%/58.7%, versus global 92.7%/89.5%/84.9% and trace
96.6%/95.2%/93.4%. The source itself uses less animal service than the global
mean; preserve that distinction instead of imposing a universal care quota.

Compiler egg/milk/wool/fertilizer output: 53/127.3/66.1/248.1, versus trace
82.6/139.5/98/314. Gross wheat/fertilizer purchases: 136.6/119.8, versus trace
66/48 and global 326.3/57.4. Net wheat/milk/wool/fertilizer sales:
204.3/124.8/63.9/104.5, versus trace 331.6/139.5/98/144.6 and global
184.3/198.4/158.1/239.4. Source-calendar gross wheat purchases are much lower
after consistent stock accounting; that is not automatically a production gain.

Compiler average wheat/fertilizer purchase hour: 12.7/6.1 versus trace 1.4/0.
Wheat/milk sale hour: 8.1/8.0 versus trace 11.3/15.2. The borrowed trace funds
and supplies early work; the compiler often buys after the service deadline is
already difficult. Timing belongs in estimation and execution, as the user
emphasized. Direct cash comparison across the original replay's different
opponent and current exact scenarios would not isolate this effect.

Compiler and trace both hire 319 hands for $8,358 and buy two extra quadrants;
global means are 286.3/$6,122.8 and 2.01. Compiler discards 31.8 versus trace
0 and global 7.81. Successful weed digs 41.3 versus trace 7.1; compiler invalid
unit commands 0 versus trace 26.1. Zero invalid commands does not imply good
scheduling. Compiler shed maximum 86.3 and 2.5 turns/game above 90; trace
94.9 and 3.75 turns. Short peaks still cause material overflow. Global detailed
weed/fault attribution remains incomplete.

## Priorities and original-intuition coverage

Integrate biology, placement/workforce and conditional market estimates now,
then compare predicted output/resources with these exact compiler profiles.
Do not let individual reactive-job tweaks replace the outer composition search.
The initial estimator may use recorded support and fixed observed opponent
flows as declared approximations, with cheaper independent alternatives exposed.
Need measured ranking quality before treating estimated scores as strategy value.

For compilation, use full borrowed day routes as an available realization
method, while developing routes for new compositions. The source trace shows
substantial recoverable output without changing the composition. Preserve cold
construction, family insertion/removal and full backbone replacement; these
have not yet been demonstrated. Shop/opponent-dependent suffix search and the
repeatable growing-league loop also remain required. Continue the public-agent
and teammate audit alongside these priorities; no GPU workload is justified.
