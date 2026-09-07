# Review 15 — 2026-09-07 05:47 checkpoint, recorded 05:51

Original objective reread at 05:48. Next review 06:07. Goal active; no GPU need.
The new work exercises the requested estimate/compile/exact feedback loop on
service and workforce, with original composition held fixed. General composition
insertion/deletion, cold rebuilding and adaptive suffix search remain active
goals; this local compiler improvement does not replace them.

## Service defaults and exact compiler evidence

src/improve_care.cpp extracts successful dated animal service from an exact
discovery game. Its isolated marginal model checks every baseline animal
harvest and morning state against the source. It models old versus newly banked
care, held caps, later harvests, unfed production and escape. A separate full
conditional replay tests resulting production, sales, cash, discards and stock
under fixed future source/rival actions. This forecast relaxes labor and knows
the sampled continuation offline; it is neither an executable policy nor a bound.

Program55 has eight fed-but-uncared opportunities, program78 eight, and new
SpaTaro90/kwa100 none in usable nonoverflow days. All sixteen have zero marginal
harvest and zero conditional full-game cash. Fifteen occur on day28; program55's
other is a goose on day20. The cheap screen prunes them before exact solving.
Program55's eight biological checks take 33 us total, versus expensive day solves.

Removal audit checks 327 existing care visits in 122 us total; 70 can be removed
without losing a future harvest under the source calendar. Twelve high-hire-cost
attempts also remove the final daily hire: ten solve and all ten pass exact
modified endpoints/cash. Each saves $144 and preserves all production in eight
live-opponent games. These are local exceptions, not a reason to abandon a
productive service default. Cases where losing output raises own cash but raises
opponent cash more are explicitly excluded by the zero-output-loss filter.

The causal control keeps every service and removes only the last hire. Across
26 usable days, V30 returns 16 schedules; 15 pass the full endpoint/cash check.
Day6 fails and is rejected; ten are UNKNOWN at the two-second soft budget, with
some overruns up to 4.68 s. Four late days save $144, two $89, two $55, and early
days $3–5 in eight full games. Day9 preserves production but loses $95 cash on
average across those games despite matching the source endpoint locally.
On day18 the care-removal problem solves while the all-service control is
UNKNOWN; this is not proof that the control is infeasible. Most measured savings
come from better routing, without needing to remove care at all.

## Actual league combination search

src/combine_days.cpp sequentially adds compiled complete days to v2's delayed-
hire branch, testing 64 common games against each of v2/public/Mao85/Junghoon78.
Day0 and day18 are excluded because v2 already changed their economic components.
Its conservative gate requires nondecreasing mean margin in all four matchups.
It retains days21,23,11,13,14: five fewer hires, $487 saved in healthy games.
The exported combine_hires_001_best needs required package checks and a new
independent gate; v2 remains the retained incumbent until then.

Failed combinations are informative: days3/5 badly worsen the already fragile
Junghoon realization, while day1 cuts its mean deficit from $54,528 to $13,452.
The strict gate rejects day1 for about $6.5 lost versus v2/Mao despite a large
aggregate gain. Retain that related alternative and test broader scenarios;
do not silently make the strict gate a universal objective. Exact trial outputs
retain all parents and allow causal reconstruction. Junghoon still wins all
initial games, so this is a reduced gap, not a solved counter.

## Global invariant audit

Rechecked the independently selected 05:04 SpaTaro/kwa cohort and replay
reconstruction. Added missing no-effect action counts and successful weed digs
to the analyzer/review. All twelve games still reconcile cash exactly. The
cohort has an average 6195 non-PASS requests and 22.08 weed digs. Deliberate
fault fixtures caught a list/tuple position comparison error that suppressed
the first fault count. Fixed, all five fixtures pass, and exact recomputation
gives 5.83 ineffective actions per game. Cash reconciliation remains exact.
Original raw replays remain the source witnesses. Earlier unit_events included
successful operations only, so the previous claim that they contained faults
was corrected in review14.

Full milestones/service and item flows remain in review14 and the refreshed
invariant_teams.csv. Cow/sheep-days 164.75/217.25; no geese. Cows grow 2→6.08
by day11, finish5.92; sheep2.5→8.42 by day11, finish8.0. Feed/care .772/.860
for cows and .821/.901 for sheep, collection .942/.935. Crops occupy1446days,
water.711 and productive fertilization.323. Harvest wheat489, carrot154,
strawberry205, melon80; no tomatoes. Wheat buys292/sales465/net173; fertilizer
buys4.25/sales232.5/net228.25. Net milk153, wool209, strawberry204, carrot152,
melon78.5. Buy/sell mean hours2.45/4.96. Hires287/$4858, land2/$3000,
discards6.25. These independent global patterns still contrast with healthy
source55's higher goose use and wheat buying, lower productive fertilization,
280 hires/$5400, and42 ineffective actions in local games.

New local variants preserve dated animal/crop services and all eight-game output
arrays; their benefit is saved labor cost, not extra production or inflated
transaction counts. Full combined profiles against v2 and the new source-family
counter are next. The original fast-estimation idea now has a concrete cheap
service pruning success, but worker feasibility and adaptive funding remain
the much larger estimation/compiler gaps. Prioritize the runnable combination
gate and financing alternatives, then return their diagnostics to composition
search and labor estimation.
