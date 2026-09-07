# Review 10 — 2026-09-07 04:07 UTC

Original objective reread at 04:04:45. Next review 04:27 UTC. The work remains
composition-first, with a useful new exact scheduling interface and a deeper
teammate audit. Neither the weak greedy compiler nor the new ports are promoted.

## Exact scheduling now connected

src/rebuild_days.cpp links day_solver V30 directly through C++; root and solver
engine headers match byte-for-byte. For program 55 versus public_router on
discovery seed 1000 seat 0, it extracts semantic day contracts from successful
tile work, purchases, hires, land, sale deadlines and exact day-end states.
The original schedule is checked against each contract before solving. No
worker routes are given to the solver. Output routes are replayed in the full
engine with the original accepted markets and recorded rival course.

Results/day_rebuild_55_v2: 26 source contracts valid, three days skipped for
source overflow; 21 solved, and all 21 preserve full-game cash, land, shed,
seeds, production and next-morning tiles. Solved median 0.256 seconds. Five
days remain UNKNOWN at the two-second soft budget; one takes 3.37 seconds.
This is conditional day reconstruction, not a complete adaptive policy or a
claim that V30 handles capacity/cash. Final day 29 has only 23 actions and is
not yet handled by this adapter.

Two concrete dependency lessons: keep accepted quantities when replacing a
worker route, because a previously failing oversized order can start executing
more goods after the route changes. Also collapse same-hour buy/sell round
trips only in the physical scheduler's warehouse contract. Workers cannot act
between those market slots. Preserve the original ordered trades for prices
and cash. This resolves the opening wheat loop while keeping physical and
economic consequences distinct. V0/V1 failures are preserved alongside V2.

## New teammate source audit and causal comparisons

Ported three six-day modes: yhay81 published backbone, teammate sixday-rl with
its additive selling head, and sixday-robust-rl with $150 purchase reserve and
floor guard. Each matches 9,347 original-package actions, including five forced
branch/low-cash sequences. Generic four-thread and debug pair one-thread
comparisons match; PASS and self-play checks pass. Immutable decoded tables
are shared, route state is per instance, and no hidden opponent state is used.

All comparisons below use 256 games, seeds 1000–1127, both seats. Against
public_router, win rates are 6.25% published, 10.16% teammate head, and 8.59%
reserve. Against teammate_shoprouter, feeltheagi_55 and opening_router_v0,
all modes lose every game. Original modes collapse to mean cash about $29–34;
reserve recovers $60–63k but still loses. Against skomuro, original modes win
256/256; reserve only 67.97%. Against Deniz, published/head/reserve win
86.72%/81.25%/61.72%; against Arman, 94.92%/94.53%/71.88%. The guard restores
production in one response family and damages others. A newer variant is not
automatically stronger; keep both behaviors as distinct league members.

Source paths, licenses and hashes are in each package and LINEAGE.md. The
remaining Kaito v58 source is now recursively unpacked without execution:
complete routes and readable controller modules in research/kaito_v58/. It
uses ten named controller slots, with two routes byte-identical, public-state
checkpoints and adaptive market-flow selling. Port and independent tests remain
pending; its known-stream headline does not establish fresh strength.

## Deep invariant comparison

Global targets still come from the independently selected 72 original and 18
fresh top-player games, not the convenient league. Review 8 gives full family
profiles and the 03:14 leaderboard. Fresh cohort means: 1,547 crop-days,
68.5% watering, 33.1% productive fertilization, 287 hires/$5,962, two extra
quadrants, 11.67 discards; wheat harvest 557 and buys 220; net milk/wool/fert
217/160/251. Distinct Howard, 3정훈 and Mao regimes remain covered as proposals.

The first search winner, program 4 through the greedy compiler, hires 317/$8,269
and buys two quadrants but produces only $64,119 against public_router. It has
1,359.5 crop-days, 62.5% watering, 44.6% productive fertilization, 214 cow-days,
95.9 sheep-days and 60 goose-days. Feed+care rates are 51.2%/47.6%/55.7%,
fertilizer collection 77.8%/73.3%/61.0%. Cow count rises 5→6→8 by days 0/8/11;
sheep 1→2→4 by days 0/5/11. The corresponding end-game counts remain 8/4/3.
Wheat/carrot/tomato/strawberry/melon output 396.75/129.75/51.38/191.75/66.75;
egg/milk/wool/fert 65/170.38/57.13/273.38. Wheat/fert buys 162.63/78.25;
net wheat/milk/wool/fert 165.63/169.13/56.63/144.75. Mean wheat/fert buy hour
12.20/6.70 and sale hour 10.52/5.37; milk/wool sales 7.12/5.28. Discards 9.25,
weed digs 53.75, zero invalid unit actions. Legal actions and ample hiring do
not imply faithful production. Data: results/search_v0_4_profile.*. Exact
package/direct/generic/debug action parity, PASS and self-play checks pass.
PASS mean $114,393; use its recorded pass_J for the risk-aware comparison.

Eight profiled cash-collapse games of sixday-rl versus teammate_shoprouter
have zero cash, 57 crop-days, only four cow-days and 12 sheep-days. Reserve
recovers $57,470, 1,271.75 crop-days, 75 cow-days, 119 sheep-days and 274 hires
but produces 131.75 discards and 973 invalid unit commands. Its 73.4% crop
watering and 80%/86.6% cow/sheep feed+care are more functional, while overflow,
missing geese and 11.3% productive fertilization expose incomplete downstream
repair. Full milestones, output, gross/net trades and hours remain in
sixday_collapse_profile.* and sixday_reserve_profile.*. This is a capital
repair with large route/storage consequences, not just a cash statistic.

## Next priorities

Use the working day-contract interface for composition/service and intraday
sale-deadline changes, then validate in complete live-opponent games. Preserve
borrowed complete days as feasible starting points and fallbacks. Improve the
fast labor/withdrawal/deposit model using these requested-versus-realized
contracts. Extend the small automatic outer search to multiple opponents and
better execution choices before simply increasing its 583-proposal budget.
Continue Kaito v58 and other promising source ports; fresh Mao/3정훈 courses
remain useful counters to our current branch. No GPU workload is needed.
