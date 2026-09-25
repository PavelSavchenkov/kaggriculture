# sep24_BC_opus: weak points in full games and highest-ROI fixes (Sep 25)

Follow-up documents: `GAPS.md` (five gaps to gold level and fixes, latest), `BOUNDARY.md` (network vs compiler boundary, probes X6-X11),
`OPPORTUNITY_night_inventory.md` (open opportunity: shed inventory across the night),
`VALUE_MODEL.md` (how to train the missing value model), `ROTATION_LOG.md` (gaps 1-4 rotation:
probes, mirror bed, labour decomposition), `KAGGLE_REPLAYS.md` (our Kaggle submission's games, first loss).

Scope: the best agents of `experiments/v10/sep24_BC_opus` (there is no `sep25_BC_opus`; the
Sep 25 work lives in `sep24_BC_opus`, `HANDOFF_sep25.md`). Main subject: the headline
candidate `models/cand_v12_vadim6` (v12_cond + Vadim opening for days 0-5). To see weaknesses
that an opening pin might hide, the same analysis covers v12_cond without the pin, v13_w384b,
v11_style and the Majkel fine-tune (zoo_majkel).

All numbers are reproducible from this folder (section 7).

## 0. Summary

Ranked by measured or estimated margin per game against the Local-LB (main gate):

| # | Weak point | Where the error is | Evidence | Value |
|---|---|---|---|---|
| 1 | Herd too small and too late (3-4 animals fewer than top teams from day 7 on) | Day compiler: funding. The next-dawn cash reserve ignores certain overnight income, and trims drop the most expensive entity first. Day 0 loses the 3rd sheep in 100% of games; the lower day-6 wool income then forces more trims on days 6, 7 and 9. | Network asks for ~20 animals by day 12 (top teams buy 19.5); our variants buy 14.7-18.0 (Vadim candidate 16.4). Lost games are the small-herd games. | Measured: +$5.1k [+1.8k, +8.6k], wins 94% → 100% (X1) |
| 2 | Loses sale races: melons sell for $42 less per unit than the opponent's | Day compiler: market model. The opponent forecast is a trailing 3-day average (blind to its visible ripe crops), receipts have a fixed hour-20 deadline, the route solver deposits at the deadline, and sale-DP ties go to the last hour. | We sell first on 0% of shared melon days; opponent starts at hour 9 and gets $232-234, we start at hours 17-22 and get $155-169; top teams race too (first seller +$19.6/melon). Smaller losses on wool, milk, tomato. | Probe: +$2.4k [-0.1k, +5.4k] (X2); with #1: +$6.8k [+3.1k, +11.1k], wins 100% (X4). Full fix worth more: own melon revenue alone is -$5.3k/game. |
| 3 | Wages $1.7k/game above top teams, 13 hires on 24-27% of mid-game days (top 2%) | Day compiler: shed returns are added after routing as hard deadlines; the solver meets them by hiring. | Routes for the requested work need 10.25 hires (mode 11, like top teams); returns raise it to 11.65 (+$165/day). | Dropping those returns saves $1.3k wages and +$2.0k own money but loses the race: margin -$1.8k (X5). The fix is cheaper returns, not fewer. |

What is not a weakness (checked, section 4): the 4th quadrant, crop yields and losses,
discards, unsold stock, feeding/care, collection frequency, fertilizer per animal, net wheat,
and sale prices for products with shop demand.

The network is not the main source of these losses: on the days that matter it asks for
top-team herds and top-team workloads. The three losses are conceptual gaps in the day
compiler. All five variants share them, so the opening pin does not hide them (the pin itself
is trimmed on day 0 like every other variant).

## 1. Data and method

- Our games (all traced): 5 variants x (5 Local-LB behaviours x 32 games + 5 C++ opponents x 32
  games + 32 self-play games), seeds 700-715, both seats: 1,760 games. Local-LB games run in the
  official Python engine and are re-simulated in the C++ engine (final money checked).
- Top reference: 2,809 recorded Kaggle episodes of Sep 18-23 with a current top-10 team
  (4,134 top-10 perspectives; 3,098 of them against a top-20 opponent).
- `tools/ledger.cpp` replays each trace and records, per seat and day: exact revenue and spend by
  category (market prices replayed in the engine's lockstep order; checked against the engine's
  counters, 0 mismatches), purchases, plantings, farm composition, harvests, worker actions,
  sale hours, and every loss (decay, weeds, escapes, cap clipping, lost care bonus, discards,
  unsold stock). Per crop cohort and per animal: output and fate.
- Per-dawn compile reports (intent, trims, reasons) for 128 C++ games; compiler debug logs for
  5 full games.
- Experiments: env-gated patches in a copy of the experiment sources (`exp_patch/`; defaults
  reproduce the baseline games exactly), 160 paired Local-LB games each (Vadim candidate, 5
  behaviours, seeds 700-715, both seats), seed-clustered 95% bootstrap CIs (`paired.py`).

Context: our economy is not behind top teams in total. Revenue 132-138k vs 138k; in self-play
our agent ends with $115.7k per seat vs $108k in top-vs-top games. The gaps are specific.

## 2. Weak point 1: herd size and timing (compiler funding)

### What differs

Animals at dawn (Local-LB games; top = mean of top-10 teams' games):

| Day | 1 | 6 | 7 | 9 | 12 | 15 |
|---|---:|---:|---:|---:|---:|---:|
| Top teams (DSM/DECEM/Vadim/Majkel/Mother-Goose) | 5.0-5.3 | 5.0-6.1 | 11.2-11.9 | 12.8-13.6 | 16.2-20.6 | 17.0-21.4 |
| ours: Vadim opening | 4.0 | 4.0 | 7.2 | 9.1 | 14.8 | 15.9 |
| ours: v12 (no pin) | 4.0 | 4.0 | 7.7 | 9.7 | 14.1 | 15.2 |
| ours: w384 / v11 / Majkel (day 9) | | | | 9.8 / 9.8 / 12.0 | | 16.9 / 13.9 / 18.0 |

Every top team opens with 2 cows + 3 sheep; every one of our games has 2 cows + 2 sheep on day 1.
Among top-team games, win rate rises with animals at day 12 (56% at 12-18, 62% at 18-20, 67% at
20-22, 71% at 22-25). In our lost Local-LB games (80 of 800, all variants) we have 14 animals at
day 15 vs 16 in won games, and milk/wool/egg revenue gaps turn negative. Margin correlates with
herd size (r = 0.36) and with the milk/wool gaps (r ≈ 0.4).

Value of one animal in our games (collected output x our sale price - feed; excluding purchase
cost and the ~20-27 fertilizer each produces): day-0 sheep $2,750-2,925, day-0 cow $2,600-2,800,
day-6 cow $1,900-2,100, day-6 goose $1,250.

### Why: the network asks, the compiler trims

Per-dawn reports, 128 C++ games per variant (vadim6 and v12 vs king_rc4 and ahmed_v25):

| Day | Network asks (goose/cow/sheep) | Dawns with trims | Mean units trimmed |
|---|---|---:|---:|
| 0 | 0 / 2 / 3 (+ 15-16 crops) | 100% | 1.0 (a sheep) |
| 6 | 1.7-1.9 / 3.1-4.1 / 0.5-0.6 + land + 15 crops | 94-100% | 2.2 |
| 7 | ~1 animal | 69-72% | 0.75 |
| 9 | 3.0-3.4 animals + land + 17 crops | 31-44% | 0.4-0.6 |

The requested animals over days 0-12 add up to ~20, the top-team figure (19.5 bought). Our
variants buy 14.7-18.0. Trims remove the most expensive entity first, so they always remove
animals.

Day 0 in detail (vadim6, seed 700): intent 9 wheat + 7 melons + 2 cows + 3 sheep. The plan runs
out of cash for the hour-19 seed purchase (~$60-100 short), so the first trim is needed. With the
crops-first order it then takes three more melon trims; these come from the next-dawn reserve,
which requires ~$186 at day end (feed for 5 animals); with the reserve off on day 0, X1 drops one
melon on average. The reserve counts cash and shed goods
only, not the fertilizer every animal makes overnight (5 x ~$90 on day 1). Top teams end day 0
with $29 and fund day 1 from that fertilizer. We end day 0 with $463 unspent and one sheep fewer.

Day 6 is the cascade: top teams earn $3,246 from the first wool harvest (3 sheep) and buy ~6.5
animals and land; we earn $2,380 (2 sheep), and the same intent is ~$500 short, so ~2 cows go.
With 3 sheep, the same day-6 intent compiles without trims (seed 700: 11 animals on day 7 instead
of 7).

### Experiment X1 (value probe, not the proposed implementation)

Day 0 only: trim crops before animals, and no next-dawn reserve on day 0
(`OPUS_TRIM_D0_CROPS=1 DC10_RESERVE_FROM_DAY=1`). 160 paired Local-LB games:

| Opponent | Margin change [95% CI] |
|---|---|
| ahmed-productive-wheat | +4,013 [+922, +7,177] |
| arlene idle seller | +3,336 [-1,404, +7,693] |
| arsgorynich | +4,664 [+797, +8,615] |
| cha22 | +8,334 [+3,713, +13,186] |
| yannik latest | +5,087 [-650, +11,590] |
| pooled | **+5,087 [+1,759, +8,611]; wins 94% → 100% (+6.2 pts [+1.9, +11.2])** |

Mechanism confirmed in the ledger: 5 animals on day 1 (3 sheep), 12.1 on day 9 (was 9.1), 17.2
on day 12 (14.8), 17.7 on day 15 (15.9); one melon fewer. Own money +$2.5k, opponent money
-$2.6k (shared markets).

## 3. Weak point 2: sale races (compiler market model)

### Melons

Melons have no shop demand (town takes 1/day), and the price falls roughly as 250 - 0.01·x²
with supply, so the first seller of the day-10 harvest takes the price.

| Local-LB games, day 10-11 | first melon sale (day 10) | units | price |
|---|---|---:|---:|
| Local-LB opponents | hour 9 | 72 | $232-234 |
| ours (vadim6 / v12) | hours 17-22 | 69 / 79 | $166 / $155 |
| top teams (top-vs-top) | hour 9 (491 games), 6-8 (178) | 73 | $201 |

In top-vs-top games the seat that sells first gets $209.5 vs $189.9 per melon. Our own melon
revenue would be $5.3k/game higher at the opponent's average price; the margin swing of winning
the race is larger (the opponent then sells second).

Day 10 of one game (vadim6 vs ahmed, seed 700): 11 workers from hour 2, but our 42 ripe melons are
harvested at hours 5-14 (opponent: 60 by hour 9), deposited at hours 17-20, and sold at hour 23
after the opponent sold 60. Causes, all in the compiler:

1. Opponent forecast = its trailing 3-day average flow per hour, which is 0 for melons before day
   10. Its 72 ripe melons are visible on its farm, but visible stock is used only on days 28-29.
2. Market returns have one fixed deadline (hour 20), and the route solver deposits at the
   deadline, not when a worker passes the shed.
3. With no demand and no expected competitor, selling at hour 20 or 23 has equal value; the DP
   sells now only if strictly better, so ties go to the last hour.

The network side is small: it plants 11-13 melons (top 12); day-0 budget trims cost one melon.

### The same gap on other products

Game-days where both farms sell the same product (Local-LB, vadim6 and v12):

| Product | our price - opponent price | we sell first | mean sale hour ours / opponent |
|---|---:|---:|---|
| melon | -$42.8 | 0% | 21.3 / 5.2 |
| wool | -$2.7 | 17% | 15.8 / 7.7 |
| milk | -$2.0 | 21% | 14.5 / 9.1 |
| tomato | -$1.7 | 35% | 19.0 / 16.4 |
| fertilizer | -$0.9 | 12% | 15.9 / 6.9 |
| carrot, egg, strawberry, wheat | ≈ 0 | 11-24% | later |

The loss grows as demand gets thinner: steady shop demand absorbs late timing; no-demand or
single-shop products pay for it.

Experiment X5 shows the same effect from the other side (section 4): dropping same-day deposits
raises our own money (+$2.0k) but lets the opponent's morning sales go first, and the opponent
gains more (+$3.8k).

### Experiments X2 and X4 (value probes)

X2 (`OPUS_SALE_TIE_NOW=1 OPUS_RACE=1 OPUS_RACE_DEADLINE=12`): ties sell now; the opponent's
visible ripe melons count as supply at the current hour; melon market returns due by hour 12.
Result: +$2,385 [-92, +5,350], wins -1.2 pts. Melon price only rises to $177 (opponent $234)
because deposits still arrive at hours 11-12 (the solver deposits at the deadline; hour 6-9
deadlines make the route solver reject the input). X4 = X1 + X2: **+$6,804 [+3,068, +11,109],
wins 94% → 100%**.

## 4. Weak point 3: wages (compiler returns)

- Wages $6.4-7.2k/game (all variants) vs $5.4k for top teams, for fewer hires (272 vs 286).
- Days 11-27: mean hires 11.7 vs 11.07, but day-to-day std 0.96 vs 0.46; 13 hires on 24-27% of
  days (top 2%), 11 hires on 35-40% (top 68%). The Fibonacci wage makes a 13-hire day cost $609 vs
  $232 for 11.
- The work itself is not spikier: collections per animal-day and lot sizes match top teams (cow
  0.34 vs 0.31, 3.1 vs 3.3 units; sheep identical), harvest counts vary similarly. We do less work
  per day (146-151 productive actions vs 155) and more walking (122-125 moves vs 112).
- Compiler logs, 95 mid-game days (5 games vs king_rc4): the route for the requested work needs 10.25 hires (mode 11,
  $193/day); adding the same-day market returns (deposit ~50 units by hour 20) raises it to 11.65
  ($358/day). The return rule budgets $2/unit (~$100/day) and never compares with wages.
- Top teams use the free night deposit more (1,337 vs 1,125 units carried overnight) and sell 626
  units at hours 0-5 (ours 263); we sell 1,038 at hours 18-23 (top 577).

Experiment X5 (`OPUS_RETURN_WAGE=1`: keep the market-return plan only if its sale gain exceeds the
extra wages): wages -$1.3k (now $5.8k), own money +$2.0k, but opponent money +$3.8k:
**margin -$1,800 [-6,586, +1,225], wins -6.9 pts**. The evening sales were beating these
opponents' morning sales. So the wage premium mostly buys a race win; the problem is that our
routes buy it with extra hires, while top teams get similar timing with 11 hires.

## 5. Checked and not weak

- Land: 82% of top-team games stay at 3 quadrants (Majkel never buys the 4th); 4-quadrant games
  win 61% vs 59%. We always stay at 3.
- Crops: we harvest more per plant than top teams (wheat 4.6 vs 4.0, strawberry 7.85 vs 7.28,
  tomato 7.9 vs 7.2, carrot 3.2 vs 2.9, melon 6.0 vs 6.0), with near-zero decay and weed losses.
- Animal service: feed 0.84 vs 0.80, care 0.95 vs 0.97, output per animal equal or higher.
- Discards < 1 unit/game, unsold stock at the end $0, held product at the end ~$20.
- Fertilizer: collection per animal-night 0.92-0.93 vs 0.96; use on crops 190 vs 190; the lower
  sales (152 vs 240) follow from the smaller herd.
- Wheat: net sales 326-339 vs 287 (top teams buy and sell more gross wheat, but net is equal).
- Sale prices for products with shop demand: equal to or better than the opponent's.

## 6. Conceptual fixes (proposals, in ROI order)

### A. Funding as a day-ahead cash-flow model, with value-ordered trims (weak point 1)

1. The next-dawn reserve protects against a funding collapse (seed 706 in the handoff). Keep it,
   but compute tomorrow's dawn liquidity from everything certain: cash, shed stock, fertilizer on
   every animal tonight, product held on animals and ripe crops that tomorrow's plan collects,
   valued with price impact and a stress discount. Top teams run day 0 to $29 because of exactly
   this income.
2. When an intent is over budget, remove what closes the gap at the least value lost, not the
   most expensive entity. Value per dollar can come from the network (another session is
   currently adding `DC10_TRIM_MODEL`, a decoder trim order by log-likelihood per dollar) or from
   an economic table (per-entity net value by placement day, section 2). Prefer the smallest
   change that closes the gap (one melon seed, a deferred seed purchase, a hire) before an animal.
3. Expected value: X1 measured +$5.1k with the crude day-0 version; the general version also
   targets days 6, 7 and 9.

### B. Competition-aware market model (weak points 2 and 3)

1. Opponent supply forecast from its visible state, every day: ripe crop yield and held animal
   product on its farm, its inferred unsold stock (History already tracks it), and its sale
   timing learned from History (for example, share of its stock sold within k hours of harvest);
   a prior from top-team behaviour for first-wave days (melons: hours 6-9 of day 10). The trailing
   3-day average stays as the steady-state term. Note: "opponent held stock in the forecast" was
   worse in the compiler experiment's frozen-replay continuations; that version added the whole
   stock at once. The timing model is what matters.
2. Receipts with time value: the sale DP should give, per product, the value of one more unit
   reaching the shed at each hour (a value curve, not one deadline). The route solver then gets
   graded receipt targets and deposits when a worker passes the shed (PLACE), as top teams do.
3. Ties and risk: with an uncertain competitor, an earlier sale weakly dominates on thin markets;
   use a per-hour discount that scales with expected competing supply, instead of "strictly
   better now".
4. Evidence of value: melons alone -$5.3k own revenue; X2 +$2.4k with lazy deposits; X4 +$6.8k.

### C. Returns and hires in one route objective (weak point 3)

Returns are added after routing as hard deadlines, and the solver meets them by hiring. Make
deposits part of route construction: each unit's value by arrival hour (from B) against the
marginal Fibonacci wage of the next hire. Keep the race value (X5 shows that dropping returns
loses it); the target is top-team timing at ~11 hires. Expected: most of the ~$1.7-3k/game wage
gap without losing margin.

### D. BC training and conditioning

- No change needed for the herd or wages: the network asks for top-team herds and top-team
  workloads (its work fits 10-11 hires). The losses come from the compiler.
- After A-C, the closed-loop states move toward the teacher's (3 sheep on day 1, larger herds on
  day 9), which should also improve later intents. Re-screen the opening pins after A: the Vadim
  pin was chosen under a compiler that trims the day-0 sheep in every game.
- Consider compiler-aware inputs (expected hires for today's intent, funding shortfall) only if
  spikes remain after C; a DayIntent that allows "harvest by tomorrow" is not supported by the
  evidence yet.

### Next steps

1. Implement A properly (cash-flow reserve + value-ordered, minimal-gap trims); gate on the full
   panel (Local-LB, extra, C++, replays) and on the v12 and w384 variants, not only vadim6.
2. Implement B with graded receipts and the visible-supply forecast; gate on the same panel and
   on top-team replay continuations from day 9.
3. Then C, measured by margin, not by wages.

## 7. Reproduction

- Build: `conda run -n kaggriculture cmake -S ../../experiments/v10/sep24_BC_opus -B build` (tools
  `full_games`, `opus_lb_bridge`); `cmake_ext/` builds `full_games_reports` (per-dawn reports);
  `exp_patch/` + `build_patch/` = experiment copy with the OPUS_* flags (defaults = baseline).
- Games: `run_games.sh cpp|lb`; Local-LB traces via `tools/lb_trace_play.py` +
  `tools/finalize_trace`; `our_list.py` builds the ledger list.
- Ledger: `tools/ledger list prefix` (`LEDGER_SALES=1` adds sale events); top list `mk_top.py`.
- Analysis: `cache.py`, `compare.py`, `q*.py` (one question each); `tools/day_dump` (one day,
  one product, hour by hour).
- Experiments: `run_exp.sh <name> <procs> ENV=...`, results `exp/*`, `paired.py games/vadim6/lb exp/<name>`.

## 8. Zoo and self-play panel (added later on Sep 25)

Our Vadim-opening candidate (baseline, X10 = network-ordered budget re-plan, X12 = X10 + melon race
probe) against 7 unpatched BC models in C++ full games, seeds 700-715, both seats (32 games each,
672 games; `run_zoo.sh`, traces in `zoo/`). Probe flags apply only to our agent
(`opus_patch_off` in `exp_patch`). The zoo opponent "cand_v12_vadim6" is the unpatched baseline
itself.

| Opponent | Baseline wins, margin | X10 wins, margin | X12 wins, margin |
|---|---|---|---|
| itself (unpatched) | 10/32 (12 ties), 0 | 32/32, +7,445 | 29/32, +7,647 |
| v11_style | 28/32, +6,878 | 32/32, +12,472 | 32/32, +10,791 |
| v12_cond | 16/32, +87 | 29/32, +4,666 | 29/32, +4,725 |
| v13_w384b | 15/32, -873 | 28/32, +6,517 | 26/32, +3,553 |
| zoo_dsm | 20/32, +455 | 21/32, +3,637 | 24/32, +3,072 |
| zoo_majkel | 19/32, -5,065 | 30/32, +7,338 | 26/32, +5,852 |
| zoo_vadim | 22/32, +1,600 | 29/32, +5,368 | 31/32, +11,120 |

Paired vs the baseline on the 6 other opponents (192 games, seed-clustered 95% CI): X10 +6,153
[+3,687, +8,869], wins +25.5 pts [+15.6, +35.9]; X12 +6,005 [+3,503, +9,269], wins +25.0 pts.

What the baseline misses against our own zoo (revenue gaps ours - opponent, `zoo_report.txt`):

- The same herd gap as on Local-LB. Every zoo model shares the day-0 trim (4 animals on day 1),
  but the models that beat the baseline reach bigger herds by day 9 (Majkel fine-tune 11.8 vs our
  9.3, w384b 11.3 vs 10.2) and win on animal products: vs Majkel milk -6.5k, wool -3.4k,
  fertilizer -1.6k; vs DSM eggs -2.3k, milk -2.1k, fertilizer -1.8k; vs Vadim wool -3.1k.
  X10 lifts our herd to 12.3-13.1 on day 9 and 18-20 on day 15, and these gaps turn neutral or
  positive.
- Mirror matches (baseline vs itself) are decided by sale races: margin correlates 0.6-0.7 with
  the wool, melon and strawberry revenue gaps. X12 moves our sales from the evening to hours 0-5
  (348 vs 28 units) and raises our melon price (203-228 vs 199-217).
- Remaining after X10: the DSM fine-tune (21/32, +3.6k) plants more wheat (166 vs 142) and wins
  wheat by 3.2k; with the bigger herd X10 buys 0.4-1.4k more wheat but does not plant more (the
  network's wheat count does not follow the herd); tomato-heavy variants (w384b, v11: 16-17
  tomatoes vs our 8-11) keep a 1.6-3.1k tomato edge (a strategy difference, not a clear miss).
- The Vadim opening's Local-LB advantage does not carry over to strong BC opponents: the baseline is
  even with v12 and w384b head to head.

## 9. Why not base the agent on the Majkel fine-tune, or combine models? (added later on Sep 25)

Our agents as the agent under test (same seeds; Local-LB 160 games, C++ opponents 160 games):

| Agent | Local-LB | C++ opponents |
|---|---|---|
| Vadim candidate (v12_cond + Vadim opening) | 93.8%, +9.2k | 100%, +21.4k |
| Majkel fine-tune (zoo_majkel) | 91.2%, +11.5k | 99.4%, +18.9k |
| v13_w384b | 96.2%, +11.7k | 100%, +23.8k |

Why the Majkel fine-tune builds a bigger herd: its opening fits the budget. Day 0: 1 cow + 3 sheep
+ 5 melons (ours: 2 cows + 3 sheep + 7 melons, trimmed to 2 sheep); it buys cows on days 3-4 with
cash our candidate leaves idle; the 3rd sheep's day-6 wool funds +5 animals on day 6. Result: 10.9
animals on day 7 (ours 7.2), 12.0 on day 9 (9.1). It plants fewer melons (6 vs 11). This is the same
problem X10 fixes, solved inside one teacher's plan.

With X10 (network re-plan of over-budget days), Local-LB, 160 paired games:

| Agent | Margin, wins | vs Vadim candidate + X10 |
|---|---|---|
| Vadim candidate + X10 | +14,527, 100% | - |
| w384b + X10 | +13,273, 97% | -1,254 [-5,963, +3,677] |
| Majkel fine-tune + X10 | +12,417, 100% | -2,110 [-5,120, +1,304] |
| Majkel opening days 0-5 + w384b + X10 | +13,904, 96% | -623 [-3,968, +2,894] |
| Majkel opening days 0-8 + w384b + X10 | +13,589, 100% | -938 [-4,191, +2,148] |

Head to head, both sides with X10 (C++, 32 games): Vadim candidate beats the Majkel fine-tune 28/32
(+5,482; Majkel ends with 9.6 animals vs our 13.0) and is even with w384b (13/32 wins, +1,713).

Conclusions:

- The Majkel fine-tune's advantage was its budget-feasible opening. X10 gives that to any model;
  after it, the models are within noise on Local-LB and the Vadim candidate wins head to head.
- Mixing models inside a decision fails (the experiment's head-averaging ensemble: -1.0k): each BC
  model is a coherent package (Majkel's opening goes with fewer melons and progressive cow buying).
  Splitting by phase keeps each part coherent but adds nothing measurable once X10 is in.
- The principled combination is propose-and-evaluate: each model proposes a complete DayIntent,
  the compiler simulates each, and a value model picks one (`VALUE_MODEL.md`, use 1, with K models
  instead of K budget variants); then distill the chosen intents into one network (expert
  iteration). Candidates worth combining: Vadim candidate and w384b (even head to head).
