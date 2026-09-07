# Review 8 — 2026-09-07 03:27 UTC

Original objective reread at 03:26. Goal active; next review 03:47 UTC. A useful
branch fixed the known counterstrategy cycle, and genuinely newer player
courses then exposed its limits. Keep both positive and negative evidence.

## Branch implementation and exact checks

public_router and feeltheagi_55 share the full first action. Their unit actions
agree through turn 144, but market orders differ at turn 1. Probing 23 existing
opponents in both seats verified the common first action. At turn 1, the three
old counters already have hired hands while public_router, the teammate and
feeltheagi still have only the original worker.

opening_router_v0 uses that one public feature: select feeltheagi when the
opponent has not hired yet, otherwise public_router. It commits before the
first different market order, and copies complete downstream policies. It
uses no identity, seed, hidden inventory or future shop information. Six
opponent comparisons (1,536 full games) match the selected parent exactly.
Generic four-thread and debug pair single-thread results match over 16 full
games. PASS and self-play checks pass; self-play utility 0.5.

Fresh seeds 300000–300511, both seats: 1,024/1,024 wins against each of
public_router, skomuro, Deniz and Arman. Mean margins respectively
$12,748/$19,239/$21,095/$24,137; lower-decile margins
$5,875/$7,951/$11,250/$9,612. Discovery versus teammate is 256/256. It ties
feeltheagi in mean margin and utility 0.5; it does not beat that parent.

## Fresh globally selected evidence

Refreshed official leaderboard at 03:14:45. Top names changed: HowardLeeTW #1
(2923.4), 3정훈 #2 (2852.9), and Mao Nishino #12 (2757.7) were new to the
previous top-12 cohort. feel the agi is now #11 (2760.3); earlier source ranks
remain tied to their original snapshot. Downloaded six recent games from each
new team, reconstructed all cash exactly, and extracted 18 compositions with
4,600 lifetimes and 540 complete days. Data stays in research/refresh_0314/;
the earlier cohort and program indices are preserved.

Appended exact courses as programs 72–89. All 64,710 actions across 90 courses
pass source parity. One source attaches arbitrary numeric arguments to HIRE;
the engine ignores them, so the typed action normalizes those fields to zero.
No source behavior was inferred from the ignored values.

Independent new-player tests: 64 games/course, seeds 4000–4031 both seats,
against both public_router and opening_router_v0. Howard courses win 0–40.6%
against either. Most 3정훈 courses also lose, but program 78 wins 64/64 versus
opening_router with +$51,200 mean margin, while winning only 28.1% versus
public_router. Mao courses win 56.3–100% versus opening_router; five of the six
win 93.8–100%, with +$19,389 to +$25,980 mean margins. They also beat
public_router in 65.6–96.9% of games. These players were selected by the fresh
global leaderboard, not by convenient local failure filtering.

Thus the opening rule fits the earlier response families but is insufficient
for the newly observed population. Retain it as a version and keep its parents
and new counters; do not proclaim broad dominance from the old league gate.

## Deep invariant checks

The new independent 18-game cohort averages $104,002 cash, 1,547 crop-days,
68.5% watering, 33.1% productive fertilization, 287 hires/$5,962, two extra land
quadrants and 11.67 discards. Wheat/carrot/tomato/strawberry/melon harvest:
557.3/82.4/23.2/252.6/78.0. Gross wheat/fertilizer buys 219.8/42.1. Net
wheat/carrot/tomato/strawberry/melon/egg/milk/wool/fertilizer:
210.3/80.2/22.7/249.7/77.9/53.9/217/160.1/251.3. These supplement the
original 72-game cohort rather than silently replacing its definitions.

Howard's mean farm is distinctly different: 235.7 cow-days, 121.8 sheep-days,
23.8 goose-days; productive fertilization 57%; 664 wheat harvested and only
86.2 bought. Milk net 242, wool 120.5, fertilizer 174.7, labor 316.8/$8,306.
3정훈 uses 149.5/217/30.7 animal-days, 20.9% productive fertilization and
262.2 hires/$3,891; net milk/wool/fertilizer 163.7/222.2/285.7. Mao has
223.8/126.8/56 animal-days, no tomato harvest, wheat buys 347.8 and net
milk/wool/fertilizer 245.3/137.5/293.7. These are useful economic family
alternatives, not just new schedules to memorize.

New local profile: opening_router against skomuro, 32 games. Its public_router
branch has 1,509.3 crop-days, 72.7% watering and 25.3% productive fertilization;
the feeltheagi branch profile from review 7 remains relevant for delayed hiring.
Cow/sheep/goose days: 212.7/136.8/45, with feed+care
83.6%/87.6%/83.3% and fertilizer collection 94.8%/90.8%/83.9%. This differs
from the feeltheagi branch's 183/121/108 animal-days, a real composition choice.
Both start with 2 cows, 2 sheep, 12 melons and 7 wheat; later animal allocation
and trades create the different response. Exact per-day milestone tables remain
in the profile CSVs and replay invariant tables.

Local branch wheat/carrot/tomato/strawberry/melon output:
536.1/76.6/0/260.3/72; egg/milk/wool/fertilizer: 59.3/236.1/146.1/363.1.
Gross wheat/fertilizer buys 143.6/66; net wheat 188, milk 235, wool 146.1,
fertilizer 266.8. Mean wheat/fertilizer buy hour 8.16/1.56; wheat/strawberry/
milk/wool/fertilizer sale hour 11.32/12.50/4.03/6.59/6.96. Labor 283/$5,612,
two extra quadrants. Discards 14.31, weed digs 24, invalid non-PASS commands
92.22. The branch preserves source behavior but does not remove route faults
or overflow. Global comparable weed/fault accounting remains incomplete.

## Reprioritization

Keep the user's outer-composition idea central. The integrated model and exact
course library now support measured proposal ranking; the missing automatic
mutation/rebuild loop and cold-family compiler must be implemented. Labor and
intraday capital/input/deposit scheduling remain major model/compiler gaps.
Do not spend all effort extending a hand-written opening classifier. Later
observable checkpoints with state-compatible continuations, or robust choices
across indistinguishable opponents, are more promising. Kaito v58's published
collision/minimax methodology is relevant and its source lineage is available;
its headline known-stream result is not an unseen strength guarantee.

Next: expose arbitrary typed composition input to the compiler, run an actual
estimate→compile→exact improvement iteration, and preserve strong borrowed
complete routes as an execution method and league baseline. Continue the
remaining teammate/public notebook audit. No GPU workload is demonstrated.
