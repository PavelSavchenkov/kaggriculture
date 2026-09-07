# Review 22 — 2026-09-07 08:07

Original objective reread at 08:07. Next review: 08:27. The goal remains active
until 00:47 tomorrow. No blockers, Git, subagents, GPU workload or submission.

## Retained improvements

The Junghoon sale-only component wins 968/1,024 fresh games against its source,
with mean margin $695 and lower-tail margin -$651. Native RNG gives 240/256 wins
and $715 mean margin. Broad discovery: 252/256 against v4, all Mao85/89, but
62 public, 101 teammate, 93 Binghua and 35 John131. Retain another specialist.

Its 32 paired profiles have identical lifetimes, service masks, production,
sale volumes, purchases, spending, labor, land, discards and unit faults. Mean
wool sale hour moves from 8.983 to 7.778. Own revenue/cash increases $312.719;
opponent cash falls $654.375, giving $967.094 margin. This is timing and shared
price response, not more production. The optional deposit code makes no physical
change in these cases. Exact package: candidates/junghoon_wool_sales.

A Binghua single cow-to-goose change wins 732/1,024 fresh games against Binghua116,
mean margin $715 and lower-tail -$4,137. Native RNG: 191/256 and $850 margin.
Broad discovery wins 183/256 Junghoon, 192 Binghua, 144 v4, 122 public, 200 teammate,
174 Mao85, 230 Mao89 and 132 John131. It is useful across several opponents but
does not replace v4. A neighboring purchase change, ticket10, is weaker overall
and remains an experimental alternative. All three packages pass generic, typed
debug, thread-count, PASS and self-play checks; validation JSON records the scope.

Against v4, ticket9's 32 paired profiles replace 23 cow-days with 23 goose-days;
sheep-days rise from 176.75 to 183 because some additional sheep purchases succeed.
Cow count falls from nine to eight by day8; one goose remains from day8 onward;
sheep reaches eight by day11 rather than 7.6875 on average. Outputs change from
257.938 milk/0 eggs/197.125 wool/371 fertilizer to 232/31/204/376. Strawberry
increases from 247.125 to 249; wheat538, carrot65 and melon72 remain unchanged.
Own cash rises $1,444.563, revenue $1,488.25 and spending $43.688. Saving $100 on
the changed animal is offset by other realized costs, including extra sheep;
this is a dependency effect, not an isolated one-animal return calculation.
Opponent cash rises $632.938, so margin gains $811.625.

Both versions hire 280 workers for $5,002, buy two land upgrades for $3,000,
and occupy 1,512 crop-days. Water rate stays .733; productive fertilization
.193→.196. New cow feed/care/collection .870/.966/.962, sheep .951/.956/.842,
goose .826/.957/.957. Discards .125→0, faults 97.313→77, weed digs remain19.
Wheat buy105, sale274.563→269, net169.563→164; fertilizer buy37, sale339.938→344,
net302.938→307. Other sales equal corresponding harvested output. Sale hours:
wheat11.70, milk12.42, wool13.74, egg14.48, strawberry18.53, fertilizer4.23.
This preserves real output, input, timing and labor attribution.

## Fast estimation and the improvement loop

The first individual-animal score used fixed observed quotes and often ranked
changes backwards. New market_tape.hpp records jointly accepted amounts at their
original order indices. Its conditional replay preserves per-unit quotes, town
and shop demand, fixed spending and the $1 floor. Both players' baseline cash
matches exactly; baseline per-animal biology also matches exactly.

One baseline trace costs 73–96 ms in the initial runs. Estimating a substitution
then costs about 27–30 microseconds. On ten selected alternatives within the
same baseline scenario, margin rank correlations are .988 for source55/public,
.997 for Binghua/v4 and .939 for Junghoon/self. These are development checks,
not independent evidence of global estimator accuracy. Rankings across shop
scenarios are less stable.

The search now traces eight seeds in both seats once, then estimates each
candidate across matching baselines before choosing exact compilation work.
All 48 baseline cases across three runs pass animal-biology and both-cash checks.
Sixteen-baseline tracing takes 1.12–1.28 s; full estimate batches take 3.72 ms for
12 alternatives, 5.48 ms for16, and 13.13 ms for24. The three new runs each test
ten changed policies and ten unchanged-animal output controls in 32 full games.

For Binghua, margin rank correlation across all 32 exact games improves from
-.543 with the fixed-quote score to .794 with the multiple-baseline market score;
on the later seeds excluded from baseline tracing it is .830. Source55 gives
.576 overall/.430 later; Junghoon .842/.733. Different runs select different
alternatives, so these are scoped diagnostic comparisons. The new ranking places
the useful Binghua cow-to-goose changes near the top.

Some source55 animal tickets match only eight of sixteen baselines. A purchase,
pickup and placement do not retain the same inventory ownership under every
funding history. Report this coverage instead of averaging silently over missing
cases. Next add input-stock feasibility and stronger deposit timing, then allow
verified observation-based activation. Current deposit delay is a one-turn
relaxation, existing sale quantities may be exceeded, rival quantities stay fixed,
and funding can become negative. These estimates remain conditional heuristics.

## Fresh global checks and new public material

The official 08:02:02 snapshot ranks ymg first, Mengfei second, Junghoon third,
JustinLee tenth and Atakan eleventh. Six recent games each from Mengfei, Justin
and Atakan were chosen by global standing, not local matchup results. All18 cash
reconstructions pass: 12,328 transactions, 4,010 crop and305 animal lives;
1,039 composition cohorts and540 complete days. These courses are not yet added
to the 144-course executable library. Three newly listed/updated notebooks were
pulled without executing their cells.

The new cohort averages 194.72 cow-, 175.89 sheep- and22.39 goose-days. Cow counts
at days0/2/5/8/11/17/23/29: 2/2.94/3.94/7.5/8.17/7.94/7.06/6.39; sheep
2/2/2/4.89/7.67/7.67/7.39/6.28; geese0/0/0/.11/1.11/1.11/1.11/1.11. Service
feed/care/fertilizer collection: cow .792/.824/.961, sheep .899/.864/.918,
goose .896/.925/.883. This reinforces that full service is a useful default,
not a universal rule, especially near terminal herd contraction.

Crop occupancy1,506.28, water .695, productive fertilization .454. Harvests:
wheat480.11, carrot137.17, tomato39.11, strawberry232.56, melon76.67. Wheat
buy175/sale313.56/net138.56; fertilizer39.33/276.89/237.56. Other net sales:
carrot134.72, tomato38.78, strawberry231.06, melon76.67, egg31.28, milk203.94,
wool188.61. Mean sale hour7.251 and input-buy hour9.275. Hires270.06/$4,646.83,
land2/$3,000, weeds23.94, faults25.83/6,193.94 requests, discards16.17. Large
family differences and some stressed games remain; these are approximate targets.

The main v4 is unchanged: 183 cow/121 sheep/108 goose-days, crop1,523, water.711,
productive fertilizer.217, wheat457/milk211/wool138/egg143/fertilizer387 output.
Wheat net85.31 and fertilizer net312; hires267.75/$4,700.63, land2, weeds21,
faults18.38, discards7.44. PASS J remains145,940 versus145,573 for v3. Stronger
sheep and crop-output compositions remain targets; universal differences are
not instructions to force the new specialists toward one herd mix.

King-v4e RC4 describes a compatible YARN_SECOND continuation, a spare sheep slot,
a fertilizer throttle and a public-observation liquidity branch. Its claimed
400-game local result is unverified here. Source extraction, licensing and C++
parity audit are next. The independent EXP006 baseline reports losing to Shape;
it is not yet a promising strong-agent port. The crop/labor notebook points out
that unfertilized melon reaches its yield cap at harvest age10, while our cold
count-span default can wait until age12. Check earliest-cap rotations and local
service pruning against the official engine; do not replace every crop rule.

## Priority and original-goal check

Keep composition search central: estimate multiple shop/opponent cases, compile
promising dated changes, diagnose realized gaps, retain useful versions and
challenge the larger league. Extend ticket edits to v4 itself, add financing/
stock guards, and explore observed-shop continuations. Preserve maintained crop
counts, cold starts, family insertion/deletion, placement alternatives and full
V30 rebuilds; the single-animal neighborhood is only one compiler option.
The best general agent still has counterstrategies. Next fresh pool800000+;
final900000+ remains unused and unanalyzed.
