# Review 3 — 2026-09-07 01:47 UTC

Reread the original objective. Previous turn made concrete progress: broader
league, stronger verified public port, replay composition/day library, and
tested C++ biology. Goal remains active; next review 02:07 UTC. No blocking
condition and no GPU need. The economic estimator, compiler and repeating
improvement loop remain the major unfinished requirements.

## Evidence and priorities

Public router won 1,989/2,048 fresh promotion games against the tested teammate
(97.12%, mean margin $7,863.06, lower-decile margin -$26.77). It won 249/256
native-RNG audit games. All 11 current external C++ ports and seven archived
packages are tested: 98.4–100% wins per opponent in 256 discovery games. This
does not prove it beats every teammate variant or every newer notebook.
Preserve it unchanged as a reference while constructing new candidates.

Reference parity now passes for both adapters: 8,628 public-router actions,
7,190 teammate actions. Every embedded route is exercised. The apparent
teammate mismatch was a replay fixture missing shared step in seat 1, not a
policy discrepancy. Explicit hydration fixes the reference invocation.

Extracted 72 dated compositions, 4,488 grouped cohorts, 17,516 individual
lifetimes and 2,160 exact days. Original tiles, turns, player/episode/seat and
submission provenance remain available. A day template requires a compatible
starting state or a complete repair; arbitrary tape splicing is not compilation.

Biology profiles cost about 3.86 microseconds per observed full composition.
128 exact isolated games pass daily output and accepted service/input checks.
This validates the biological calculation under its service contract, not
profit ranking, resource availability, routing or complete feasibility.

## Global invariant comparison

Reference remains the fresh normal-game top 12, 72 games. Recorded animal-days
average 186.1 cows, 159.8 sheep, 35.8 geese; feed+care shares 76.8%, 79.5%, 80.3%.
Fertilizer collection shares 92.7%, 89.5%, 84.9%. Dated population milestones and
life departures stay in invariant_games.csv. Full-service estimated feed demand
is 382.46 wheat/game. This should be compared with actual consumed feed after
input accounting, not gross wheat buys, because own wheat and stock changes
also supply it. Service remains a default with explicit local overrides.

Global crop occupancy is 1,517.7 crop-days/game, watering share 72.2%, and
water+active-fertilizer share 34.9% of yield-relevant days. At the recorded dated
lifetimes, no-fertilizer biological harvest/game is wheat 471.5, carrot 94.8,
tomato 17.3, strawberry 128.6, melon 76.8. With productive-day fertilizer it is
728.3, 129.3, 34.6, 257.1, 76.8, respectively. Actual global harvest is 504.1,
99.3, 31.3, 237.2, 76.1. Thus blanket fertilizer is a poor economic rule:
ongoing crops need much of the extra yield, while wheat/carrot require selective
valuation. The all-fertilizer profile consumes 307.25 fertilizer/game, which
cannot be treated as free even when animals produce it.

Full-service biology gives eggs 61.2, milk 216.5, wool 180.4 and fertilizer
366.0/game. These are potential collected outputs under the stated service
policy. Global net sales are eggs 51.6, milk 198.4, wool 158.1, fertilizer 239.4;
do not compare these as identical metrics. Bought fertilizer, field use, held
stock and discards must explain the output-to-net-sale gap. Gross buys in the
global sample are wheat 326.3 and fertilizer 57.4/game, while net wheat sales
are 184.3. Gross turnover does not establish production strength.

Global labor: 286.3 hires/game, $6,122.8 cost; extra land 2.014 quadrants.
Biology requires 3,354 field operations/game without fertilizer and 3,662 with
it. Travel, seed/animal delivery, pickup/deposit, weeding and hire timing are
still absent. Coarse peak occupied counts average 86.6, above three quadrants'
75 slots: intraday harvest/replant reuse and exact lifetime boundaries must be
handled before calling such observed strategies infeasible or buying extra land.

Global discards average 7.81; the new router's discovery mean was 2.25 under
different opponents/shops. Trade timing and capacity pressure are recorded in
the replay events; capacity is brief rather than constant. Local per-day
service, net flows, timing and weed-cost measurements remain incomplete. They
are the next instrumentation task, not grounds for assuming parity with leaders.

## Design decisions

Proceed with a fast economic model and a compiler in parallel stages of the
same implementation effort. Keep daily biological calculation cheap. Add
intraday distinctions when evidence requires them: same-day tile reuse, opening
cash, earliest deposits, and expiration-day salvage. Do not solve every exact
worker route at every outer search node. Keep both individual lifetimes and
maintained product-count intervals (which expand into repeated crop lifetimes).

Prioritize cold composition realization and estimate/actual calibration now.
Further notebook repairs remain useful bounded experiments, but collecting
more variants of one route cannot replace the requested improvement loop.
