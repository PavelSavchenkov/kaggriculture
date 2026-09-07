# Review 23 — 2026-09-07 08:27

Original attachment reread at 08:20. Review written at 08:30. Next review 08:47.
Goal active until 00:47 tomorrow; no blockers, Git, subagents, GPU or submission.

## New public agent

King RC4 is now a C++ package with 52 program blocks, 3,738 block actions and
22,289 nearest-market examples. The first differential check matches all14,380
source actions across eight recorded sequences and12 forced sequences. Forced
cases trigger the spare sheep purchase, fertilizer reduction, late hiring
reorder, weed recovery, second-YARN continuation and high-capital continuation.
Additional MID/HIGH opening-response coverage is running, followed by generic
and typed debug builds. Full-game strength is not established yet.

The final Python entry point bypasses several legacy controllers. The old
super-classifier, capital-adaptive context and anti-router mirror are inactive;
porting them as active rules would change behavior. The live tail is exactly
the Thomas source already ported here, verified by its full source hash. Its
per-instance C++ state is reused; a read-only accessor supplies the immutable
next-turn schedule for King's presale rule. This does not alter Thomas actions.
Full source hash, active/dead layer map and attribution limits are in IMPORT.json.
The notebook's rating and reuse license remain unknown. Its local win claim is
not accepted as our evidence. No notebook policy runs in local gameplay.

Reusable ideas include buying one extra sheep only at a compatible pickup/place
slot, reserving only next day's bootstrap hires, and choosing between aggressive
and throttled sale schedules from public production differences. Test these as
separate components after the faithful whole-agent league screen. Keep the
original controller so changes have a causal baseline and recoverable lineage.

## Global invariant check

No new replay batch since the 08:02 snapshot. The independent global cohort in
review22 still contains six games each from then-top players Mengfei, JustinLee
and Atakan. All18 cash reconstructions pass. Neither this selection nor its
metrics depend on our local matchup wins. The executable library remains144
courses;18 additional complete courses and540 day templates await integration.

Global mean animal-days are194.72 cow/175.89 sheep/22.39 goose. At days0/2/5/8/
11/17/23/29, cows are2/2.94/3.94/7.5/8.17/7.94/7.06/6.39 and sheep2/2/2/
4.89/7.67/7.67/7.39/6.28. Geese are absent through day5 and reach1.11 by day11.
Feed/care/fertilizer collection per animal-day: cows .792/.824/.961, sheep
.899/.864/.918, geese .896/.925/.883. Continued herd growth followed by some
terminal contraction supports testing dated service exceptions, not mandatory
full daily service or mandatory escapes.

Crop occupancy1,506.28, watering .695, productive fertilization .454. Harvests:
wheat480.11, carrot137.17, tomato39.11, strawberry232.56, melon76.67. Gross
wheat purchases/sales175/313.56, net138.56; fertilizer39.33/276.89, net237.56.
Other net sales: carrot134.72, tomato38.78, strawberry231.06, melon76.67,
egg31.28, milk203.94, wool188.61. Mean sale hour7.251; input purchase hour9.275.
Hires270.06 costing4,646.83, two land upgrades costing3,000, weed digs23.94,
faults25.83 among6,193.94 requests, discards16.17. Family variance remains large.

The retained v4 is unchanged:183 cow/121 sheep/108 goose-days; cows reach7 by
day8, sheep/geese5 by day11. Feed/care/collection cow .858/.913/.956, sheep
.926/.901/.926, goose .898/.870/.926. Crop occupancy1,523, watering .711,
productive fertilization .217. Wheat457, milk211, wool138, strawberry262,
egg143 and fertilizer387 output. Wheat buys353.25/sales438.562/net85.312;
fertilizer50.938/362.938/net312. Wheat buy/sale hour7.318/10.053, fertilizer
1.982/6.168, milk4.995, wool7.964, strawberry11.909, egg7.559. Hires267.75/
4,700.625, land2/3,000, weeds21, faults18.375, discards7.438. PASS J remains
145,940 versus145,573 for v3. No new performance measurement is implied here.

The newer Junghoon wool specialist improves timing with identical output,
volumes, costs and labor in32 paired profiles; it is not a production gain.
Binghua ticket9 changes actual biology:23 fewer cow-days,23 goose-days and
6.25 more sheep-days, with additional sheep funded successfully. Own cash
+1,444.56 and rival cash+632.94 separate private gain from competitive gain.
Neither specialist has been promoted over v4. Detailed paired flows in review22
are unchanged; README now records Junghoon's actual fresh and deployment checks.

## Original ideas and priorities

The user's composition-first intuition remains central. Single-animal search
already estimates alternatives across multiple shop cases before exact worker
execution and uses observed estimate/realized differences to improve pricing.
Its current limits are fixed rival quantities, relaxed deposit delay, missing
stock feasibility in some matching histories and no generic v4 adapter. These
remain higher priority than adding increasingly narrow fingerprints to one agent.

After King parity and broad screening, extract useful components and return to
stock-safe dated composition changes, v4 support and observed-shop continuations.
Also test earliest-cap melon rotations: the cold compiler may wait past the
first capped harvest. Preserve cold starts, insertion/deletion, complete farm
rebuilds, tile placement and V30 scheduling alternatives. A successful notebook
port expands the league and compiler library; it does not finish the improvement
loop. Next fresh pool800000+; final900000+ unused and unanalyzed.
