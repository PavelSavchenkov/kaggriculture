# Review18 — 2026-09-07 06:47 checkpoint, recorded06:49

Original objective reread06:47. Goal remains active; next07:07. No GPU workload
is needed. The strongest retained scheduled policy improves, but generic
composition execution remains the central bottleneck. Do not equate a failed
greedy realization with an economically bad composition.

## Retention and causal evidence

guarded_hires_v3 improves all three new globally selected counters on fresh
650000–650511 both:401/1024Binghua116,503John128,570John131, versus v3's
393/497/565. Mean margins -2352/+366/+286 versus -2511/+214/+133. It still
loses most Binghua games and almost all Junghoon78 games. Earlier fresh gate
has860/1024versusv3 and broad older-league strength. Named opening_router_v4
wrapper created; underlying source checks complete, wrapper parity pending.
Next fresh promotion700000+; final900000+ untouched.

Paired32healthy profiles have identical output, sale volumes, daily life/service
masks and land. Average5.25fewerhires saves175.375; cashgain173.4375 decomposes
exactly into hire savings, revenue+.375 and other spend+2.3125. Two games buy
and discard one extra wheat. Opponentcash changes+.1875mean. This is primarily
labor efficiency, not more production or inflated trade volume. PASS256games
J145940 versus145573parent, using the prescribed80%mean/20%lower-decile cash.
scripts/check_guarded_hires.py preserves causal checks and freshcountergate.

## Independent global invariant check

Use the30games independently selected by official06:04global ranks1,2,9,11,12,
not the local counter ranking. Compare current32healthyguarded profiles with
that cohort; different opponents/scenarios mean these are approximate targets.
Cow-days183 vs181.7; sheep121 vs177.37; goose108 vs14.43. Our milestones cows
2→3day2→4day5→7day8/end, sheep2→4day8→5day11→3end, geese0→2day8→5day11/end.
Global cows2.2→3→3.8→6.53→7.17day11→6.53end; sheep2→4.67day8→7.57day11→6.27end;
geese.27day8/.4day11/.73end. Binghua/John specifically reach9–10sheep byday11.
The sheep/goose allocation is a meaningful outer-search alternative.

Feed/care/collection local versus global: cows.858/.913/.956 vs.835/.865/.925;
sheep.926/.901/.926 vs.880/.877/.901; geese.898/.870/.926 vs.892/.843/.824.
Service defaults are useful, but late care-bank additions can have zero value;
the previous marginal model found16such opportunities acrossfourfamilies.
Crop occupancy1523 vs1549, water.711 vs.720, productive fertilization.217 vs.241.
Local output wheat457,milk211,wool138,strawberry262,egg143,fertilizer387.
Global wheat520,milknet202,woolnet188,strawberrynet253,eggnet22.3: separate
produced output from net sales when assessing the gap.

Local gross wheatbuy353.25/sale438.56/net85.31 versusglobal236.23/427.5/191.27;
fertbuy50.94/sale362.94/net312 versus49.7/306.8/257.1. Our livestock-heavy feed
cost and lower wheat production are not explained by sale transaction counts.
Local milk/wool/egg nets211/138/143; strawberrynet261.25. Wheatbuy/salehour7.32/
10.05,fert1.98/6.17,milk4.995,wool7.964,straw11.909,egg7.559. Global family
timing differs substantially; review17 preserves per-family and all-item flows.
Local hires267.75/$4700.63 versus277.3/$5330.9;land2each;discard7.44vs7.8;
weed21vs17.67;ineffectiveactions18.38vs38.87. The absent generic 'faults' key
in the global JSON is not a zero fault count. Junghoon-stressed profiles remain
much worse, as review16 documents; healthy matches do not establish robustness.

## Compiler and league investigation

composition_greedy_v1 modes0/1/2/3 isolate baseline, lower fertilizer priority,
care-after-feed and both. Mode0 reproduces v0 in sixgames. Thirty-two profiled
games each across3compositions show no win improvement. Lower fertilizer
priority slightly increases animal service but loses crop output; care-after-
feed usually loses useful care opportunities. Preserve negative evidence.
Generic source4 makes~4339moves versus~3006for stronger guarded schedules;
animal feed+care remains~.51cow/.47sheep despite$8269hires. New routing ablation
counts immediate movement reversals and compares soft/hard continuation of
valid worker targets. It is running; no conclusion yet.

New replay wrappers and compiler mode1 pass generic4thread versus debugtyped
1thread exact16gamefield comparisons, PASS and self-play. Broad256games/course:
Binghua116 beatsguard56.64%,John12844.92%,John13157.42%. John131 beatsJunghoon
85.94%,Mao8583.98%,Mao8999.61%,Thomas49.22%. Strong countercycle; do not infer
the complete original policy or universal superiority. All132originalcourses
and94908actions have source parity. New FarmManager source remains inspected,
licensed and available, not yet adopted into a C++ policy.

## Original-goal faithfulness and priorities

Keep dated composition search, warm/cold proposals, family insertion/deletion,
large rebuilds, input/capital/layout/workforce/timing dependencies, many sampled
shops, legal observable branches and estimate-versus-realized feedback. Current
day-template improvement is a useful compiler component, not the completed
outer search. Generic warmdata still72 versus132replaycourses; expand isolated
version after control ablations. Add explicit maintained-count spans that
expand repeated crop rotations; one plant lifetime does not express 'two wheat
fromday1through20'. Next effort goes to movement/realization, that interface,
then marginal sheep/goose replacements with full dependency rebuilding and
league calibration. Remaining public-source audits and cold FarmManager port
are retained; no source-policy execution in Python or external submission.
