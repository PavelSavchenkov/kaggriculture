# Review 11 — scheduled 04:27 UTC, completed 04:43 UTC

Original objective reread at 04:39:56 after context continuation. The review
was late; retain the next scheduled review at 04:47 rather than silently
claiming the 20-minute cadence was met. Goal remains active to September 8
00:47 UTC. No GPU workload is needed.

## First compiled local schedule improvement

advance_sales_001 proposes 21 earlier noninput-product sales cheaply, sends
the best 16 to V30 with a two-second budget, and obtains two solved schedules.
The other 14 are UNKNOWN, not proven infeasible. Edits preserve composition,
ordered tile work, purchases, hires, land and the daily biological endpoint;
the complete worker day is rebuilt to meet the new withdrawal deadline.
Every solved schedule is checked in the exact engine, then in eight complete
live-opponent games. This implements a useful part of the user's proposed
cheap estimate → exact scheduling → realized economics feedback loop.

advance_sales_001_7 moves program 55's day-18 sale of 14 strawberries from hour
22 to hour 18. Estimated local gain $119; realized conditional own gain $100,
margin gain $219. Solver takes 0.411 seconds. The eight discovery games gain
$80–405 cash and $183–894 margin, with identical production. Proposal 9 moves
day 17's ten strawberries from hour 21 to hour 18; gain is smaller. Full
artifacts, generated C++ packages and failures remain in runs/advance_sales_001.

On 2,048 new games per opponent, seeds 400000–401023 both seats, proposal 7
beats parent feeltheagi_55 in 95.46%, mean margin $208 and lower-decile margin
−$1,512. It wins 2,047/2,048 versus public_router, mean margin $13,329.
The negative tail against its parent is real; no pointwise improvement claim.
Old Skomuro/Deniz/Arman counters still defeat this unbranched course frequently.

opening_router_v1 retains v0's public turn-1 worker-count selector, replacing
only its program-55 branch with proposal 7. Independent seeds 450000–450511,
both seats: 989/1,024 wins against v0, mean margin $227, lower-decile −$1,130.
All 1,024 wins each versus public_router, Skomuro, Deniz, Arman, teammate
shoprouter and robust sixday. These are seven declared opponent comparisons,
not universal dominance. New Mao and 3정훈 courses still require v1 testing.
An initial invocation misspelled two catalog names; no result came from that
invocation, and the remaining four batches ran with the actual names.

Generic four-thread and debug pair one-thread actions/results match in 16
complete games, for both proposal 7 and v1. PASS and self-play checks pass.
PASS mean cash $159,102; this is slightly below the parent's $159,119 on the
same small batch. Preserve the measured PASS J rather than treating a league
margin gain as a PASS improvement. All final 900000+ audit seeds remain unused.

## Deep global and paired invariants

The 32 paired profiles versus public_router have identical crop/animal output,
discards, daily lifetimes and service masks, input purchases, hires and land.
Intraday harvest/plant times change, as intended; compare daily life contracts
after sorting, not insertion order or raw transition timestamps. Mean unit
faults fall from 42 to 41. Trading and worker timing explain the gain; it is
not extra production or extra gross volume. check_opening_v1.py preserves the
causal comparisons in opening_router_v1_validation.json.

V1 has 1,523 crop-days, 71.1% watering and 21.7% productive fertilization;
global original/fresh cohorts have 1,518/1,547 crop-days and 34.9%/33.1%
productive fertilization. Cow/sheep/goose days remain 183/121/108, feed+care
84.2%/88.4%/84.3%, fertilizer collection 95.6%/92.6%/92.6%. Cows rise 2→3→4→7
on days 0/2/5/8, sheep reach five by day 11 and finish at three, geese reach
five by day 11. The fresh global mix is less goose-heavy and spans distinct
Howard, 3정훈 and Mao regimes; these remain alternative composition targets.

Output wheat/carrot/tomato/strawberry/melon remains 457/115/0/262/72;
egg/milk/wool/fertilizer 143/211/138/387. Wheat/fertilizer buys 355.5/50.94;
net wheat/strawberry/milk/wool/fertilizer 85.06/261.25/211/138/312. This differs
from fresh globally selected wheat output 557, wheat buys 220 and net milk/
wool/fertilizer 217/160/251. More wheat buying here is not more farm output.
Worker hiring remains 280/$5,400, two land purchases, 7.69 discards, 21 weed
digs. Fresh global means are 287/$5,962, two land purchases, 11.67 discards.
Full purchases/sales, net flows and average hours are in v1_profile.profile.*;
strawberry sale timing is the targeted change. Fertilization and composition
mix remain possible improvements with their full input/labor dependencies.

## Audit and priorities

Three six-day source-faithful C++ modes are complete and lose to our strongest
references; their collapse/reserve diagnostics remain in review 10. Kaito v58
has ten controller slots and nine distinct complete routes, adaptive market
flow estimates, a short sale-advance horizon, weed transaction repair and
public-capital checkpoint routing. Its active logic is now fully mapped, but
the C++ port and source parity are still pending. Complete those next, then
use its strongest distinct behaviors in the growing league.

The original composition compiler still loses badly despite an actual 583-
proposal outer search. Keep its cold/family-edit coverage, fix run export
collisions before repeating, and extend exact day compilation to service/
composition edits. A successful sale edit does not establish that arbitrary
composition compilation is solved. Fresh top-player counters and cheap labor/
deposit timing calibration remain priorities alongside the notebook audit.
