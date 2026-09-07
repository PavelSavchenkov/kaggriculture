# Review 24 — 2026-09-07 08:47

Original objective reread at08:45. Next review09:07. The24-hour goal remains
active until00:47 tomorrow. No blockers, Git, subagents, GPU work or submission.

## Stronger sources and complete checks

King RC4 now passes14,380 original-action comparisons, including LOW/MID/HIGH
responses, all four Thomas continuations, S1/YARN reroutes, recovery and the RC3/
RC4 changes. Generic/typed debug/thread results agree in16 full games; PASS/self
pass. Fresh1,024games per opponent:830 Junghoon wins,962 public,990 teammate,
423 v4,117 Binghua. Native RNG confirms the Junghoon strength and v4 weakness.
PASS256 J136,012. Retain the complete source port and its active-component map;
do not treat the author's local win claim as a leaderboard rating.

Eighteen latest global replay courses were appended as144–161. All162 courses
and116,478 original actions match. Justin150/152/153 give identical tested
behavior;150 is retained. Justin154 andAtakan161 add distinct alternatives.
All named wrappers pass full source, generic/debug/thread, PASS and self checks.

Justin150 is the most promising new broad reference. Fresh850000..850511 both:
805/1,024 wins versusv4,736 Junghoon,666 Binghua,676 Binghua-ticket9,550 King,
610 John131,585 public and802 teammate. Mean margin versus King is-$1,050 and
lower-tail-$23,935, despite a majority of wins. Against v4 mean margin$3,050,
lower-tail-$4,129. Native256 gives220 v4 wins and174 Binghua. PASS256 J146,163,
slightly above v4's145,940. These tests support a new search starting point;
older pulled-agent and previous-version comparisons are still running.

08:48 follow-up: the26 older-opponent screen also finishes with a majority
against every opponent. It includes the three six-day variants, Kaito v58,
Finance7, public capacity router, seven archived families and v0–v3. Lowest
win rate is170/256 against the capacity router. Justin150 is now the primary
broad search starting point while v4 and all specialists remain in the league.

Justin154 is weaker across the larger league than its small first screen.
Atakan161 wins234/256 versusKing but loses more often againstv4/Binghua/public;
retain it as another counter. No inferred original branching policy is claimed
for any one replay course. Exact source episode/seat/submission/hash accompanies
every package. The main v4 archive remains unchanged.

## Composition-first improvement

The new reproducible runner builds a content-addressed C++ binary, records its
hash/command, runs estimate→compile→exact comparisons, attaches code and replay
lineage, and formats calibration. Python only orchestrates and analyzes; all
strategy and gameplay work is C++. See research/ticket_search_runner.md.

Two rounds start fromJustin150 againstKing and itself. Each traces16 baseline
games, checks both cash balances and animal biology exactly, estimates14 dated
single-animal alternatives, and tests10 changes plus10 output-only controls in
32 games each. Every alternative matches all16 baseline histories in these
rounds. Trace time1.23–1.24s; estimate batches5.04–6.82ms. Margin rank correlation
is .830 overall/.564 on later seeds againstKing, .709/.673 againstself. Fixed
quote scores are much weaker. These remain selected development comparisons.

No clear broad improvement is established from these first edits. Several
changes beat self but lose advantage againstKing; some apparent gain comes
from the output-only timing control. Preserve the choices and rejected evidence.
Next investigate terminal unsold output, stronger stock/finance safeguards,
observed-shop choices and generic incumbent support. Do not collapse the search
to one species substitution neighborhood.

The crop/labor notebook supplied a useful independent lifecycle idea. The new
earliest-equal-output option changes melon harvest age12→10 and fits two mature
rotations into22 days. Two maintained tiles now realize all44 requested crop-days
and24 melons instead of26 actual crop-days/12 melons plus an immature request.
All32 games match those totals; cash6,054→9,008. In a mixed farm, early harvest
raises mean cash45,078→49,006 and J43,982→47,887. Ten-melon realization can miss
one output unit, explicitly recorded as119 versus120 predicted.

Separate local fertilizer/water deletion preserves dated biological output and
occupancy in40 exact engine micro-cases. It saves labor in simple farms and can
remove all unnecessary melon fertilizer. In the fertilized ten-melon control,
44 purchases/14 resales/30 applications coincide with only38 of60 predicted
melon output; removing that dependency restores60. Mixed-farm service pruning
regresses despite fewer hires; early harvest plus pruning also trails early
harvest alone. Keep both options explicit. Original CountSpan defaults unchanged.
Full profiles/contracts for16variants×32games and lineage are retained.

## Deep global comparison

Independent global reference remains the08:02 official snapshot cohort: six
games each from globally strong Mengfei, JustinLee andAtakan, selected before
local evaluation. All18 cash reconstructions match. Animal-days194.72 cow,
175.89 sheep,22.39 goose. Cow counts atdays0/2/5/8/11/17/23/29 are2/2.94/3.94/
7.5/8.17/7.94/7.06/6.39; sheep2/2/2/4.89/7.67/7.67/7.39/6.28; geese near0
throughday8 and1.11 fromday11. Feed/care/fertilizer collection per animal-day:
cow .792/.824/.961, sheep .899/.864/.918, goose .896/.925/.883. Full service
remains a useful starting policy with locally optimized exceptions.

Global crop occupancy1,506.28, water .695, productive fertilization .454.
Harvests wheat480.11, carrot137.17, tomato39.11, strawberry232.56, melon76.67.
Gross wheat buys/sales175/313.56/net138.56; fertilizer39.33/276.89/net237.56.
Other net sales carrot134.72, tomato38.78, strawberry231.06, melon76.67,
egg31.28, milk203.94, wool188.61. Sale hour7.251, input-buy hour9.275.
Hires270.06/$4,646.83, land2/$3,000, weeds23.94, faults25.83/6,193.94 requests,
discards16.17. These are approximate targets with substantial family variance.

Justin150's32 exact games versusv4 have209 cow/145 sheep/59 goose-days. Cows
2/3/4/8/8/8/8/8 at the same milestones; sheep2/2/2/4/6/6/6/6; geese0 through
day8 and3 thereafter. Feed/care/collection: cow .885/.962/.962, sheep
.938/.972/.897, goose .898/.966/.814. Crop occupancy1,508, water .725,
productive fertilization .197. Outputs wheat516, carrot82, tomato0,
strawberry249, melon72, egg71, milk245, wool161, fertilizer379.

Justin wheat buys153/sales295/net142; fertilizer50/354/net304. Milk sells239,
wool154, eggs70; some produced output remains unsold. Other harvested crops
are fully sold. Buy/sale hours wheat7.582/11.003, fertilizer2.18/5.198;
carrot sales7.744, strawberry16.165, melon9.25, egg2.971, milk13.368,wool13.630.
Hires279/$5,088, land2/$3,000, weeds20, faults26, discards5, shed maximum100
with eight turns at90+. Terminal recovery is a concrete component opportunity.

For comparison v4 remains183 cow/121 sheep/108 goose-days, crop1,523, water.711,
productive fertilization.217. Cow feed/care/collection .858/.913/.956, sheep
.926/.901/.926, goose .898/.870/.926. Wheat457/milk211/wool138/egg143/fertilizer
387 output, net wheat85.31/fertilizer312. Hires267.75/$4,700.63, land2/$3,000,
weeds21,faults18.38,discards7.44. Justin's improvement includes a different herd,
real output and trade timing; do not attribute it solely to more transactions.

## Priorities and remaining original ideas

Use the new strong complete course as a warm start, improve it against the
growing league and preserve earlier versions. Continue learning reusable days,
placements and continuations from fresh globally selected sources. Keep the
new crop lifecycle rule in cold-start and larger farm search. The generic route/
workforce compiler, stock-safe composition edits, branching suffix search and
multi-generation league loop remain unfinished; the goal is not complete.
Fresh pools800000 and850000 are consumed. Next fresh1,000,000+; final900000+
remains unused and unanalyzed. Next source refresh around09:04.
