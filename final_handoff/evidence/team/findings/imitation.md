# Imitation findings

Written only by the Imitation session. Other sessions read it. Format: `- (time) finding. Number. Source.`
Imitation's home is experiments/v10/sep29_mm_copy/ (from Sep 29 18:25); older paths under sep28_top_lb_imitation still work (frozen).

## Findings

- (17:30, Imitation) Local league (the user's main test: arm vs our 8 real local agents, 12 games per pair, paired seeds,
  SHOP_CRN; experiments/v10/sep28_top_lb_imitation/runs/league/league.sh, scripts/league_read.py): the M&M copy CPP5c -1.21k
  (47% wins) vs d3crop +0.87k (54%); paired -2.08k (SE 0.57k): the copy earns +2.1k more itself, its opponents +4.2k more.
  hyb-melon68 and giovanni cma beat d3crop there. LEDGER.md 18:2x.
- (17:30, Imitation) Where the copy loses to real M&M (6 real M&M-vs-ours worlds; the arm plays M&M's RECORDED actions to
  dawn X, then the copy; exact, paired; SE 1-2k): copy +3.1k, M&M +9.5k, M&M to dawn 12 +6.5k, to dawn 18 +11.0k. So the gap is
  built on days 0-11 (~3.4k) and 12-17 (~4.5k); the copy's days 18+ beat M&M's. M&M's daily new crops / animals / land forced
  into the copy: no change. duel_dc11 keys DUEL_REC / DUEL_LOAN / DUEL_REC_UNTIL / DUEL_PLANT (build/v88p). LEDGER.md 17:3x.
- (17:30, Imitation) M&M-world bed (runs/duel/mm.sh: M&M's recorded play with a cash loan vs LIVE d3crop in 72 of M&M's own
  games; the copy live in the same worlds): at equal thin volume d3crop earns +1.3k milk / +1.2k wool more vs the copy. Days
  12-24 milk: M&M 34.3 units @ $126 at h0-2 and 18.3 @ $93 at h21-23; the copy 14.6 @ $100 / 32.2 @ $107; evening milk of both
  farms 54 vs 80 units. Frozen (recorded) play costs ~4k per game vs a reactive opponent (our sub's recorded play vs the live
  copy -7.2k vs -3.1k live), so this bed understates M&M. LEDGER.md 18:0x-18:2x.
- (17:30, Imitation) Own money: the copy's second melon wave is half of M&M's (melons produced dawn 12 -> 20: +16 vs +32;
  totals 64 vs 74) and tomatoes 117 vs 138 units; melon waterings d6-12 52 vs 63 per game [CAVEAT 19:40: DUEL_OPS skips h23 actions (night reset), like BC's retracted probe; treat watering counts as unreliable, production counts stand]; M&M spends $1.8k more on days 0-11;
  the copy has ~0.9 more geese (BC: a live-world effect, the net matches M&M's herd on M&M's states; cows -0.7 from compiler
  drops). LEDGER.md 18:3x.
- (17:30, Imitation) Offline (sale_score, forecaster on from day 1, 216 M&M games): our DP's schedule on M&M's own stock equals
  M&M's in the real market (strawberries +13, milk +18, wool 0 per product-day), so the seller loses only in the live
  interaction (ordering vs the opponent), not on a fixed market path. LEDGER.md 17:1x.
- (17:30, Imitation) Engine order: within an hour both players' orders are processed slot by slot (order index 0 of both, then
  1, ...), units of the same slot interleaved at shared quotes; a sale in an earlier slot executes fully before a later slot's
  sale. So "first" = earlier hour or earlier order slot. sim.hpp process_market.

- (18:20, corrected 18:50, Imitation) Where sellers sell (experiments/v10/sep29_mm_copy/runs/sell/sell_levels.txt from DENSE hourly
  paths of 396 real games, build/sell_path; days 12-24; level = market inventory minus its trailing 24-h mean, higher = cheaper):
  milk after-lot median M&M +3.8, the field +4.6, our subs +9.6 (live duels +8.4..+10.7); wool M&M +2.8, field +3.1, ours +11.0.
  Chance to sell milk when holding stock, by level <=-10 / -10..-5 / -5..-2 / +-2 / 2..5 / 5..10 / >10: M&M 60 / 29 / 18 / 12 / 9 /
  11 / 24%; field 52 / 29 / 21 / 16 / 15 / 18 / 28%; our subs 42 / 20 / 13 / 11 / 13 / 14 / 17% (lot 4.1 at <=-10). NOTE: this is
  observational; used as a response term it fails (entry below).
- (18:45, Imitation) Own-money gap located (copy CPP5c live vs d3crop in 36 M&M worlds, LOGERR=1 DC11_INTENTLOG=1 + DUEL_OPS
  plantings; experiments/v10/sep29_mm_copy/scripts/plant_read.py copy_i): days 0-12 per game, asked / planted / M&M planted:
  melons 10.9 / 10.9 / 12.3, mostly days 1-2 (copy 2.0 + 1.0, M&M 1.1 + 2.9); yield per plant equal, so -1.4 plants = the -9
  melons (~-$1.3k) later. Day 2: the copy's plan falls back in every game (trims 1.0; dawn cash $202; the asked strawberry is
  never planted) and its network asks 1 melon where M&M plants 2.9 + 0.6 strawberries (M&M funds days 2-4 by selling
  fertilizer / dawn wheat before h8: Weaknesses). Tomatoes 10.2 vs 11.4 plants (d8 1.4 vs 2.5, d12 0.5 vs 1.8) plus fewer
  waterings (BC: -15..-17) = -21 units (~-$1.3k). Wheat / strawberry / carrot plantings level.
- (18:50, Imitation) Opponent-model gate (72 audited M&M-world games; experiments/v10/sep29_mm_copy/scripts/opp_model_eval.py,
  opp_response_gate.py; runs/sell/opp_model_eval.txt, opp_response_gate.txt): (a) a reservation-level seller (sell up to trailing
  mean + c while stocked) predicts the opponent's hourly sales far worse than our learned forecast (M&M milk corr 0.13 vs 0.61,
  over-predicts +12..+32 units / day); (b) the fitted table (src/dc11/opp_response.hpp) as forecast x R(level + our deviation) /
  R(level) makes predictions WORSE (mirror milk MAE 0.705 -> 0.826): when we sell less than planned the opponent also sells less than
  forecast (0.40 vs 0.60): deviations co-move, observational level effects are not causal; (c) zeroing the forecast when the
  opponent holds no stock (true stock) cuts MAE 20-30% (M&M strawberry 0.61 -> 0.41, corr 0.41 -> 0.60). Causal response by forks
  (duel_dc11 DUEL_PERTURB) running.

- (19:00, Imitation) Seller model bug behind our evening dumps (experiments/v10/sep29_mm_copy/scripts/defer_check.py,
  runs/sell/defer_check.txt; 72 audited M&M-world games, days 12-24, thin products): with the night room free, the SCENARIO seller
  sells ~25% of the lot the deterministic plan wants this hour (h3-11: plan 0.94-1.20 vs sold 0.25-0.28 units per decision;
  h12-20: 1.16-1.38 vs 0.32-0.45); units roll on until the night-room charge forces them out (h21-23 decisions 3.5-5.1 units).
  Cause (market.cpp v95 l.430-456): each sampled path's value of "q now" uses that path's own optimal continuation (hindsight), so
  waiting is overvalued every hour (information-relaxation bias). In the mirror both sellers do it: the evening chicken. Fix
  under test: scenna=1 (one continuation schedule per candidate, scored on every sampled path). Also: our plans put lots where
  the forecast expects the opponent to sell MORE (1.1-1.6 vs 0.2-0.4 units; post-drain hours + the rival term; curse_check.txt).

- (19:25, Imitation) ROOT CAUSE of our missing dawn sales (all dc11-lineage agents, not only the copy): realize() orders the first
  hire wave (up to 10 hires, one order each) at the plan's first hour, the engine caps orders at 10 per turn, and Executor::act gives
  sales only the leftover slots (v95 l.426 / l.1302). On 9-10-hire dawns the seller's h0 sales are silently dropped. Audit (72
  M&M-world games, days 12-24, per dawn, decided / executed at h0): d3crop vs M&M strawberry 1.80 / 0.18, milk 1.31 / 0.14, wool
  1.06 / 0.07; mirror strawberry 2.24-2.36 / 0.17-0.20 (scripts/h0_check.py, runs/sell/h0_check.txt). Slot log (DC11_SLOTLOG,
  src/dc11/compiler.cpp): d14 h0 plan_orders 10 (10 hires), seller wanted 26 units, placed 0; 31 units at h1. Explains Weaknesses'
  live dawn gap (top teams sell ~30 strawberries per game at h0 at $148; ours 2.3, and 24.9 at h1 at $95). Fix: keep h0 slots for
  the seller (first wave <= 10 - sale slots - purchases; the rest start at h2): local key saleslots=-1 (sep29_mm_copy) and the
  Day compiler's dc12 fix; league test running.

- (19:40, Imitation) League results (experiments/v10/sep29_mm_copy/runs/league; scripts/league_read.py, league_pool.py; paired seeds,
  SHOP_CRN; "3 strongest" = d3crop / hyb_nolp / giovanni cma):
  * copy with BC's v3 main (CPP5c_v3) vs copy: +1.31k (SE 0.35k, 132 games); vs d3crop head-to-head +1.16k (83% wins); whole
    league margin +0.11k vs d3crop's +0.87k (seed 501 block).
  * dawn-slot fix (saleslots=-1) on the v3 copy: -1.72k (SE 0.31k, 132 games) against capped opponents (every local bed: all
    our agents hit the 10-order cap at h0 on 94-100% of dawns, so the dawn is uncontested and h0 = pre-drain price); +0.37k
    (SE 0.67k, 36 games) when the opponents also have the fix (contested dawn). Day compiler: saleslots=1 -1.2k on 200 pinned real
    games. So the every-day dawn lot loses; the fix needs contest-conditional h0 lots (dc12 saleslots=2) and live / contested
    judging.
  * BC hybrids vs hyb_melon68 (3 strongest, 36 games): hyb_v3m68 -0.34k, hyb_v3m68_s1 -1.15k, hyb_m68_ms +0.08k,
    hyb_v3m68_ms (v3 main + .decode mainshare 8 10) +1.26k (SE 0.54k), margin +1.86k, 75% wins = best agent measured;
    confirmation on all 8 agents + seeds 507-512 running.
  * non-anticipative scenario seller (scenna) -0.12k (SE 0.71k); scen=0 +0.06k (SE 0.56k): the deferral bug is not a lever alone.
  * seed-blind copy (v3sb) +0.73k vs copy (36 games), below v3 (+1.63k same games); M&M-style opening "0 7" cancels it.

- (19:55, Imitation) Why the pure copy concedes in the league (traced games vs d3crop, 12 each, days 12-29, per game;
  experiments/v10/sep29_mm_copy/runs/league/traced_products.txt): mostly WOOL - the copy (M&M's plan: fewer sheep, more geese)
  sells 164 units @ ~$166 vs the hybrid 176 @ ~$146, and d3crop's wool revenue is $28.2k facing the copy vs $25.1k facing the
  hybrid (+$3.1k); milk +$0.5k to d3crop; eggs +$2.7k to the copy itself. Live M&M has the same mix and compensates with
  conditional dawn lots (runs/sell/dawn_rule.txt), which our capped executor cannot place. Hybrid league standing (132 games vs
  8 agents): hyb_v3m68_ms +2.11k (73% wins), hyb_melon68 +1.80k (69%), paired +0.30k (SE 0.30k); CPP5c_v3 -2.39k vs hyb_melon68.

- (20:02, Imitation) Contested test of the every-day dawn-slot fix against REAL dawn lots (M&M-world bed, 48 games: d3crop live vs
  M&M's recorded play; runs/duel/mm_rec_d3ss): our money -1.66k, M&M -1.03k, margin -0.63k (SE 0.26k) against us. With pinned
  -1.2k and contested clones -1.28k: the current implementation loses because the smaller first hire wave costs production; dc12
  needs conditional h0 lots without the second-wave cost.

- (20:16, Imitation) M&M does not pre-sell on land days (dense all-product paths of 216 real M&M games;
  experiments/v10/sep29_mm_copy/scripts/land_days.py, runs/sell/land_days.txt). Land: day 6 (all games), 10 (all), 8 (180) / 9 (36),
  median hour 6. Same-day control (day 8, land n180 vs day-9 buyers n36): revenue by h3 / h6 / h9 $1,390 / 2,565 / 2,768 vs
  $1,455 / 2,490 / 2,728; milk 12 units sold by h8 either way (the morning collection). Its everyday early selling funds the land
  (land games start with more cash, hold wheat). So the funding fix is our everyday early selling, not a land-day cash schedule.

- (20:27, Imitation) Deposit and sale timing, M&M vs the field vs our subs (dense all-product paths of 396 real games, days 12-24;
  experiments/v10/sep29_mm_copy/scripts/deposit_hours.py, runs/sell/deposit_hours.txt): milk - M&M deposits 21% by h2 (ours 2%, ours
  at h3-8) and sells 28% at h1-2 (ours 16%; ours 44% at h21-23 vs M&M 17%); wool / strawberry: ours hold deposited units to the
  evening (35-42% sold h21-23 vs M&M 14-15%); fertilizer / wheat: M&M carries them overnight in pockets and sells 57% / 33% at the
  next dawn (h1-2). The field matches M&M on all of these.

- (20:31, Imitation) Value of M&M's committed (leader) selling over our best-response DP on the SAME farm, against our follower
  lineage (M&M-world bed, 48 games vs live d3crop; M&M's recorded farm play + our DP selling all but wheat vs M&M's recorded play):
  margin -1.43k (SE 0.36k); our own +0.56k, d3crop +1.99k. The leader gain is mostly the follower's lost revenue. Target for the
  dc12 leader seller (Day compiler).

- (21:07, Imitation) Farm-side gap = DEPOSIT timing, not collection timing (dense paths with per-hour collections, 396 real games, days
  12-24; experiments/v10/sep29_mm_copy/scripts/collect_hours.py, deposit_hours.py): mean collection hour milk M&M 8.4 vs ours 8.1,
  eggs 7.7 vs 8.0, wool 7.8 vs 8.3; but milk deposited by h2 M&M 21% vs ours 2%, and about half of ours reaches the shed only at
  h21-23 / overnight. Our workers carry the morning collections all day (deposit values from an evening-selling DP). Also: M&M's
  selling schedule on OUR stock timing (mmpolicy) was -6.2k / -6.0k, so seller changes need this farm-side timing first.

- (21:18, Imitation) Day compiler milestone m1 (dropany=6 cashsell=1 wheatcash=1 reserve=0 saleslots=3) on reactive beds (paired, its
  build; keys-off identical to Imitation's baselines): contested league (opponents with saleslots=1) d3crop +1.58k (SE 0.71k), v3
  copy +0.57k (SE 0.61k); 48 M&M worlds (v3 copy vs live d3crop) +1.33k (SE 0.59k); recorded-M&M bed d3crop +0.69k (SE 0.43k);
  uncontested league v3 copy +0.61k (SE 0.43k), d3crop -0.31k (SE 0.57k); hyb_melon68 +0.38k (SE 0.51k); 6 real worlds -1.72k
  (SE 1.72k). Positive on nearly every reactive bed, strongest where the dawn is contested; pinned +1.5k.

- (21:39, Imitation) Both-halves test of M&M's operating point on our farm (league vs the 3 strongest, 36 games, paired vs the v3 copy +
  m1; Day compiler's depcredit key + mmpolicy ported into a snapshot): + depcredit=0.5 +0.22k (SE 0.79k); + mmpolicy (M&M's fitted
  hourly selling) -4.90k (SE 0.77k); + both -5.55k (SE 0.77k). M&M's selling schedule is not transferable to our farm, with or without
  morning-deposit credit; M&M's +1.4k seller value is specific to its whole operating point.

- (21:42, Imitation) Seed robustness of BC's v3 main in the hybrid (league vs the 3 strongest, 72 games each, paired vs hyb_melon68): seed 1
  (hyb_v3m68_ms, the Local-LB candidate) +0.98k (SE 0.42k), seed 2 +0.01k (SE 0.42k), seed 3 -0.23k (SE 0.57k): the gain is seed 1's
  luck, not the recipe. hyb_v3m68_ms + m1: +0.38k (SE 0.52k) over hyb_v3m68_ms (margin +1.93k, 79% wins).

- (21:59, Imitation) The v3 main's gain in the PURE copy is seed-robust (league vs the 3 strongest, 72 games each, vs the old copy CPP5c):
  seed 1 +1.46k (SE 0.44k), seed 2 +1.26k (SE 0.51k), seed 3 +0.82k (SE 0.54k); in the hybrid it is not (seeds 2 / 3 level vs
  hyb_melon68). hyb_v3m68_ms + m1 minus cashsell -0.39k (SE 0.50k) vs +0.38k with cashsell: the league and the wide bed disagree on
  cashsell; the contested league (m1 vs m1b) is running to decide.

- (22:06, Imitation) Cashsell inside m1 (m1 vs m1b = m1 without cashsell, paired): contested league (opponents with saleslots=1, 36 games)
  d3crop +1.58k vs +0.53k, v3 copy +0.57k vs +0.14k; recorded-M&M bed (48) d3crop margin +0.69k vs +0.13k; uncontested hybrid league
  (72) +0.38k vs -0.39k. Every bed with dawn sellers / reactive opponents favours keeping cashsell (+0.4 to +1.0k); only the wide bed
  disagrees (-1.24k vs +0.03k).

- (22:15, Imitation) Options diff (experiments/v10/sep29_mm_copy/build/options_diff, scripts/options_read.py, runs/options/v3_mm216.txt):
  on M&M's own dawns (216 games, days 1-24) the v3 copy net's per-group options match M&M's labels closely (largest: strawberry
  harvest +0.4-0.7 / day; within ~1 per day everywhere). The copy's gap is trajectory divergence: on its own trajectory (48 M&M
  worlds) geese +1.1 from day 10, cows -0.4..-0.7, sheep -0.3..-0.5 vs M&M (with m1: geese +1.0-1.2, cows -0.2..-0.4). BC asked to
  attribute it (INDUMP block-swap on the copy's own dawns 8-10).

## dc11 audit: market model (market.cpp) and the executor's selling (compiler.cpp Executor::act) - (18:05, Imitation)

Source: experiments/v10/sep25_compiler_overhaul/dc11/ (v95), read in full; line numbers from that tree. Evidence from a new
seller audit: DC11_AUDIT (every hourly decision's plan, forecast and predicted inventory path; local key in
experiments/v10/sep28_top_lb_imitation/bcsrc_v88/dc11/market.cpp) on the M&M-world bed (72 of M&M's own games: "vs M&M" =
d3crop's seller facing M&M's recorded play; "mirror" = the copy CPP5c vs d3crop, both our DP), days 12-24; readers
scripts/audit_read.py (prediction error split into the opponent's deviation from forecast and our own deviation from plan;
drains cancel exactly), audit_charge.py. Numbers are milk unless stated.

| # | Assumption | Where | Evidence vs M&M | Severity / dc12 |
|---|---|---|---|---|
| M1 | The opponent's hourly sales are a fixed point forecast that does not react to ours; the DP best-responds to it. | fill_flow l.255-268 (rival_units, cum_demand), solve l.192-239, day_market l.271-297 (learned forecaster replaces forecast()) | vs M&M (truly fixed schedule): planned lots realize within ~$1 per unit of prediction (dawn -> h12-20 -0.7, -> h21-23 +0.6; h3-11 -> evening +0.1). Mirror: planned afternoon / evening lots realize $3-8 per unit BELOW prediction (dawn -> h12-20 -7.6 copy / -5.7 d3crop; h3-11 -> h12-20 -4.9 / -3.4); the opponent sells +0.5 to +1.2 units more than forecast by the evening. The model is right against a fixed schedule and wrong against a reacting opponent: the game of chicken. | HIGH. dc12: model the opponent's response (or the equilibrium), not a fixed path. |
| M2 | Our own later sales follow today's plan (the plan's predicted path includes its own later lots). | solve l.192-239 (plan -> path) | We sell 1.5-2.7 fewer units than planned by the afternoon / evening target (vs M&M -1.5 to -1.7; mirror -2.1 to -2.7): each hour re-plans and pushes lots later (time-inconsistent plans). | MEDIUM. dc12: a policy that is consistent hour to hour (or commits lots). |
| M3 | Same-hour units interleave with the opponent's at shared quotes. | solve rival_cost / gained (interleave), MarketOptions::interleave | Engine interleaves only within the same ORDER SLOT; an earlier slot fills completely first (sim.hpp process_market). Our executor puts sales in the first slots (S7); the opponent's slot is unknown to the model. Slot stats of M&M vs ours: not measured yet. | LOW-MEDIUM. |
| M4 | Drains: shops at step % 4 == 0 after the step's sales, town 1 at h0 (not fertilizer). | demand_by_step l.241-251 | Matches the engine exactly (checked). So under a fixed opponent path, waiting always gains the drains. | OK. |
| M5 | Held units are worth 0.95 x the price after 12 more hours of drains and of the opponent's forecast flow for tomorrow's h0-h11 (today's hourly profile reused), plus hold_supply (visible supply only on day 28): one lot sold behind the opponent's morning sales; our own next-day output does not compete with them. | solve l.192-205, hold_value l.133-146, fill_flow (wraps to today's profile) | Hold value vs realized next-day prices (h20-23 decisions): vs M&M 87.8 vs next h1 85.3 / next h12 75.0; mirror 97.2 vs 96.3 / 93.9. So it matches the next-dawn price and overstates noon; the implied noon price (hold / 0.95) is $17 too high vs M&M (its morning sales under-forecast). | MEDIUM. dc12: value carried units at the dawn race they can win (first after the h0 drain), with our own next-day output. |
| M6 | Night room: one uniform per-unit charge on stock left after the last market, bisected until it fits tonight's room. | choose_sales l.388-400 | Binds often: vs M&M 38% of dawn decisions (h0-2), 27-30% midday, 17% evening; mirror 16-34%; charge $11-14 per unit when binding; about half of all units our sellers sell are sold in decisions where it binds. M&M carries as much thin stock overnight (strawberry / milk / wool 19.3 vs ours 18.5-20.9 at dawn) with a FULLER shed (91 vs 73-76 units, wheat 38 vs 30). | MEDIUM-HIGH. The room itself is not smaller than M&M's; check what fills it (S2) before rebuilding. |
| M7 | Opponent uncertainty = Poisson noise per hour around the point forecast (K = 16), for this hour's units only; off when the night room binds or cash is short. | choose_sales l.430-456; Executor l.1172-1176 | No timing uncertainty (who sells first), no reaction. Off in ~25-38% of decisions (M6). | MEDIUM. dc12's uncertainty model should cover timing / response. |
| M8 | Margin objective: our sales lower the opponent's revenue on its fixed forecast units at the same hour (rival_weight). | rival_cost in solve / solve_floor | Offline (sale_score, 216 M&M games): rival=1 credits +28..45 per product-day of denial the real market path does not show (the opponent's revenue is equal under both schedules). | MEDIUM. Denial comes from ordering, not from the same-hour term. |
| M9 | Deposit values: value of a unit reaching the shed at hour h = the same DP's value with the unit available from h minus carried overnight, on a 2-hour grid, forced non-increasing. | deposit_values l.322-342, return_value l.299-320 | Inherits M1 / M5: if the DP sells in the evening, early deposits are worth nothing extra, so routes collect late (Day compiler's R2: animal service ~2 later per kind on days 12-17). | HIGH (couples routes to M1). |
| M10 | Forecast heuristic (trailing 3-day profile, blend / fit select, first_sales prior). | forecast l.58-89, first_sales l.45-56 | Replaced by BC's learned forecaster in our packages. | BC's domain. |
| M11 | rival_units are rounded per hour (lround) for the interleave / margin terms; cum_demand uses the rounded cumulative. | fill_flow l.255-268 | Small expectations (< 0.5 units per hour) vanish from the same-hour terms. | LOW. |
| S1 | Reserves: wheat / fertilizer for later pickups and tomorrow's first feed are never sold. | Executor l.1139-1157 | M&M holds more wheat at dawn (38 vs 30). | LOW. |
| S2 | Tonight's room = 100 - the plan's night pocket carry - fixed stock (reserves + later purchases - later pickups) - 2. | Executor l.1158-1170 | Feeds M6. What fills it for us vs M&M: not measured yet. | MEDIUM (measure). |
| S3 | Cash for the plan's purchases: cashsell gives a revenue bonus to sales up to the first unfunded purchase; "cover this hour" then sells the highest-PRICED stock (price now, not value vs holding) to cost x 1.05 + 20. | l.1191-1201, l.1232-1250 | M&M's early sales fund its purchases (with our seller on M&M's farm, its day-8-10 buys fail without a loan: spend 1,118 vs 1,570 on day 8). | MEDIUM: funding and selling are one decision. |
| S4 | Shed overflow before the next deposits: sell the product with the smallest loss = 0.95 x price(inventory - 30) - price now (a fixed "-30" as tomorrow's price). | l.1251-1276 | Untested. | LOW-MEDIUM: use the seller's hold value. |
| S5 | Seed / animal purchases are dropped when cash would fall below $8 (cashsell: as many seeds as cash covers). | l.1277-1300 | Linked to S3. | LOW. |
| S6 | bandsell / respsell / race / mmpolicy / peakshare / lotcapthin / flatfc are rule or probe overrides of the DP. | l.1203-1231 (+ my local keys) | All tested negative (stop list). | Drop in dc12. |
| S7 | Order slots: all sales first (sorted by revenue at stake), purchases after. | l.1301-1320 | Right for being first within an hour; M&M's slot order not measured. | Keep; measure M&M's. |

Summary: against a FIXED opponent schedule (M&M's recording) the market model predicts prices within ~$1 per unit and best-responds
correctly; against a REACTING opponent (our own DP) its later lots realize $3-8 per unit below prediction. The core fault is M1
(no response model) propagated into deposit values (M9) and the hold value (M5); M2 (time-inconsistent plans) and M6 (frequent
night-room charge) shape how the waiting shows up.

## (22:43) The M&M gap on the 6 exact worlds is the opponent's income (denial by selling hours)

- Exact worlds (our subs' real games vs M&M; the sub reproduces its real game vs M&M's recording), arm in M&M's seat vs the live sub:
  arm money - real M&M / sub money - sub's real money: v3 copy -0.8k / +4.3k; v3 copy + m1 -1.1k / +5.7k; hyb_v3m68_ms + m1 +1.1k /
  +5.6k; M&M to dawn 12 then the copy +0.05k / +3.2k; to dawn 18 +0.3k / +0.9k. Our arms earn what M&M earned; the whole margin gap
  is what our sub earns extra against them. It builds from day 12 (~+400-600 / day), in 3 of 6 worlds (+6..15k, +9..13k, +4..7k).
- Mechanism, days 12-24 (DUEL_SELL, scripts/sell_race.py; share of units sold at h21-23 / mean price):
  strawberry: M&M 8% / $135, our sub vs M&M 52% / $127; hybrid 37% / $136, our sub vs hybrid 43% / $139.
  milk: M&M 17% / $114, sub vs M&M 45% / $99; hybrid 33% / $113, sub vs hybrid 29% / $104.
  wool: M&M 14% / $106, sub vs M&M 50% / $80; hybrid 31% / $104, sub vs hybrid 13% / $106.
  M&M sells at dawn and through the day (after each drain), the market stays high-inventory all day, our follower seller waits for a
  recovery that never comes and dumps at h21-23. Our arms sell in the evening themselves and leave the day to the sub. M&M's own price
  is no better than ours: the gain is pure denial.
- Also days 6-11: the sub sells +8 melons vs our arms (a production response on days 1-3; M&M hires 30 workers on days 0-5 vs our 19).
- Consistent with the league: mmpolicy (M&M's hourly share of OUR shed stock) own -4.87k, opponent +0.03k: it never denied (our stock
  isn't in the shed in the morning: late deposits). depcredit=0.5 (morning deposit credit): own -2.06k, opponent -2.27k: early deposits
  DO deny, but cost us as much. Next: where depcredit's own loss comes from (production vs price), on the exact worlds.

## (22:49) Gap decomposition with m1: M&M's days 6-11 = carry to dawn, worth +2.0-2.4k, mostly denial

- 48 M&M worlds vs live d3crop (build_dc12m1/duel_mm = my duel tool on the m1 snapshot): M&M's recorded play until dawn X, then the
  v3 copy + m1. Margins: copy all game +0.42k; X=6 +0.81k; X=12 +2.80k; X=18 +2.83k; M&M all game (frozen) -0.01k. M&M's days 6-11
  are worth +2.0-2.4k (SE 0.7k) over the copy's; days 12-17 add nothing now (m1 closed that part). Of it, the opponent -1.7..-2.7k:
  its later milk / wool prices fall on the same units (the linear market: M&M's earlier extra units stay in the inventory).
- What M&M's days 6-11 leave at dawn 12 (same herd: cows -0.3, geese +0.2; same plants): wealth held as STOCK - wheat 45.9 vs 21.4, eggs
  6.0 vs 1.7, milk 4.0 vs 1.1, fertilizer 14.2 vs 5.9 (+2.5k at base prices; cash -2.6k) - sold on day 12 (+3.8k that day). Also +25
  strawberry waterings (survival ages) and +1-3 hires / day on days 6-11.
- Same leader pattern as the exact worlds: carry, then sell first at dawn / through the day.

## (23:26) The gap is price lead on thin products; our seller's within-day model is accurate; ~half is which days

- Exact worlds, days 12-24, real M&M vs our real subs, per-unit price lead strawberry / milk / wool: +7.3 / +15.1 / +26.0 (~$4.5k /
  game = the sub's whole extra income vs our arms). Split (scripts/price_split.py; each unit valued at its day's mean post-tick quote =
  day mix, rest = hours): day mix +3.7 / +6.3 / +9.3, hours +3.6 / +8.8 / +16.7. Hybrid + m1 in M&M's seat: -2.7 / +8.8 / -2.2 (~+$0.4k).
- 72 M&M worlds, M&M's recorded play vs live d3crop: +3.7 / +7.0 / +15.4 = day mix +2.9 / +5.4 / +10.9 + hours +0.8 / +1.7 / +4.5; copy +
  m1 vs d3crop: -2.8 / +1.9 / +1.1.
- Seller model within the day is accurate (DC11_AUDIT ported to the m1 snapshot, scripts/path_bias.py): predicted book change from
  drains + opponent to h21 vs actual, milk -4.9 / -5.1, strawberry -5.8 / -5.6, wool -3.2 / -4.0 (d3crop vs M&M: milk -3.6 / -4.3). The
  fault is not the forecast: it is the one-day objective (which days) and our evening lots (which hours).
- M&M's top-field opponents sell with M&M's hour pattern (drain_match.py: strawberry h21-23 M&M 1.5 / opp 1.8 units a day; our agents
  30-58% of volume at h21-23); intraday book is a flat sawtooth after each tick (book_profile.py). Local beds (evening sellers) make
  waiting self-consistent.

## (23:33) FIELD-WIDE: every top-10 team out-sells our subs on thin products (~$3.1k / game), mostly day mix + evening share

- 234 exact live games (Weaknesses' seat-swap list: our subs d3crop / hyb_nolp / econm6 / v17 / hyb_melon68 / cma vs ranks 1-30), real
  play, days 12-24 (sell_path + scripts/field_lead.py): the top team's per-unit price minus our sub's = day mix + hours:
  ranks 1-10 (70 games): strawberry +9.5 = +7.2 + 2.3, milk +2.7 = -1.3 + 4.0, wool +15.0 = +6.6 + 8.4 -> +$3.1k / game at our sub's volume;
  share sold at h21-23: top 13%, our sub 39%. Ranks 11-30 (164): +$1.6k / game; h21-23 17% vs 43%.
  Per team ($ / game): DECEM +5.2k, DSM +4.9k, yuto083 +3.6k, akmr +3.3k, Vadim +2.4k, Mother-Goose +2.4k, Boey +2.3k, M&M +2.0k (1 game;
  6 exact M&M worlds: +4.5k), Majkel +1.8k, Azat +0.6k. All positive.
- So the thin-market seller (which days: smoothing production pulses; which hours: after ticks instead of h21-23) is the one
  systematic gap vs the whole top field, not an M&M quirk. Day compiler is working on the hold value (holdhour / hold 0.98) with
  price_split.py / day_spread.py as reads.

## (23:47) Seller vs farm, collection lag, and the hybrid on the swap bed

- Seller-only bed (48 M&M worlds; M&M's recorded farm, our m1 seller on strawberry / egg / milk / wool): margin vs M&M's own selling
  -0.68k (SE 0.37k: own +0.55k, d3crop +1.22k); the same price lead as M&M's. Day compiler's seller keys (rival 1.5 / 2, hold 0.9,
  holdown) move neither margin nor lead. -> seller decisions are not the lever on these beds.
- Caveat: the "day mix" part of the price lead is dominated by the game-scale price fall (days 12-17 -> 18-24 ~-40%); on the M&M bed
  d3crop sells ~10 more milk and wool units late than M&M (lower late volume, not sell skill). Use margins on exact games first.
- Live (234 exact games; sell_path SELL_HERD): creation phase equal (cow parity concentration top-10 0.75 vs our subs 0.78; sheep 0.93
  vs 0.89); collection lag: cows hold 1.70 (top-10) vs 2.13 (our subs) units at dawn d12-24 -> our pulsed milk supply (daily CV 1.04
  vs 0.75) is collection timing; wool pulses are selling-through (sold CV 1.44 vs 1.17).
- Swap bed, hyb_v3m68_ms + m1 in the top team's seat vs our real sub: ranks 1-10 +1.44k (SE 0.72k) better than the real top-10 teams
  did (own -1.25k, our sub -2.69k; wins 45% -> 72%); paired vs d3crop_m68 +1.36k (SE 0.55k) top-10, +0.87k 11-30; no collapses. Against
  our own lineage the hybrid beats our subs more than the top teams do, mostly by denial (league too: hybrid vs copy rival -3.56k).

## (00:18) Weaknesses' recsell settles it: vs the field, the gap is our SELLER's denial (~$2.4-2.8k / game)

- 234 exact live games; the top team's recorded farm with our m1 seller on strawberry / egg / milk / wool vs the real game: margin
  -2.43k (ranks 1-10) / -2.78k (11-30); own +0.45k, our sub +2.9..3.3k. Same stock, days 12-24: the field sells milk 34% at h0-2 and 18%
  at h21-23 (ours 17% / 35%), strawberry 29 / 13% (ours 21 / 32%), wool 23 / 19% (15 / 27%); it takes ~$5 / unit less for its own milk
  and pushes our sub's milk / wool down ~$5-6 / unit. Our DP waits for the drains (a fixed-forecast best response) and cannot see the
  follower's reaction. Hold-value variants are closed (they raise both farms). Next seller target: dawn-first selling of overnight
  stock with low evening sales, judged on this bed (recorded farm + seller variant, exact games) and the field bed (pinned).
- The copy + m1 and the hybrid + m1 share a land-slip cascade on league seeds 504 / 505 (3 quadrants, -13..-16k); Day compiler's
  landfirst=1 fixes the cascade; league judge (all 8 agents) running.

## Sep 30 03:13 (48 h M&M imitation plan, Imitation = main; PLAN.md / GATES.md / DEVIATIONS.md)
- Gates: G1 teacher_day (216 M&M games; yield waterings; deposit-hour shares dep_*), G1-seller seller_diff (teacher-forced per-hour lots,
  201 held-out games in 32 s), G1s (same-stock reacting; same-hour deposit bug fixed in duel_mm's REC path), G2 options_diff decode-only
  (216 games ~1 min; collect / ongoing-harvest valued by units lost tonight; options checked on signed $), G3 exact (6 worlds; biased:
  open-loop recording + outcome-selected, tiny perturbations cost 0.6-1.3k) and G3-wide (Weaknesses; 278 held-out worlds, quick 60).
- Target reframed: M&M's recording vs the live package on unselected worlds +1.43k .. +2.09k (not 9.5k).
- Network: BC's copy line passes G2 (copy_or_v5d 70 / 71 on 201 held-out): root cause of the land-day / mid-game crop gaps was the
  package's non-M&M networks + decode patches; optrate fixes the per-group rate sharpening.
- Seller: M&M = a fixed clock policy on its own stock (hour after each drain; eggs at the day's end; no deep opponent use). Seller gap on
  M&M's own stock only ~0.4-1k. BC's learned seller copies per-hour decisions (56-65% same-hour overlap vs DP 39-41%) but loses money on
  G1s (-1.3..-2.1k vs DP -0.38k): not integrated, triage open.
- Compiler: D3 funding fixed by achieve + feedcost + reservenet (G1 d2-5 plantings 0.806 -> 0.953, fallback 39.9% -> 2.5%); reserve=0
  closed (-5k in games: tomorrow's feed money gone -> dawn fire-sales). D10 open: M&M's h0 hire wave collects and deposits 25% of the
  day's products at h2; our routes have no such wave and leave 14% in pockets overnight.
- Integration tree: sep29_mm_copy/src_dc12i (build_dc12i) = step 68 + seller_diff + deposit fix + Day compiler m5 / m6 patches.
- Games so far: no copy arm beats the package yet (league vs 3 strongest: copy_pure -1.68k, copy_pure_af -0.77k, copy_or_v5d_af -1.65k);
  G3-wide quick on copy_pure_afrn / copy_or_v5d_afrn running.
