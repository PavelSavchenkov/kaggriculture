# Rotation log: gaps 1-4 (started Sep 25 15:30, 5 hours)

Rule (user): work on gaps 1-4 of `GAPS.md` concurrently; pick the most underexplored gap, work on it
for 15-20 minutes, document, switch. Main validation runs belong to the main session; here: ideas,
smoke tests (`smoke.sh`: 64 games, Local-LB cha22 + yannik-latest and zoo_dsm + v13_w384b, seeds
700-707, both seats, vs X10 on the same games via `smoke_compare.py`), and more tests only when CPU
allows. Detailed notes per gap: `BUDGET_FIX.md` (gap 1) and sections below.

## 15:30 status

| Gap | Explored so far | Next |
|---|---|---|
| 1 budget | main session (trim model, knapsack, land first, reserve farm, recovery); me (X1, X6, X10, feasible style) | smoke: deferral, day-0 Majkel style (running) |
| 2 contested sales | me (race probe X2/X12, slot priority X13); main session (race defaults) | visible-supply opponent forecast |
| 3 night horizon | me (X5, X8, X11: all negative) | dawn sell-down, hold value |
| 4 routes | diagnostics only | **first stint**: route search settings as a diagnostic |

## Stint 1 (15:30-15:50): gap 4, routes

Findings (reading `day_policy_local/source/routes.cpp`, earlier trip audit):
- The solver bundles jobs into small nodes (a fertilizer collection with the crop that uses it,
  Hungarian-paired under variant 512; same-tile jobs), assigns nodes to workers (beam/regret),
  and improves by local moves (reorder, span moves, tail exchange). Feeding and product collection
  join a fertilizer node only under the optional ANIMAL_SERVICE_SEED variant. There is no trip
  shape like top players' (pick up wheat at the shed, serve animals, take the collected fertilizer
  to crops, carry products home). Consistent with our trip audit: combined animal+crop trips 50%
  vs 64% for top teams, 11.0 vs 12.0 actions per worker-day.
- The workforce search is cheap by default (`minimize_variants = 1`, "8 for a more expensive
  search"; opportunistic hire reduction off; 4 route variants, "16 also tries finishing same-tile
  work before returning").
- Diagnostic smoke `routesearch` (16 variants, 8 workforce variants, opportunistic reduction):
  running. Question: are the 1.4 extra hires/day for same-day returns a search failure or inherent?

Proposal (design, not implemented): a trip-based route builder. Generate candidate trips from
templates (wheat pickup → animal chain with feed/care/collect/fertilizer collection → crop chain
using that fertilizer → deposit), with cargo and deadline checks; choose a covering set of trips
for K workers (greedy or small set-partitioning); sequence trips per worker within 24 hours;
objective = Fibonacci wages + value of deposit timing (from the market planner, gap 2/3). The
current node/assignment search can seed it. Placement needs no change (layout compactness equals
top teams').

Side finding (gap 1): the day-0 Majkel style probe collapses in some games (seed 701 vs cha22:
-191k, 19 fallback days): the main session's collapse mechanism (day 7-8 cash crunch, survival
level stops harvests). Ported their recovery level into `exp_patch` as `OPUS_RECOVERY` so later
budget probes are not contaminated.

## Gap 1 smoke results (deferral, day-0 style, feasible style): see BUDGET_FIX.md

X10 remains best; whole-plan replacements (quantile, style, day-0 style) are negative; deferral
neutral. Recovery level ported (`OPUS_RECOVERY`).

## Stint 2 (from 15:35): gap 3, night horizon and wages

Probe `deadline22`: the market part of the returns is due by hour 22 instead of 20 (top players
deposit and sell at hours 22-23; our deadline forces +1.4 hires/day while we sell at hours 20-23
anyway). Running.

Results (64-game smokes vs X10 on the same games):

| Probe | Local-LB | Zoo | Mechanism |
|---|---|---|---|
| `routesearch` (gap 4): 16 route variants, 8 workforce variants, opportunistic hire reduction | -2.3k (13/19) | +2.0k (21/11) | wages -$583/game, 9 fewer hires, 100 fewer idle turns, own money flat; opponent +2.3k (sale timing shifts); compile 3.6x slower (worst day 12.4 s) |
| `deadline22` (gap 3): market returns due by hour 22 | +2.4k (18/14) | -1.8k (18/14) | wages +$333 (more units qualify for same-day sale), night cargo -71; inconclusive |

Take-aways: about a third of our wage gap is search quality (the rest is the return rule); route
changes shift sale timing and races, so gaps 2-4 must be evaluated together. Moving the return
deadline alone does not cut hires. Next gap-3 probe: `hold100` (hold value 1.0) running.

### Why did route search raise the opponent's cash? (user question, 15:45)

- Mechanism: shared market. Route search delivered our strawberries later (hours 12-17: 69.8 vs
  75.1 units per game; hours 18-23: 167 vs 149); the opponent sold the same volume at a higher
  price ($130.3 vs $116.8). Opponent money diff correlates 0.88 with its strawberry revenue diff.
- Statistics: opponent richer in 19/32 games; diff std $14.5k (-20k..+41k), median +1.2k: the mean
  +2.3k is not robust at 32 games (a few strawberry-race games dominate).
- Conceptual point (stands): routing and selling optimise different objectives. The route solver
  minimises hires under fixed return deadlines; the sale DP maximises margin taking arrival times as
  given. Nothing values when our goods reach the market relative to the opponent, so a cheaper
  route can hand races to the opponent. Fix: one objective, expected margin: each deposit gets a
  value by arrival hour from the market planner (including the opponent's forecast supply), and
  the route solver minimises Fibonacci wages minus deposit value. This ties gaps 2, 3 and 4.

| `hold100` (gap 3): hold value 1.0 instead of 0.95 (sale DP and return planner) | -0.8k (14/18) | -1.6k (13/19; wins 26 -> 20) | holding for dawn loses evening races (as X5) |

Gap 3 status: all four probes that make us sell later or hold more (X5, X8, X11, hold100) lose;
deadline22 is inconclusive. Against these opponents, same-day evening selling is doing real work
(race), and the top teams' dawn inventory cycle is not automatically better for us. The part of
gap 3 that remains is the wage cost of the returns, which is route construction (gap 4) and the
missing joint objective (routes x sale timing), not the holding decision itself.

## Stint 3 (15:43-): gap 2, opponent supply forecast from the visible farm

Probe `forecast` = X13 + OPUS_FORECAST_VISIBLE=0.6: expected opponent sales today >= 0.6 x its
visible ripe/held output, spread over its recent hourly pattern (hours 6-12 without history); used
by the sale DP and the return planner. Compared with X13 on the same games. Running.

Result `forecast` (vs X13): Local-LB -65 (14/16), zoo -2.3k (13/19; wins 32 -> 30): neutral to
negative. With the margin objective, a larger expected opponent supply makes the DP sell earlier to
deny it; the zoo opponents do not sell that way. A forecast change must first be validated as a
forecast (per-product prediction error on real games, by opponent family) before the DP uses it.

Next gap-2 probe `raceall` (vs X13): per-product delivery timing, same-day deposits of product p
due one hour before the opponent usually starts selling p (trailing forecast, never before hour 10).

### Forecast validation (15:55; `tools/forecast_audit.cpp`, `q42.py`; 752 games)

WAPE of the opponent's daily sales per product (days 4-27), trailing 3-day mean vs estimators
from the opponent's visible ripe/held supply at dawn:

| Opponent | Product | trailing | max(tr, 0.6 vis) | 1.0 vis | 0.5 tr + 0.5 vis |
|---|---|---:|---:|---:|---:|
| Local-LB agent | melon | 1.67 | 1.17 | 0.17 | 0.81 |
| Local-LB agent | wool | 0.80 | 0.68 | 0.42 | 0.57 |
| Local-LB agent | milk | 0.50 | 0.41 | 0.36 | 0.39 |
| top team | melon | 1.43 | 1.16 | 0.46 | 0.85 |
| top team | strawberry | 0.57 | 0.53 | 0.61 | 0.49 |
| top team | milk | 0.69 | 0.63 | 0.72 | 0.60 |
| top team | wool | 0.87 | 0.98 | 1.14 | 0.88 |
| zoo BC | melon | 1.52 | 1.28 | 0.31 | 0.90 |
| zoo BC | milk | 0.82 | 0.75 | 0.71 | 0.74 |

Validated estimator: melons = visible ripe melons; other products = 0.5 trailing + 0.5 visible
(best or near-best in most cells). The probe's max(tr, 0.6 vis) was among the weaker ones, which
fits its neutral-to-negative smoke. Next: `OPUS_FORECAST_BLEND` (vs X13). A small learned
regression per product and opponent behaviour would be the proper version (learned helper).

Result `raceall` (vs X13): Local-LB +3.1k (20/12), zoo -1.9k (13/19). Opponent-dependent: racing
early deliveries pays against early sellers (Local-LB) and costs against late sellers (zoo BC).
The deliveries must follow a forecast of *when* this opponent sells, from its observed hourly
pattern, not a fixed rule. Running: `blend` (validated daily forecast, vs X13).

Next planned (gaps 3+4 joint objective): X13 + validated forecast + wage-aware returns (X5 lost
because the value of beating the opponent's sales was underestimated; with the better forecast
the return decision may price it correctly).

Result `blend` (validated daily forecast; vs X13): Local-LB -1.2k (16/16), zoo -2.0k (14/18). The
forecast is more accurate yet play is worse: the sale DP (margin objective, opponent's forecast
treated as fixed, our same-hour units assumed to fill first) turns any extra forecast supply into
earlier dumping to deny it. So the DP's model of the opponent is the problem, not only the forecast:
it should treat the opponent as responsive (its sales depend on price) and model the slot lockstep.

User asked (15:58): make race timing adaptive. Implemented `OPUS_RACE_DP`: per product, the sale
DP values delivery by hour 10/12/.../20 against this opponent's observed hourly pattern and picks
the latest hour within 1% of the best (generalising day-29 `receipt_deadline`); tried first, then
the default deadline, then capacity-only returns. Running `racedp` (vs X13).

Result `racedp` (vs X13): Local-LB -315, zoo +1,951.

**Noise level of the 64-game smokes (16 seed clusters per half):** seed-clustered SE $0.6-2.1k, so a
95% interval is about +-1.3k to +-4.2k. Only effects of ~4k or more are readable (X10, X13, styles,
collapses). Intervals: racedp LB [-1.6k, +1.0k], zoo [-0.2k, +4.1k]; raceall LB [-0.3k, +6.5k],
zoo [-4.5k, +0.8k]; blend LB [-4.8k, +2.5k], zoo [-6.3k, +2.2k]; defer LB [-3.1k, +4.8k].
The gap-2/3 timing and forecast probes are within noise; smokes screen, they do not decide.

Larger test started (CPU room at 16:02): `x13racedp` on the full zoo panel (7 opponents x 32 games),
paired with X13. Running in parallel: `rw05` smoke (sale-DP rival weight 0.5 instead of 1; the
margin objective was tuned against frozen replays, where denial always works).

Result `rw05` (sale-DP rival weight 0.5; vs X13): Local-LB +87 (16/16), zoo -91 (11/19): neutral.
The denial weight is not the lever; the opponent model (fixed volumes, our same-hour units first)
is the more likely issue behind `blend`.

## Stint 4 (16:03-): gap 4 again, trip shape

Probe `bundle` (vs X13): animal feeding/care bundled into the fertilizer-collection nodes on every
route variant (`ANIMAL_SERVICE_SEED`, default only on variants 1, 3, 5), moving trip shape toward
top teams' combined animal+crop trips. Running. Full-zoo `x13racedp` also running.

### Proposal: adaptive race timing (user request 15:58)

1. Opponent profile, learned online per product from observed market flows (History already
   infers its sales per hour): its sale-hour histogram and its sell-through relative to its
   visible supply (validated estimators above as the starting point), smoothed day by day. This is
   what makes timing adapt to early sellers (Local-LB) and late sellers (zoo BC) in the same game.
2. Delivery hours chosen by the sale DP against that profile (`OPUS_RACE_DP` is this layer with the
   trailing profile), passed to the route solver as graded deadlines, with the default deadline as
   fallback.
3. Offline-learned prior for days without history (first melon wave, first wool/milk waves): a
   small per-product regression on replay features (visible supply, trailing sales, day, shops),
   trained on the `forecast_audit` output of the 1.45M replay days.
4. The DP's own opponent model must change with it (see `blend`): opponent sales depend on price
   (it holds when prices are low), and same-hour competition follows the slot lockstep.

Result `bundle` (vs X13): Local-LB -501 (13/19), zoo +152 (17/15): neutral; trips unchanged.
Important: with X13 our combined animal+crop trip share is already 65% (top teams 64%); the
baseline's 50% came from its smaller herd, so the herd fix (X10) closed that metric. Gap 4 now =
the wage gap ($7.0k vs $5.4k): return hires (+1.4/day) and search quality (~$0.6k), not trip shape.

## Stint 5 (16:10-): gap 1, re-test with the recovery level

`d0maj_rec` = X13 + OPUS_RECOVERY + day-0 Majkel style (the earlier day-0 style result was dominated
by one collapse that recovery prevents). Running.

Result `d0maj_rec` (X13 + recovery + day-0 Majkel style; vs X13): Local-LB -2.0k (18/14), zoo
-4.1k (11/21): no collapse, still negative. Closed: X10's trim order already makes day 0 feasible
with 3 sheep.

Result full-zoo `x13racedp` (224 games vs X13): +446 [-396, +1,353], 223/224 wins: neutral.

## Method change (16:15): mirror test bed

Smokes against Local-LB/zoo have 95% intervals of +-1.3k to +-4.2k. The X10 mirror (X10 vs X10) had
14 ties in 32 games and mean |margin| $681, so a mirror match "X13 + probe" vs "X13" measures a
probe's head-to-head effect against a strong equal opponent with little noise (closest setting to
top-vs-top). `mirror.sh` (opponent gets X13 flags, ignores the probe flags via OPUS_OPPONENT_DROP).
Sanity run `mirror/base` (no probe) running.

Result `noopen` (X13 without the Vadim opening pin; vs X13): Local-LB -235 (16/16), zoo +1.4k
(14/18): neutral. With budget re-plan and slot priority, the opening pin shows no measurable value
in smokes; to be confirmed in the mirror bed (opponent keeps the pin via BC_OPPONENT_OPENING=6:8).

### Learned forecast helper (16:22; `q44.py`)

Per-product non-negative linear model of the opponent's daily sales, sold ~ a*trailing + b*visible,
fitted on half of the top-team games, evaluated on the other half and on Local-LB/zoo opponents.
Coefficients (trailing, visible): carrot (0.59, 0.48), tomato (0.48, 0.45), strawberry (0.39, 0.48),
melon (0.10, 0.97), egg (0.40, 0.44), milk (0.32, 0.46), wool (0.42, 0.34). WAPE held-out top teams,
trailing -> linear: melon 1.42 -> 0.49, milk 0.69 -> 0.55, wool 0.86 -> 0.76, tomato 0.68 -> 0.52,
strawberry 0.57 -> 0.48; transfers to Local-LB (melon 0.24) and zoo (melon 0.37). Forecast accuracy
is solvable; the open question is the DP's use of it (see `blend`), tested in the mirror bed.

## Consolidated idea table (16:25)

Smoke = 64 games (Local-LB cha22 + yannik-latest, zoo_dsm + v13_w384b, seeds 700-707), 95% CI
about +-1.3k to +-4.2k per half; full = 160 LB or 224 zoo games; mirror = X13+probe vs X13.

| Gap | Idea | Result | Verdict |
|---|---|---|---|
| 1 | X10 network trim order (loss only) + no day-0 reserve | LB +5.3k, zoo +6.2k (full) | adopt (main session did) |
| 1 | deferral of trimmed animals | LB +0.9k, zoo 0 (smoke) | neutral |
| 1 | coherent smaller plan (crop-total quantiles) | never fired | dropped |
| 1 | feasible style re-plan (any budget day) | LB -9.2k, zoo -3.1k | reject |
| 1 | day-0 Majkel style (+ recovery) | LB -2.0k, zoo -4.1k | reject |
| 1 | no opening pin (with X13) | LB -0.2k, zoo +1.4k | neutral; mirror queued |
| 2 | X13 slot priority by revenue at stake | zoo +8.9k (full), LB +0.1k | adopt |
| 2 | melon race probe (X2/X12) | LB +2.4k (full) | superseded by adaptive timing |
| 2 | visible forecast max(tr, 0.6 vis) | LB -0.1k, zoo -2.3k | reject (weak estimator) |
| 2 | validated blend forecast | LB -1.2k, zoo -2.0k | DP misuses it; mirror queued |
| 2 | fixed race deadlines (raceall) | LB +3.1k, zoo -1.9k | opponent-dependent |
| 2 | DP-chosen delivery hours (racedp) | zoo +0.4k (full), LB -0.3k | neutral; mirror running |
| 2 | rival weight 0.5 | 0 / 0 | neutral; mirror queued (0.5, 0) |
| 2 | responsive rival in the DP | - | mirror queued |
| 3 | wage-aware returns (X5) | LB -1.8k (full) | reject (loses races) |
| 3 | sell-fraction cap / fixed holding (X8, X11) | -23.6k / -13.2k | reject (shed overflow) |
| 3 | market deadline 22 | LB +2.4k, zoo -1.8k | inconclusive; mirror queued |
| 3 | hold value 1.0 | LB -0.8k, zoo -1.6k | neutral-negative; mirror queued |
| 3 | own supply in the hold value | - | mirror queued |
| 4 | wide route search | wages -$583, LB -2.3k, zoo +2.0k, compile x3.6 | mixed; opp-hire alone queued |
| 4 | animal service bundling | 0 | neutral; trips already top-like with X13 |

### Candidate lever checked: adapt the product mix to the opponent's (16:20, `q45.py`)

Top-vs-top games, controlling for shops (correct shop tool): per extra opponent producer our price
moves wool -0.4, milk +1.0, egg -0.6, strawberry +0.1 and our revenue ~0 (wool +0, milk +166, egg
-60, strawberry +138); per own producer revenue +891..+1,782. Product overlap with the opponent
costs little, so specialising away from the opponent's mix is not a lever. (Players' mixes are
strongly correlated beyond shops: 0.50-0.89.)

### Deposit timing (16:23, `q46.py`, trip audit, days 11-27)

Share of deposited units by hour: ours (baseline, Local-LB) 0-5: 8%, 6-9: 13%, 10-13: 13%, 14-17:
20%, 18-19: 24%, 20-21: 16%, 22-23: 7%; top teams 21%, 8%, 11%, 12%, 10%, 19%, 19%. Our returns
come mid-afternoon (the hour-20 deadline makes workers go back and out again); top teams deposit at
the day's edges (dawn and hours 20-23). Supports a later deadline or deposit-at-the-end trip shape
(deadline22 mirror queued).

## Mirror results (16:35; X13 + probe vs X13, 32 games, seed-clustered 95% CI)

| Probe | Gap | W-T-L | Margin |
|---|---|---|---|
| base (no probe) | - | 9-14-9 | 0 |
| blend (validated forecast: melons = visible, others 0.5 trailing + 0.5 visible) | 2 | 24-0-8 | **+2,434 [+866, +4,090]** |
| blend + responsive rival | 2 | 25-0-7 | +2,169 [-10, +4,306] |
| racedp (DP-chosen delivery hours) | 2 | 19-0-13 | +547 [-1,245, +2,458] |
| raceall (fixed early deliveries) | 2 | 19-0-13 | -855 [-3,083, +1,003] |
| responsive rival alone | 2 | 14-0-18 | +344 [-108, +852] |
| rival weight 0.5 / 0 | 2 | 13-2-17 / 13-0-19 | -244 / -334 |
| hold value 1.0 | 3 | 11-0-21 | **-3,810 [-8,526, -606]** |
| market deadline 22 | 3 | 17-0-15 | +99 [-1,020, +1,124] |
| own supply in the hold value | 3 | 11-4-17 | -67 [-363, +214] |
| animal service bundling | 4 | 20-0-12 | **+803 [+246, +1,401]** |
| opportunistic hire reduction | 4 | 9-14-9 | 0 (never changes a route: no effect) |
| no opening pin | 1 | 14-0-18 | -200 [-1,473, +962] |

Readings:
- The validated forecast wins head to head (+2.4k) although it lost in the smokes against Local-LB
  and zoo opponents (-1.2k, -2.0k, both within noise). Against an opponent that sells like us
  (same DP, evening sales), predicting its sales from its visible farm is right; against opponents
  whose sale timing differs, the DP's reaction (sell earlier to deny) costs. Traced mirror runs
  (`mtr/base`, `mtr/blend`) running for the mechanism.
- Holding more is confirmed negative (-3.8k). The denial weight does not matter (0, 0.5, 1 equal).
- Animal-service bundling is a small real gain (+0.8k) in the mirror; neutral in smokes.

## Gap 3: are held units worth holding? (16:35, `q47.py`, 160 Local-LB games of X13, 900 top-team games)

Per product and night: held units H = shed at dawn; realized = average price of the first H units
sold the next day; evening = average price of the last selling hour the day before.

| | wool | milk | strawberry | melon | egg |
|---|---:|---:|---:|---:|---:|
| ours: realized / evening | 0.89 | 0.94 | 0.94 | 0.87 | 1.00 |
| top teams: realized / evening | 0.97 | 0.96 | 0.96 | 0.88 | 0.99 |
| ours: held units per night | 14.6 | 10.4 | 14.4 | 7.4 | 9.5 |
| top: held units per night | 9.4 | 7.2 | 15.0 | 9.4 | 9.3 |

Held units sell 3-13% below the previous evening's price for everyone, top teams included. Top
teams' dawn inventory is not a price gain; it comes from their labour pattern (deposit at the day's
end). Our held wool and melons do worse than theirs (0.89, 0.87): holding them is overvalued by the
fixed 0.95 discount. This closes "hold more" as a lever. The remaining gap-3 value is wages
(return_wages in the main session) and possibly a lower hold value for wool.

## Kaggle replays of the submission (16:47): see KAGGLE_REPLAYS.md

18 games, 18 wins against low-rated opponents. Costs: day-0 melons trimmed by the trim0 reserve
(3 of 6; melon revenue -$7.5k per game against 12-melon opponents), wages +$2.0k per game against
top teams (13 hires on 28% of mid-game days), 23 missed fertilizer collections. so2 +
return_wages fixes the first two.

### Mechanism of the blend mirror gain (16:50, traced mirror `mtr/`, `q50.py`)

Ours minus opponent per game (blend vs X13): milk +$2,370 (+16 units), wages -$570 (we pay less),
melon +$539, eggs -$720, wheat -$496; the rest within +-$300. With the visible-supply forecast our
milk moves from the evening (hours 22-23) to hours 6-17 (14.2 vs 7.0 units at hours 6-11), ahead of
the opponent's evening sales; fewer same-day returns are worth their wages, so hires drop. The
gain is the "sell before the opponent" race on milk, found by the DP once it sees the opponent's
supply. Against smoke opponents that already sell early or hold, the same shift costs, so the next
step is the adaptive part: learn each opponent's sale hours online (layer 1 of the adaptive-timing
proposal) before using the visible supply.

## Mirror batch 4 (17:00): wage-aware returns and combinations (vs X13)

| Probe | W-T-L | Margin |
|---|---|---|
| retwage (market returns must pay their extra wages; same rule as the main session's return_wages) | 20-0-12 | +538 [-337, +1,510] |
| retwage + racedp | 25-0-7 | +1,411 [-84, +2,748] |
| retwage + blend | 27-0-5 | +2,420 [+560, +4,180] |
| blend + bundle | 21-0-11 | +1,774 [+299, +3,368] |

retwage matches the main session's mirror (+646). On top of blend, neither retwage nor bundling
adds anything: blend already moves sales earlier and cuts return hires (-$570 wages).

## Labour: what is the core issue? (user question 16:52; `tools/ledger.cpp` LEDGER_HIRES, `q51.py`)

Days 10-28, per day; ours = Kaggle submission (18 games), Local-LB X13 (160), C++ mirror (32);
top = 900 top-team games (top-10 perspectives).

| | ours | top |
|---|---:|---:|
| hires / wage | 11.6-11.7 / $344-366 | 11.0 / $262 |
| productive actions | 145-148 | 155 |
| worker-turns | 286-288 | 274 |
| actions per worker-turn | 0.51 | 0.57 |
| moves per action | 0.84 | 0.71 |
| idle turns | 17 | 9 |
| wage lost to uneven hires (actual - even spread) | $28 | $14 |
| daily sd of hires / of work | 0.99 / 15.4 | 0.75 / 15.5 |

- Not the workload: we do 5% less work than top teams.
- Not hire timing: all hires, ours and theirs, are made at hours 0-5; the 12th and 13th hires
  work full days (22 turns, 11.7 actions) like the others.
- Not mainly unevenness: our daily workload varies exactly as much as top teams' (sd 15.4 vs
  15.5); uneven hiring costs $28/day vs their $14.
- The core: efficiency per worker. Top teams get 0.57 actions per worker-turn, we get 0.51 (more
  walking: 0.84 vs 0.71 moves per action; twice the idle turns). At top efficiency our work would
  need about 0.9 fewer workers per day, which is the whole $80-100/day wage gap (the 12th hire
  costs $144, the 13th $233).
- Next: where the walking and idling happen (per-hour ledger running) and how much same-day
  returns cause (traced mirror of retwage running).

### Labour, continued (17:12; per-hour ledger `q53.py`, action mix `q52.py`, traced mirrors)

Action mix per day, days 10-28 (ours = Local-LB X13; Kaggle and mirror are the same within 1):

| | moves | idle | water | shed visits | other work | busy turns (work + moves) | turns bought |
|---|---:|---:|---:|---:|---:|---:|---:|
| ours | 123 | 17 | 43 | 14 | 90 | 269 | 286 |
| top | 110 | 9 | 49 | 11 | 95 | 265 | 274 |

By hour: our idle turns are all at hours 20-23 (0.8, 2.2, 4.4, 9.1 per hour; top 0.4, 0.8, 1.4,
3.0); at hour 23 top workers still walk and work (3.5 moves, 5.6 actions), ours stand (0 moves,
3.5 actions). Our walking is higher all day (5.5-6.2 moves per hour at hours 5-19 vs 4.3-5.2), and
our shed visits peak at hours 18-20 (0.5-0.8 per hour vs 0.1-0.2: the hour-20 market returns).

Answer: the core of the labour gap is walking, about 13 extra moves per day, which is 0.55 workers
per day, and at the 12th-13th hire's Fibonacci price ($144, $233) that is the $80-100/day wage gap.
Busy turns are otherwise equal (269 vs 265); top teams turn the same time into more actions.
- About 40% of the extra walking comes from same-day market returns: wage-aware returns (traced
  mirror) cut daytime moves 112.8 -> 107.2 per day and hires 11.59 -> 11.33 ($344 -> $305/day).
- The rest is route construction: moves per non-watering action 1.18 vs 1.04 (earlier trip audit:
  2.1 vs 1.76 moves per unit of the day's spanning tree). The route value does price walking
  (route time, linear plus quadratic, which also balances workers), so the loss is in how jobs
  are chosen and grouped (which tiles, node bundling, heuristic assignment), not in a missing
  term.
- End-of-day idle (16 vs 6 turns at hours 20-23) is not a cost in itself: with an integer
  workforce some slack remains; top teams fill theirs with extra watering and care (water per
  plant 0.84 vs 0.79), and the auto-deposit at day end brings their carried goods home without a
  walk. For us it is free capacity: the ~1 missed fertilizer collection per day could use it.
- Not the cause: hire timing (all hires at hours 0-5 for everyone; 12th and 13th hires work full
  days), workload (we do 5% less), unevenness across days (our daily work varies like top teams';
  uneven hiring costs $28/day vs $14).

Misalignment behind it: the compiler's objective is "fewest hires that finish the network's jobs
by the deadlines". The deadline itself is not priced (the hour-20 market returns force deposits
and trips back to the shed; return_wages now prices it), tiles for count-type jobs are chosen
before routing, and slack has no use. Fixes, in order of evidence:
1. Wage-aware returns (main session's return_wages): measured, -$38/day wages, mirror +0.5k.
2. Route search that also minimises total moves within the chosen workforce, so hire reduction
   finds room (`routesearch` mirror running; earlier smoke: -$583 wages per game, 3.6x compile
   time; Kaggle overage use is at most 2.8 s of 60 s, so there is time budget).
3. Slack filling: after the workforce is fixed, add optional valuable jobs to idle turns
   (missed fertilizer collections, care) at zero wage.

## Labour fixes in the mirror bed (17:16; X13 + probe vs X13)

| Probe | W-T-L | Margin | Note |
|---|---|---|---|
| routesearch (16 route variants, 8 workforce variants, opportunistic hire reduction) | 21-0-11 | **+2,044 [+399, +3,647]** | worst-day compile 2.2 s -> 3.2 s (median per game, machine loaded) |
| retwage (return_wages) | 20-0-12 | +538 [-337, +1,510] | from batch 4 |
| routesearch + retwage | 27-0-5 | +1,654 [+282, +2,928] | |
| collectall (collect every available fertilizer, `OPUS_COLLECT_ALL`) | 25-0-7 | **+1,667 [+965, +2,461]** | slack filling; full zoo panel running |
| joint (`OPUS_JOINT`: solve every market option, keep max sale gain - extra Fibonacci wages) | | running | gaps 2+3+4 in one objective |

Search quality is a real head-to-head lever (+2.0k), in line with the labour decomposition: about
60% of our extra walking is route construction. Its smoke (Local-LB -2.3k, zoo +2.0k) was within
noise. Time budget: Kaggle games use at most 2.8 s of the 60 s overage, and compile time grows
mostly on the heaviest dawns; the total per game must be measured before adoption (cheaper
settings to test: 16 route variants only, or 8 workforce variants only).

collectall (17:17): the compiler attached a fertilizer collection only to animals that already had
a feed, care or product job that day, so the fertilizer of animals without a job was lost when
the next one appeared (Kaggle: 22.8 missed units per game vs 11.0 for top teams). Collecting every
available fertilizer uses idle capacity and wins the mirror by +1.7k with the tightest interval so
far. One condition in `compile` (`OPUS_COLLECT_ALL`); a candidate for the main session.

Result `joint` (17:18): 21-0-11, +494 [-948, +1,883]: neutral. Taking the best option by estimated
sale gain minus wages is no better than trying the DP delivery hours first with the wage check
(retwage + racedp +1.4k); the sale gains come from the same trailing forecast, so the choice is
only as good as the opponent model. Not pursued further until the forecast is adaptive.

## Stint (17:19-): gap 2, does the visible-supply forecast generalise?

blend: mirror +2.4k [+0.9k, +4.1k]; smokes (64 games) Local-LB -1.2k [-4.8k, +2.5k], zoo -2.0k
[-6.3k, +2.2k]. Larger panels running: Local-LB 160 games (`exp/x13_blend` vs `exp/x13_slot`),
then the full zoo (224 games) after `x13collect`.

Result `x13_blend` Local-LB panel (160 games vs `x13_slot`, 17:21): +344 [-2,300, +2,718], wins
100% -> 99% (2 losses of 32 vs yannik; per opponent -1.2k to +1.5k, all within noise). Neutral on
Local-LB, +2.4k head to head in the mirror. Full zoo panel next (after `x13collect`).

### Compile time of the wider route search (17:20; `timing.sh`, build_patch4 writes compile_ms_total)

8 mirror games each, 2 threads, machine load 35-40 (absolute times inflated):

| Setting | total compile per game (median / max) | worst day (median / max) |
|---|---:|---:|
| base (4 variants, 1 workforce variant) | 7.2 s / 9.8 s | 1.5 s / 2.9 s |
| routesearch (16 variants, 8 workforce variants, opportunistic reduction) | 24.8 s / 31.7 s | 4.1 s / 8.9 s |
| 16 route variants only | 18.8 s / 27.2 s | 4.0 s / 7.5 s |

Kaggle gives 1 s per turn plus 60 s of overage per game; the submission uses at most 2.8 s of
overage. The full wider search would use roughly 15-25 s of overage per game at these speeds,
with single dawns up to ~9 s: feasible but with little margin on a slow machine. Adoption path:
search wider only when it can pay (days whose plan needs 12+ hires, where one hire less saves
$144-233) and only while the remaining overage is large (the compiler already has time-pressure
levels). Next test if CPU allows: that conditional version in the mirror.

### Own money vs opponent money in the mirror (17:22; paired with `mirror/base` on the same seed and seat)

| Probe | own - base | opponent - base | margin |
|---|---:|---:|---:|
| collectall | +1,548 | -119 | +1,667 |
| routesearch | +3,982 | +1,938 | +2,044 |
| routesearch + retwage | +3,211 | +1,557 | +1,654 |
| blend | -3,458 | -5,892 | +2,434 |
| retwage | -1,666 | -2,205 | +538 |
| retwage + blend | -2,921 | -5,341 | +2,420 |
| bundle | -1,831 | -2,634 | +803 |
| hold100 | -4,461 | -651 | -3,810 |
| combo4 (blend + collectall + routesearch + retwage) | +3,051 | +635 | **+2,416 [+1,150, +3,697], 28-4** |

Two kinds of gains:
- Productive (we earn more, the opponent is unchanged or gains): collectall (+1.5k own) and the
  wider route search (+4.0k own, the largest own-money gain measured today). Search also moves our
  deliveries and so helps the opponent (+1.9k), as in the earlier smoke.
- Competitive (both earn less, the opponent more so): blend, retwage, bundle. They win races and
  deny the opponent; the whole market pie shrinks. In a two-player game the margin is what counts,
  but these gains depend on the opponent's selling behaviour (blend: neutral on Local-LB).
The combination is not additive in margin (+2.4k) but keeps most of the productive gain (+3.1k
own) while blend removes most of the opponent's gain from search (+1.9k -> +0.6k). Next: blend +
collectall + the conditional wide search (`OPUS_WIDE_IF=12`).

Caveat (main session's measurement, memory `mirror-eval-variance`): 20-seed mirror sets swing a
lot (one opening won 25-15, 31-9, 26-14 and lost 17-23 on another set). All mirror results here
use one set (seeds 700-715, both seats). They screen; adoption needs >= 4 seed sets including the
exact Local-LB seeds. My CPU use is kept to about 12 game threads (shared machine).

### collectall on the full zoo panel (17:24; 224 games, paired with X13)

```
cand_v12_vadim6  n= 32 x13collect wins 32/32 (x13 32)  diff    -116 [-2491, +2151]
v11_style        n= 32 x13collect wins 32/32 (x13 32)  diff   +1261 [-1510, +4589]
v12_cond         n= 32 x13collect wins 32/32 (x13 32)  diff   +2617 [+115, +4653]
v13_w384b        n= 32 x13collect wins 32/32 (x13 32)  diff   +1384 [-1230, +4622]
zoo_dsm          n= 32 x13collect wins 32/32 (x13 32)  diff    +685 [-703, +2090]
zoo_majkel       n= 32 x13collect wins 32/32 (x13 32)  diff    +673 [-1143, +2722]
zoo_vadim        n= 32 x13collect wins 30/32 (x13 30)  diff    +990 [-529, +2593]
pooled           n=224 x13collect wins 222/224 (x13 222)  diff   +1071 [+239, +1954]
```

Pooled +1,071 [+239, +1,954]: positive on the zoo as in the mirror (+1,667). Diagnosis confirmed on
160 Local-LB games of X13: missed fertilizer is 0.02 per day on days when every animal is fed and
1.73 per day when some are not (53% of days have unfed animals: 0.5 on days 0-9, 1.7 on days
10-19, 4.1 on days 20-25, 8.5 on days 26-29); an animal without a feed job gets no collection.
Candidate for the main session: one condition in `compile` (collect wherever fertilizer is
available, outside survival). Needs the Local-LB panel and >= 4 mirror seed sets.

Result `wideif12` (17:26; wide search only on days whose plan needs 12+ hires): 18-6-8, +306
[-118, +804], own +425; compile 8.4 s per game (base 7.2 s). Cheap, but most of the full wide
search's +4.0k own gain does not come from saving a 12th/13th hire. Traced full `routesearch`
queued to find where it comes from (wages, deliveries, returns).

Result `x13blend` full zoo (17:34; 224 games vs X13): -245 [-1,597, +1,132]. blend = mirror +2.4k,
Local-LB +0.3k, zoo -0.2k: it only wins against our own agent (a competitive effect against an
opponent that sells like us). Not a general lever; an adaptive forecast would be needed.

## Gap 3 re-opened (17:29): selling into price crashes (first Kaggle loss, see KAGGLE_REPLAYS.md)

The first Kaggle loss (vs sekai013, rank 27, -4.4k) turned on the strawberry endgame: after both
sides dumped strawberries (price $12-52), the opponent held 16-32 in the shed for days and sold 54
at $123-190; we sold ours into the crash. Across games (`q54.py`: units sold on a day when that
product's price reaches >= 1.5x within 5 days; upper-bound revenue given up per game): top teams
$3.8k (milk 27, strawberries 21, wool 26 units), ours Local-LB $6.1k (39, 38, 30), Kaggle $5.7k,
self-play mirror $10.3k (42, 49, 51).

This refines the earlier gap-3 verdict. One-night holding is not a price gain (held units sell
3-13% below the evening price for everyone), and raising the hold value everywhere loses. The
missing decision is multi-day: hold through a crash when the forward price, from known shop
consumption, recovers over several days. The sale DP cannot see it (held stock is valued as one
sale around noon tomorrow). Probe `OPUS_HOLD_DAYS=K`: the hold value is the best of noon tomorrow
and noon of day +2..+K (0.95 per day; forward inventory = shop and town consumption, minus the
opponent's expected sales, plus `OPUS_HOLD_OWN` our own daily output), capped at the last day; the
shed-capacity charge still allocates room. Mirrors `hold5`, `hold5own` running.

### Where the wider route search earns (+4.0k own; traced mirror, ledger `ledger/mtr_rs`)

Per game vs base: wages -$732 (10 fewer hires per game), milk +$2,225 (+11 units collected),
tomato +$1,079, wool +$965 (-5.7 units, better prices), carrot +$509, wheat -$1,100 (fewer sold).
So half of the gain is labour and the rest is more animal output delivered (more of the planned
milk collected), not only fewer hires. The conditional version (12+ hires) captured little
(+425 own), so the benefit is spread over ordinary days. Cost: 3.4x compile time per game.

Results (17:38): mirror `hold5` 11-0-21, -291 [-1,020, +425]; `hold5own` 15-0-17, -8 [-542, +575]:
neutral, and behaviour barely changed (traced: crash-selling bound $10.3k -> $8.5k per game, shed
at dawn 41.9 -> 43.4 units). Market facts from the lost Kaggle game (`tools/market_audit.cpp`):
shops ate 25-31 strawberries per day; the price curve is very steep near base inventory (10,045
units -> $34, 10,004 -> $112, 9,919 -> $196; about $2-3 per unit sold), so without sales the price
recovers by $45-60 per day. Why the probe saw little: (1) the opponent's forward sales come from
its trailing 3-day mean, which during a crash is its dumping rate (sekai had sold 18/day, then
sold 0-7); (2) held units were valued as one bulk sale on one day, whose own price impact
(~$2-3 per unit) eats the recovery. Version 2: `OPUS_HOLD_SPREAD` (each held unit goes to the day
with the best discounted price, days 1..K) and `OPUS_HOLD_RESP` (the opponent's forward sales scale
linearly from 0 at half the base price to all at the base price). Mirror `holdsr` and frozen
replays (145 top-10 games + our 31 Kaggle games, `frozen.sh`) running.

Results (17:44), multi-day holding:

| Probe | Mirror | Own / opponent (mirror) | Frozen top-10 (145) | Frozen Kaggle (31) |
|---|---|---|---|---|
| hold5 | -291 [-1,020, +425] | -1,769 / -1,478 | -600 [-4,763, +3,184] | -435 [-1,782, +833] |
| hold5own | -8 [-542, +575] | -573 / -565 | -653 [-4,413, +2,890] | -513 [-1,258, +133] |
| holdsr (5 days + spread + responsive opponent) | -844 [-1,928, +92] | -4,834 / -3,989 | | |

Holding through crashes, as modelled, does not pay. The full version holds much more and loses
$4.8k of own money in the mirror: the X13 opponent keeps selling during a crash, so our held units
meet continued dumping. Against sekai (which held), dumping was the mistake. The decision is a war
of attrition that depends on the opponent's crash behaviour, so it needs an online estimate of
whether this opponent holds when the price is low (learned opponent response, gap 2/5), not a
fixed rule. Closed as a fixed-rule probe; kept as a documented opportunity (top teams sell into
crashes half as much as we do).

Panels started (17:44): Local-LB 160 games for `collectall` (`exp/x13_collect`, build_patch4) and
the full zoo for the wider route search (`zoo/x13rs`).

### Crash behaviour by player type (17:45, `q55.py`, days 10-28)

Share of available stock (shed at dawn + harvested) sold on days when the product's mean price is
below half its base price; normal days (price >= 0.8 base) in brackets:

| | strawberry | wool | milk | melon |
|---|---:|---:|---:|---:|
| top-10 teams | 0.39 (0.49) | 0.43 (0.58) | 0.60 (0.62) | 0.63 (0.86) |
| rank 11+ opponents of top teams | 0.40 (0.47) | 0.50 (0.50) | 0.60 (0.59) | 0.45 (0.72) |
| Local-LB family | 0.58 (0.67) | 0.59 (0.63) | 0.57 (0.70) | 0.57 (0.83) |
| ours (X13, Local-LB) | 0.56 (0.61) | 0.54 (0.62) | 0.52 (0.64) | 0.65 (0.95) |

Strong players hold back strawberries and wool in crashes (-10 and -15 points vs normal days);
we and the Local-LB family keep selling. So the top-level opponents we will meet more often on
Kaggle are exactly the ones against which dumping in a crash loses (the sekai game). Our test
beds (X13 mirror, Local-LB family, zoo BC models trained mostly on our own style) contain few
holders, which may be why the holding probes could not show a gain there. A test bed with
crash-holding opponents is needed before any hold rule can be judged: the frozen top-10 replays
are the closest (hold5 -600 [-4.8k, +3.2k], too noisy at 145 games).

Gap 1 check (17:46): so2's herd (400 exact Local-LB games, `reports/lbseeds3/so2`) is 12.5 / 17.8
/ 18.3 / 18.7 animals at days 9 / 12 / 15 / 20 vs top teams' 13.0 / 18.0 / 18.8 / 18.8, with 10
melons by day 3. The submitted robust agent had 11.9 / 17.4 / 17.8 / 18.0 and 8 melons on Kaggle.
Gap 1 is closed in so2; it only needs to be submitted.

Caveat on the wider search's milk gain (17:47): the games with the largest milk differences
diverge in herd composition (seed 707 seat 0: day 9 routesearch buys 3 cows -> 6 cows + 10 sheep +
3 geese; base keeps buying sheep -> 3 cows + 17 sheep), with routesearch paying more wages on days
7 and 9. So part of its +4.0k own gain is game divergence (different network decisions after
different schedules), not better collection. Wages -$732 per game is the systematic part. The
zoo panel (224 games) decides.

### Is an opponent's sale timing learnable online? (17:47; day-level hour buckets, days 6-14 vs 15-28)

Per player, the share of units sold at hours 0-5 and 18-23 early (days 6-14) and late (15-28),
and the correlation across players between the early and late share:

| | dawn share early -> late (r) | evening share early -> late (r) |
|---|---|---|
| top-10 teams (1,506) | 0.47 -> 0.38 (0.31) | 0.17 -> 0.38 (0.36) |
| rank 11+ (294) | 0.45 -> 0.46 (0.39) | 0.22 -> 0.36 (0.47) |
| Local-LB family (210) | 0.33 -> 0.42 (0.87) | 0.25 -> 0.36 (0.93) |
| ours (210) | 0.02 -> 0.16 (0.23) | 0.74 -> 0.69 (0.39) |

Rule-based agents keep their timing all game (r ~0.9), so an online profile works against them.
Top teams change timing with the phase (more evening selling late) and differ little from each
other in a stable way (r ~0.3), so an online profile from early days predicts them poorly. An
adaptive opponent model therefore needs a phase-conditioned prior learned from top-team replays
(the learned helper of `VALUE_MODEL.md` / q44), updated online, not a pure online estimate.
Note also our own timing: 2% of units at dawn early in the game, 74% in the evening.

## collectall confirmed on three beds (17:56)

`OPUS_COLLECT_ALL` (collect every available fertilizer, also from animals without a feed job):
mirror +1,667 [+965, +2,461]; zoo 224 games +1,071 [+239, +1,954]; Local-LB 160 games +2,287
[+930, +3,812] (every opponent positive: +0.8k to +3.1k). Wins unchanged (already ~100%). The
cleanest candidate of the rotation: one condition in `compile`, no compile-time cost, positive
against every opponent family. Kaggle losses 2 and 3 missed 17 and 14 collections.

## Melon race on frozen replays (17:55)

`melonrace` = OPUS_RACE (opponent's ripe melons expected early) + OPUS_RACE_DEADLINE=10 +
OPUS_SALE_TIE_NOW (all products): frozen top-10 -6,770 [-13,958, -120] (own -4,502, wins 134 ->
126); frozen Kaggle -1,274 [-4,264, +1,822]. Strongly negative: tie-now applies to every product
(selling everything at the first tied hour) and the early melon deadline adds return work.
Separating the parts: `tie_melon` (ties -> now for melons only), `race_dl` (deadline without
tie-now), `tie_all`, and `melon12` (`OPUS_MELON_TARGET=12`: raise day 0-2 melon counts to 12,
extra melons trimmed first) running on frozen replays.

## More results (18:05)

- Wider route search on the full zoo (224 games vs X13): +445 [-870, +1,733] (per opponent -1.3k
  to +2.6k). With mirror +2.0k and mixed smokes, not robust enough for 3.4x compile time; the
  systematic part is -$730 wages per game. Not a candidate as is.
- Frozen replays, melon parts: `tie_melon` (ties -> now for melons only) top-10 -601 [-4,417,
  +2,913], Kaggle +37 [-281, +339]: nothing changes because melons reach the shed only at the
  evening deposit. `race_dl` (melon deadline 10 + opponent's visible melons early, no tie-now)
  -1,576 / -1,306 (within noise; 10 is after the opponents' hour-9 sales). `tie_all` (ties -> now
  for every product) top-10 -2,745 (own -2,507, wins 134 -> 130): the loss in `melonrace` came
  mostly from dumping every product at the first tied hour.

## Opponent anticipation, hour by hour (18:02; user: "anticipate this kind of stuff from opponents,
## hour by hour and in general")

Data: `tools/hourly_audit.cpp` (per player, day and product: dawn visible supply, dawn shed, units
sold at each hour) on 1,200 top-team games, our 36 Kaggle games and 160 Local-LB games.
`q56.py` scores forecasts made at dawn on cumulative units sold by each hour (CWAPE: what decides a
race). Held-out top teams / Kaggle opponents / Local-LB opponents:

| product | trailing (ours now) | blend (visible daily, trailing shape) | mix (visible daily, 0.5 trailing + 0.5 learned shape) |
|---|---|---|---|
| melon | 1.48 / 1.66 / 1.68 | 0.73 / 0.68 / 0.39 | 0.70 / 0.61 / 0.37 |
| milk | 0.87 / 0.67 / 0.70 | 0.83 / 0.65 / 0.66 | 0.80 / 0.61 / 0.60 |
| wool | 1.00 / 1.02 / 0.93 | 1.11 / 0.90 / 0.74 | 1.03 / 0.90 / 0.73 |
| strawberry | 0.68 / 0.68 / 0.75 | 0.71 / 0.75 / 0.79 | 0.67 / 0.72 / 0.77 |
| egg | 0.75 / 0.58 / 0.66 | 0.85 / 0.65 / 0.73 | 0.90 / 0.63 / 0.64 |

Best per cell: melons mix everywhere (error halved or better); milk mix (Local-LB: learned shape
0.59); wool mix or blend for Kaggle and Local-LB opponents, trailing = learned shape (1.00) for top
teams; strawberries trailing except top teams (mix 0.67 vs 0.68); eggs trailing except Local-LB
(mix 0.64). So anticipation pays for melons and helps milk and wool; eggs and strawberries stay on
the trailing forecast. Key learned fact: on their first melon day,
top teams have sold 1% of their melons by hour 3, 5% by hour 8, 31% by hour 9 and 64% by noon; our
melons must be sold by hour 8 to be first. Hourly errors stay large (0.6-1.0): opponents' hours are
noisy, so decisions should use the forecast's expected cumulative supply, not a point hour.

Implementation (`OPUS_OPP_PRIOR`, table `exp_patch/source/opp_prior.hpp` generated by q56.py):
melons, milk and wool use the mix forecast in the executor's sale DP and in the return planner;
`OPUS_RACE_EARLY` lets the DP-chosen delivery hours (`OPUS_RACE_DP`) start at 6 and 8; ties sell
now for melons only. Running: frozen replays (`oppprior`, `oppprior_race` with traces) and mirror
`oppprior_race`.

Result mirror `oppprior_race` (18:08; OPUS_OPP_PRIOR + OPUS_RACE_DP + OPUS_RACE_EARLY + melon
tie-now): 28-0-4, +9,736 [+3,694, +20,100]. One outlier: seed 705 seat 1 +154,611 (the X13
opponent collapsed to $35.9k; X13 has no recovery level). Without it the other 31 games average
about +5.1k; own about +0.4k, opponent about -4.7k: a competitive gain (we sell milk, wool and
melons ahead of an opponent that sells late). Debug on a frozen Kaggle game (loss 4, DC10_DEBUG):
the DP picked delivery hour 6 for the 36 day-10 melons, which is unroutable (new hires act from
hour 1, melons ~4 tiles away): status 2 with 0 attempts, then the default hour-20 plan. Fixed in
`OPUS_RACE_STEPS` (an unroutable early delivery retries +2, +4, +6, +8 hours). Mirror
`oppprior_steps` running; frozen `oppprior`, `oppprior_race` running.

## Anticipation stack on frozen replays (18:28; 145 top-10 games + 31 Kaggle games, opponent frozen)

| Probe | top-10 margin | own | wins (base 134) | Kaggle margin |
|---|---|---:|---:|---|
| oppprior (hourly forecast only) | -1,045 [-5,420, +3,212] | -1,679 | 133 | +99 [-2,068, +2,293] |
| oppprior_race (+ DP delivery hours from 6, melon tie-now) | **+2,883 [-1,866, +7,701]** | +566 | 136 | +649 [-1,948, +3,313] |

Mirror: oppprior_race +9.7k (about +5.1k without the collapse outlier), oppprior_steps 30-0-2
+9.4k. The race version is positive in every bed, significant only in the mirror. Melon timing
in its frozen traces (`tools/melon_audit`): our first melon sale moved from day 10 hour 23 to
mostly hours 14-18 (93 of 176 games), hours 6-9 in 22 games. Most games still sell after the
opponents' hour-9 start, because the DP's hour-6 delivery is unroutable and the plan falls back
to the default deadline; `OPUS_RACE_STEPS` (retry +2..+8 h) is the fix (frozen run queued).

Result frozen `oppprior_steps` (18:40; + retry unroutable early deliveries): top-10 +2,055 [-2,706,
+6,853], wins 134 -> 137, own -414; Kaggle -202 [-3,297, +2,722]. Mirror 30-0-2, +9.4k. Our first
melon sale moved from day 10 hour 23 to hours 10-11 (110 of 176 games), hours 6-9 in 22. Physical
limit: hires act from hour 1, melons sit ~4 tiles from the shed, so a harvest-and-return trip
lands at hour 9-10; opponents start at hour 9 (31% of their first-day melons). Beating them needs
melons planted nearer the shed (day-0 placement: melons are harvested once, then the tiles are
free) or the farmer (who persists overnight) ending day 9 next to the melons, harvesting at hour
0-1 of day 10. Both are compiler routines, not network decisions.

Anticipation stack verdict so far: positive in every bed (mirror about +5k per game excluding one
collapse, frozen top-10 +2.1k to +2.9k with 2-3 more wins, Kaggle frozen neutral), significant
only in the mirror. Worth a main-session validation on >= 4 seed sets and the Local-LB panel.

## Anticipation stack on Local-LB (18:56; `exp/x13_oppsteps` vs `exp/x13_slot`, 160 games)

OPUS_OPP_PRIOR + OPUS_RACE_DP + OPUS_RACE_EARLY + melon tie-now + OPUS_RACE_STEPS: +3,007 [+1,048,
+5,013]; every opponent positive (ahmed +4.4k, arlene +3.4k, arsgorynich +4.9k, cha22 +1.8k,
yannik +0.5k). The Local-LB family is the Kaggle public plan (12 melons, sold from hour 9), so
this is the closest local proxy for most Kaggle games. With mirror +5k (excluding one collapse)
and frozen top-10 +2.1k (within noise), the anticipation stack is the second strong candidate
after collectall.

Result frozen `steps1` (19:00; retries in 1-hour steps): top-10 +2,698 [-2,083, +7,534], wins 135;
Kaggle +1,172 [-1,828, +4,229]. First melon sale: day 10 hour 10 in 91 games, hours 6-9 in 31
(2-hour steps: hours 10-11). One hour after the opponents' typical first sale; the remaining hour
needs an earlier start (the farmer ending day 9 near the melons, or melons on the tiles nearest
the shed).

`melon12` (OPUS_MELON_TARGET=12 on frozen replays) stopped after 70 minutes without finishing: the
extra melons multiply the per-unit trim-and-recompile rounds on days 0-2 (4x+ the CPU of other
runs). Not usable as implemented; the melon count is tested instead with the Kaggle opponents'
opening (`OPUS_OPENING_MELON`: day 0 = at most 2 sheep, at least 12 melons) on Local-LB.

Result `x13_openmelon` (19:07; Local-LB 160 games, day 0 = at most 2 sheep, at least 12 melons):
-4,581 [-9,318, +56], wins 100% -> 88% (every opponent negative, -3.1k to -7.0k). Copying the
Kaggle opponents' opening breaks the network's later plans (as the feasible-style probes did) and,
with our hour-23 melon sales, the extra melons meet the opponent's supply. Testing the same
opening with the anticipation stack (melons sold early): `x13_openmelon_ops` running.

## 4th quadrant results and the "both richer" anomaly (19:15)

`OPUS_Q4_DAY=11` (buy the 4th quadrant on days 11-13): mirror 13-0-19, -1,038 [-3,326, +1,207] with
own +8,194 and opponent +9,231; frozen top-10 -975 [-5,226, +3,110] (wins 134 -> 131); frozen
Kaggle -2,158 [-5,338, +875]. Not a gain as implemented.

Why both sides got richer in the mirror (user: "strange, investigate"; 8 traced games, seeds
700-703, 35 s): the shops differ completely between the base and the 4th-quadrant game on the same
seed (for example 700_0: base 3 pizza, no yarn; q4 1 yarn, 1 pet cafe, 2 pizza; same count, 8 shops
by day 24). Shop draws share the night RNG with weed spawns (one draw per empty tile on both
farms), so 25 more mostly empty tiles shift the stream from day 11 on and reshuffle the shops for
both players. In those 8 games both sides gained ~$6k of wool (more yarn stores) and $6-7k of milk.
With ~16 independent shop draws and shop effects of +-$10-20k per game, the opponent's +9.2k is a
shop lottery, not a 4th-quadrant effect.

Consequence for every paired comparison (mirror, zoo, Local-LB, frozen): any probe that changes the
number of empty tiles (land, planting, clearing) reshuffles the shops after the first difference,
so the games are paired only until then. The margin stays meaningful (both players face the same
shops), but own/opponent money splits (e.g. routesearch own +4.0k / opponent +1.9k, hold probes)
mix the probe's effect with the shop lottery and should not be read as causal.

Method change (user, 19:13): screen ideas on small runs first (8 traced mirror games take ~35 s;
30 frozen games ~2 min), confirm only the survivors on long panels.

Result `x13_openmelon_ops` (19:20; Local-LB 160 games, Kaggle-opponent opening + anticipation
stack): -3,696 [-9,654, +2,194] vs X13 (wins 91%), -6,703 [-11,715, -1,800] vs the anticipation
stack alone. The 2-sheep/12-melon opening is harmful even when melons are sold early. Closed; the
melon count stays with so2 (10 by day 3).

Screen (8 traced mirror games, 35 s each): anticipation stack 8/8 wins +2.1k; + collectall 8/8
+2.7k. Local-LB confirmation of the combination (`x13_collect_ops`) running.

## Melon race: what it takes to be first (19:40)

- Placement (`day_policy_local/source/placement.hpp`, Staged): on the opening day animals are placed
  first (tiles next to the shed are soft-reserved for them), then melons, and wheat is pushed away
  from the shed when melons are planted; melons end ~3.9 tiles from the shed, like the opponents'
  (4.0). Day-1 melons get no priority.
- Physical limit with fresh hires: a hire acts from hour 1, walks ~4 tiles, harvests, walks back and
  deposits: melons in the shed at hour 9-10 at best. The DP-chosen early deliveries reach hour 10-11
  (`steps1`: hour 10 in 91 of 176 frozen games). Opponents sell from hour 9: a tie at best.
- The lever: the farmer persists overnight. We have ~16 idle worker-turns at hours 20-23. If the
  farmer ends day 9 (and 10, 11) next to the melons ripening tomorrow, it harvests at hour 0 and
  deposits by about hour 5, four hours before the opponents. This needs an end-of-day position goal
  in the route solver (the farmer's final tile is free today). Top next step for the melon race;
  second: day-1 melons placed with the same priority as day-0 melons.

Local-LB, melon opening (Kaggle opponents' 2 sheep + 12 melons) with early selling: the extra
melons slowed the routes (first melon sale at hour 18) and pushed the herd back (9.6 vs 11.1 animals
on day 7): -3.7k. Count stays with so2 (10).

Local-LB combination (19:30; `x13_collect_ops` = collectall + anticipation stack, 160 games): +4,070
[+2,111, +5,963] vs X13; every opponent positive (+1.3k to +5.4k); vs anticipation alone +1,063
[-321, +2,365]; vs collectall alone +1,784 [-848, +4,172]. The best result of the rotation.
