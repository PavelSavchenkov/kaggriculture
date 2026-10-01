# Day compiler findings

Written only by the Day compiler session. Other sessions read it. Format: `- (time) finding. Number. Source.`

## Findings

Note: entry times after (20:05) were estimates that ran ahead of the clock (by up to ~3 h); their order is right.

- (Sep 29 17:05, Day compiler) Crew packing: on M&M's intents our hire search ran one hire more than M&M on 31% of days
  12-25. dc11 v94 `dropany=6` brings crews to M&M's size. Pinned real games +533 trimmed, p < 1e-4. REWORK.md, snapshots/v94.
- (17:40, Day compiler) On M&M's days 12-17 (its own intents and states) our routes serve animals later (h3-8: about 2 fewer
  collects / cares / feeds each, made up at h15-23). M&M sells 34.5 units at h0-2; we sell 7.4 fewer there and 20 more at
  h21-23. PROGRESS.md 17:40.
- (18:05, Day compiler) Engine market: inventory is linear. Each sale adds 1 permanently (except $1 sales), drains subtract
  fixed amounts, nothing floors or caps it. So the hour of a sale changes our own price and the ORDER of sales around the
  opponent's, not the opponent's later prices. The contest is being first after each drain; M&M sells at h1 / post-drain
  hours. Our seller's point forecast of the opponent's flow decides the order. day_solver/include/fast_game_engine/sim.hpp
  (commit_unit, town_consume).
- (18:05, Day compiler) Our seller's late selling follows from its model: deterministic drain minus a point forecast of the
  opponent. Held stock is valued behind the opponent's morning supply. Deposit values come from the same seller, so routes
  collect late. REWORK.md items 1-4.

- (18:30) dc12 seller with the opponent as a reservation-level seller (sell at post-drain hours down to the last day's mean
  inventory + 3, plus excess): duels on 6 pinned M&M worlds, copy in M&M's seat -2.8k (own -3.7k, opponent -0.9k), our sub's
  seat -2.7k. Calibrated to M&M's levels (down to mean - 2, no excess): copy seat -9.5k (opponent +6.7k), sub seat -0.7k. In the
  linear market every withheld unit is the opponent's revenue and every early unit costs us price unless sold high; M&M does
  both, a level rule does neither. The opponent's response must be learned, not assumed. sep29_dc12/PROGRESS.md 18:30.
- (18:30) Engine rules for care / feed banking and ongoing-crop waterings (BC): care + feed bank one product unit per service day
  up to the held cap; before the first production a young cow banks up to 5 milk (~$800), a sheep 5 wool (~$1,000), a goose 3 eggs
  (~$150). dc11 values care at $20 flat. An ongoing-crop watering adds yield only on a fertilized production day.
  findings/bc.md.

- (18:50) The day-3 cow fails on cash timing, not placement or purchase order: our workers collect about as much fertilizer by h8
  as M&M but carry it (Weaknesses). Funding patches that don't change that all lose: buyahead (cows -1.68 -> -3.02),
  affordanimal (-2.80), nightbuy, soft animals, animals-first ranking (no change). For our own agents the herd gap is only
  ~$0.2-0.6k / game. sep29_dc12/PROGRESS.md 18:40-18:50.
- (18:50) Scenario seller bias (Imitation): hindsight continuations overvalue waiting every hour. A non-anticipative version
  (scenopen, dc12 tree) moves lots from the evening to dawn on M&M's intents (evening 34 -> 28, dawn 27 -> 33 units / day); duels on
  6 worlds are mixed by seat, league pending. scen=0 alone: league +0.06k (Imitation).
- (18:50) The seller's target is the day's first turn (Weaknesses, 54 live games vs the top 10): the thin-book price gap sits at
  dawn (-3.3k / game); top teams sell at h0 from stock carried overnight, we sell at h1 after their lot.

- (19:00) ROOT CAUSE of the dawn gap (found by Imitation's audit, fixed in dc12 key saleslots=1): the engine takes 10 orders per
  turn, one per hire; dc11 ordered up to 10 hires at the first hour, so the seller's h0 lots were dropped (strawberry decided 1.80 /
  executed 0.18 per dawn) and sold at h1 behind the opponent. With the fix, on M&M's intents (days 12-17), our h0 thin sales go
  0.15 -> 9.4 units / day (M&M 5.2) and h1 13.3 -> 5.4 (M&M 5.7). Any analysis of our dawn / h1 selling made before this fix measured the
  bug, not the seller's choice. Judges running (league, 48 worlds, 760 + wide, pinned). sep29_dc12/PROGRESS.md 19:00.

- (19:25) saleslots=1 on pinned real games (200, recorded opponents incl. real h0 sellers): trimmed -1211 [-1540, -905], p < 1e-4,
  own -1695, rival +673. It reserves a first-hour slot for every stocked product (2.7-2.9 per dawn) and lets the seller sell at h0
  every day (9.4 thin units / day vs M&M 5.2). Weaknesses (54 live games vs the top 10): the top teams keep 1-2 h0 slots and sell at
  h0 on crowded days only (strawberry P(h0) 0.45 when the book rose >= 5 since yesterday's h0, 0.02 when it fell > 10); on flooded
  days their h0 lot gets $150 / unit vs our h1 lot $91. The delayed hires cost ~2.4 worker-hours / day (small). So the loss is the
  every-day h0 lot. Next: saleslots=2, slots only for the products the seller's own dawn decision sells (dc12 tree).

- (19:45) Method (user): isolate the compiler on M&M's recorded states before any full-game bed. Ladder: (1) one day from M&M's
  dawn with M&M's intent vs the recorded opponent; (2) 5 / 10 days; (3) reacting local opponents; (4) pinned live / Kaggle.
  Tools: teacher_day (dc12 build work/sep29_fund/build_dc12d; ~1 min per 240-day arm), env TEACHER_ORACLE=1 (true opponent flow
  replaces the forecasts), TEACHER_REASON=1 (compile report per day), TEACHER_DAYS=k; summaries sep29_dc12/tools/race_sum.py.
- (19:45) Rung 1 (240 days, M&M days 12-17): with M&M's intent our compiler reproduces M&M's production (plants created per crop,
  harvests, field units 87.6 vs 87.5, field value within $5) and ends the day +$1.2k richer with 22 fewer shed units; margin vs M&M's
  own day +90 / day (own +183, opponent +93). Waterings 54.5 vs 60.6 / day: the extra dry plants are one-day-safe by the rules.
  The true opponent flow adds +86 / day margin (SE 15): the forecast is the largest compiler-side lever vs a fixed opponent.
  DC12_LATEHARVEST changes nothing on this line (the tf forecaster reads its own History); stockcap -2 (ns).
- (19:45) Dawn slots, root cause chain: the DP decides h0 lots every day (probe DC12_SELLLOG: same lots with or without slots);
  dc11's 10-order cap dropped them silently. Freeing slots: the dawn shift itself pays (contested bed +$1.3k / game at h0 vs h1,
  Weaknesses) and is margin-neutral in one day vs a fixed opponent; the loss is production: the smaller first-hour wave makes the
  router drop planting stops (35 of 36 days with fewer plantings had dropped stops; contested: wheat -13, carrot -14, eggs -5
  harvested per game). Next: why the router drops instead of re-sequencing.
- (19:45) Rung 2 (5 days from M&M's dawn; our network decodes days 2-5): margin vs M&M -428 (SE 370) from day 10, +18 from day 15,
  +716 from day 20; from days 10 / 15 we end with ~3 fewer plants and -$340 field value.

- (20:20) FIX WITH EVIDENCE: saleslots=3 (dc12 tree work/sep29_fund/bc_dc12; builds work/sep29_fund/build_dc12d, Weaknesses'
  work/sep29_validation/builds/dc12d). The crew is routed as before; if the seller's dawn decision wants more h0 sales than the free
  first-hour slots, the router runs again with the smaller first wave, kept only if the crew is the same and no required stop is
  dropped. Pinned real games (200): +362 (SE 109), trimmed +302 [+124, +492], p 0.001, own -31 (gain from the opponent's side).
  Rung 1: h0 3.15 thin units / day (M&M 5.24), production unchanged. The every-day variants lose (see stop list). Contested and
  M&M recorded-play reads pending. Key: add "saleslots=3" to the dc11 options (inert without it; the tree reproduces the base).
- (20:20) Early-game funding (M&M's intents, days 2-4 and 10): the compiler under-executes M&M's investments because selling,
  depositing and purchasing are decided separately: M&M raises cash in the morning (dawn wheat, post-drain sales, a steady
  fertilizer deposit loop, more workers) and buys at h5-h9; our seller and router hold for the evening, the funding check fails,
  the ladder defers or trims. cashsell (the seller's Lagrangian cash constraint) fixes the seller side; the router's deposit values
  still ignore the cash need. Short beds cannot value this (investments pay off over weeks); full-game beds needed.

- (21:00) MILESTONE dc12 m1: append "dropany=6 cashsell=1 wheatcash=1 reserve=0 saleslots=3" (frozen tree work/sep29_fund/bc_dc12_m1,
  build work/sep29_fund/build_dc12_m1). Pinned real games (200): trimmed +1,495 [+866, +2,081], p < 1e-4, 70% up, own +1.7k.
  Reactive judges (league, 760, contested) running at Imitation / Weaknesses.
- (21:00) The seller's evening bias, root cause (Imitation's framing, tested): our DP is a follower (best response to a forecast of
  the other side's schedule), M&M a leader; the leader's gain is the first post-drain slot after overnight production (h1). A
  leader model whose opponent best-responds to our COMMITTED schedule fails (it front-runs every committed lot, so the model holds
  more; rung 1 -141 / day). A working leader model needs an opponent that reacts to prices, not to our plan.

- (23:05) m1 reads complete: pinned +1.5k; Weaknesses' 760 +1,097 (SE 174, all 8 seed sets positive); contested clones +0.93k;
  Imitation: contested league d3crop +1.58k, 48 M&M worlds +1.33k, league level-to-positive; wide bed -1.24k (vs our capped sub,
  a game effect: our dawn lots push that follower into the evening; 1-6% of the live field per Weaknesses' opponent mix). Clean
  patch experiments/v10/sep29_dc12/m1_saleslots3.patch (v95 + BC's local additions; identical to build_dc12_m1). State for the
  user: experiments/v10/sep29_dc12/HANDOFF.md.

- (Sep 30 00:00) SAFETY: survivalfloor=1 required in any m1 package (an all-unfunded day ran an idle plan; 1 of 234 real-world
  games collapsed to $10, -186k). No side effects: 0 of 200 pinned games change. Patch sep29_dc12/m3_survivalfloor.patch.
- (Sep 30 00:00) Pulsed sales, root cause: our farm CREATES product more evenly than M&M's (days 18-24, detrended CV milk 0.68 vs
  0.82) but COLLECTS it in batches (0.68 -> 1.16; M&M 0.82 -> 1.02): the network's collect intent takes cohorts together (28% of
  cow holders a day, 3.4 units each); the router drops none. Then the seller sells each batch through. Probe collectall=1 queued.
  The high arm-vs-d3crop output correlation on the M&M-world bed is a twin artifact (same network); vs independent farms our
  correlation equals M&M's. Tool: sep29_dc12/tools/made_phase.py (needs the duel_mm "field" column, build_dc12e / build_dc12p).
- (Sep 30 00:00) Hold value: in a linear book, a held lot is worth one lot at tomorrow's lowest expected inventory (spreading adds
  nothing: our units leave only through drains). The fixed noon reference undervalued holding: holdsteps=20 on the M&M-world bed
  margin +0.79k (SE 0.60k), arm +2.0k. A larger discount (hold=0.98) helps the follower opponent more (margin -0.9k). Key
  holdbest=1 (argmin step) under test; judge on pinned too.
- (Sep 30 01:30) Live package on Weaknesses' swap bed (our arm in the top team's seat vs our real sub, 234 exact games): vs the
  d3crop_m68 base, ranks 1-10 -1.39k (SE 0.66k; own +0.34k, our sub +1.72k). Per product (sep29_dc12/tools/swap_products.py):
  the arm moves strawberry from h21-23 (-12.8 units / game) to h0-2 (+8.8) and h12-20 (+7.5); the follower sub sells +15.2 more
  at h21-23 and earns +1.0k on strawberry, +0.7k on milk. A higher dawn share alone does not deny a follower; leaving its
  evening feeds it. Check the opponent's evening units, not only our shares, when judging "sell like M&M" keys.
- (Sep 30 01:32) Denial horizon: the market is a fixed-drain queue, so a unit we sell stays in the book until drained and lowers
  every later opponent sale (near inventory 10,000 about $2 per unit for strawberry / milk). The DP's rival term stopped at h23:
  the opponent's sales tonight and tomorrow morning were not credited to units sold today. Key rivalnight=W adds them over
  the hold horizon (dev tree work/sep29_fund/bc_dc12, build_dc12e). Live check (sep29_dc12/tools/dawn_react.py, 183 games vs
  ranks 1-30): the field's h0-2 share of its dawn stock does not fall after our bigger evenings (slope ~0), so the denial is real.
  Result on pinned live29 (75): margin -553 (SE 1,278), median -783, own -1,323, opponent -770: denial costs more than it takes.
  Closed (stop list).

- (Sep 30 02:45) M&M plan, D3 early funding: root cause = the funding ladder returned the FIRST stress-funded variant, and the
  WATER of a planting the executor skipped for seed cash failed the funding simulation, so every variant that buys M&M's h6-10
  day-3 cow and runs one seed short was rejected (fallback 34.6% of days 2-5 on M&M's own intent, copy_pure keys). Key achieve=1:
  every variant is evaluated with skipped plantings counted; the one achieving most is kept (animals, then seeds, then drops).
  G1 d2-5 (60 games): fallback 34.6% -> 1.7%, cows kept, 14 of 15 checks; plantings 0.80 -> 0.83 (0.88 with hourdisc=0.99). Needs
  reserve=0 (the next-dawn reserve rejects the dawn-wheat variants that fund M&M's day 3). Day 3 remains.
- (Sep 30 02:40) M&M's selling is a tick seller (same-stock bed, days 12-24): about one unit per product in the hour after each shop
  drain, a third of the drained room, more with stock and when the book is below the day's mean; its high dawn price comes from
  evening restraint (the book drains overnight). Per unit over the day our DP prices equal M&M's; the margin gap is the reacting
  sub's revenue (denial), which a lone-seller DP does not choose.

- (Sep 30 03:00) Funding, second root cause: the router's feed-wheat accounting. Feeding harvested wheat forfeits its deposit value
  but wheat picked up at the shed was free, so routes fed early with bought wheat and sold the harvest (day 3: $141 of wheat bought;
  M&M feeds after its harvests, buys none). Key feedcost=1. With achieve reserve=0: G1 d2-5 (216) plantings 0.806 -> 0.954, fallback
  39.9% -> 2.3%, cows kept; no G1 loss on d6-24. BUT in games (Imitation, 6 exact worlds) -2.30k whole, -1.47k from day 12, the
  sub gains: ablation running. G1 one-day passes are not enough; G3 decides.
- (Sep 30 03:05) G1s bug (Imitation): our seller could not sell this hour's deposits. Fixed: our DP seller on M&M's stock -0.38k
  (SE 0.56k) vs M&M's own selling. I1 learned seller (sampled) -3.08k (end-game holds; wrong-moment lots). Tick rules -5k.
- (Sep 30 03:15) Deposit values by hour (router): milk h0 27 -> h22 7 $/unit; early deposits are worth more under our DP, so mid-day
  collection is value-driven; M&M collects at h21-23 (D10). Eggs: our DP sells eggs in ~85% of hours (flat book, tieall sells on
  ties), M&M in 2-16%, mostly the evening.

- (Sep 30 03:10-03:45) Next-dawn reserve: reserve=0 costs ~5k in games (Imitation's ablation); the reserve covers tomorrow's feed
  (~$900 mid-game). reservenet=1 (tomorrow's feed net of our own wheat incl. tomorrow's harvest, no slack) matches reserve=0 on G1
  d2-5 (0.953 / 2.5%) but also lost on the 6 exact worlds (-2.34k vs copy_pure; per-world +-5k). Cashsell fire-sales are NOT the
  cause (live cashsell only on days 0-11, 6-12 thin units per game); the loss is a product-mix shift after day 12 (our milk / eggs up,
  wool / strawberry / tomato down, the sub gains those books). G3-wide pending. Keys built for it: nofire, achieve=2 (search only on
  seed shortfalls), achieve=3 (variants by value: a seed dollar = 4 animal dollars).
- (Sep 30 03:45) D10 deposit timing follows the seller: same collection hours as M&M on M&M's states, but M&M drops at h2 while our
  routes carry products along; our DP values an early drop at about one return trip. Deeper search does not change it.
- Seller imitation, all on G1s (fixed bed, vs M&M's own selling; our DP -0.38k): I1 sampled -2.65..-3.08k, threshold -1.31..-1.53k,
  day windows -1.51..-2.13k, model hours + DP lots -1.10 / -1.16k; tick rules -5k; hourdisc -1.15k; rival=2 -0.24k (noise); I2 fit
  of the DP's parameters to M&M's hours: no setting close (eggs). None beats the DP yet; BC's v4t pending.

- (Sep 30 04:10) Line B candidate: achieve=1 feedcost=1 (league: copy -1.68k -> -0.92k vs the package); reservenet dropped (league
  -2.03k): when the old reserve fails the ladder trims new entities, so mid-game it cut some network asks by accident. I1 v5t (BC,
  level inputs) on G1s -0.56k vs M&M's own selling = level with our DP (-0.38k), denying the sub 1.0k for 1.6k of own revenue.
  Day 0-1 (Imitation's network drift): M&M's day-0 wheat sales are a buy-and-resell round trip; day 1 M&M hires 3 and sells 5
  fertilizer (collect-drop-sell) vs our 1 hire / 3 sold; plantings equal.

- (Sep 30 05:00) Deposit values from the learned seller (a rollout of its threshold decode per deposit hour): closed. The model
  fires on inputs a route plan does not have (pockets, units sold today, money; live milk h3 pocket 8 -> P(sell) 0.97, the rollout's
  states 0.08-0.30), and on M&M's own stock timeline the learned seller is level with our DP (G1s -0.56k vs -0.38k), so the best case
  is the DP. The seller is not the gap; the stock timeline (M&M's h2 drops, evening collection) is.
- (Sep 30 05:05) Gap split, seller vs farm (tools/band_rev.py on G1s, 24 worlds, paired vs rec_s): our DP selling M&M's own farm
  earns +297 over M&M's selling, the sub +675 (net -378). On M&M's stock our DP still sells strawberry / milk / wool later (dawn band
  units 28 / 24 / 13 vs M&M 55 / 36 / 19; evening 47 / 43 / 19 vs 28 / 27 / 11) and dumps eggs at dawn (117 vs 39; M&M 117 in the
  evening). With G3-wide (copy in M&M's seat: own -313, sub +1,442): seller hour pattern ~0.4k (own +0.3k, sub +0.7k), farm ~1.4k
  (own -0.6k, sub +0.8k; melon / tomato / sheep asks per Weaknesses' production values).
- (Sep 30 05:32) Line B (achieve=1 feedcost=1) on the package fails the 760 (-439, SE 205; afrn -718). Two mechanisms, traced on
  40 F2R games (runs/af760): achieve=1 ranks funded variants by animal dollars first, so on day 3 (4 strawberries + 1 cow asked,
  unfundable) it keeps the cow and skips ~2.8 strawberry seeds (10 / 10 games vs the M&M clone); the first wave shrinks (d12-17
  -4.2 units) and the opponent's thin prices rise (afrn's channel, without a reserve change). feedcost=1 cuts day-2 feed-wheat buys
  and raises d6-11 unfed animal-days +19%; valuing feeds by banked product (feedvalue=1) does not fix it. The copy line carries
  both keys (copy_or_v5d_af): suggested arm achieve=3 without feedcost.
- (Sep 30 05:46) CORRECTION to 05:05: on all 216 M&M worlds (G1s, build_dc12e, runs/duel/mm_rec216 / base216 / so216) the package
  seller selling M&M's own farm is level with M&M's selling: -102 (SE 180; own +1,391, sub +1,494). scenopen (non-anticipative
  scenario seller) moves the hour pattern toward M&M's (dawn strawberry / milk / wool 47 / 38 / 18 vs 33 / 27 / 14, M&M 55 / 42 / 20)
  but gains only +164 (SE 171). The seller's share of the G3-wide gap is ~0.1k; the farm carries ~1.6k. Seller work stopped.
- (Sep 30 05:58) Rung 2 (5-day continuations on M&M's asks, labels translated onto our groups; teacher_day TEACHER_LABELS=2):
  the compiler reproduces M&M's multi-day value from day 12 (package +163 / +928, copy keys -89 / +614 at D+5, 60 games). Days
  6-10 lose ~5 plantings: day-10 Q4 ($4,000) waits for melon cash (M&M harvests ~20 melon units at h4-6 and buys at h8-12; our
  routes harvest one plant every 2-3 h: land h11-21 in continuations, h12 from M&M's own state). Router cause: melons have flat
  deposit values; the land credit needs a harvest-then-shed combined move. Small in real games (our networks plant Q4 on day 11).
- (Sep 30 06:12) Rung 2 on compiler keys (12 days from day 1 on M&M's asks): the package's funding keys (reserve=0 + cashsell +
  wheatcash + survivalfloor) plant M&M's strawberries and melons (-0.52 / -0.08; only wheat -4.7 short); the copy's keys
  (achieve=1 feedcost=1) lose -1.9 strawberries and -1.2 melons, the melons in days 1-5 (achieve's animals-first ranking skips a
  seed on cash-short days). The copy's live melon deficit is its ask (2 + 1, pre = final); pair BC's ask fix with reserve=0 funding.
- (Sep 30 09:21) Dawn slots (task d): the executor gives the plan's orders priority (`slots = 10 - plan orders`), so the h0 hire wave
  cuts the dawn sells the seller wants on 77-89% of dawns (copy + v5t 26.8 -> 2.2 units, copy + DP 24.5 -> 2.0, package saleslots=3
  18.5 -> 6.9; h1 all placed). M&M: sells in slot 0 (~15 units), 8.5 hires at h0, 3.5 at h1. Fix at compile time (unit actions are
  index / position bound): saleslots=4 (m12 patch) = the first wave always leaves the dawn sells their slots, sized by the seller that
  sells (the learned model when on). League 501: v5t + ss4 vs v5t +959 (SE 263); DP + ss2 vs DP -1,302 (SE 481).
- (Sep 30 10:08) Astra pass 1 triage (Day compiler area): astra-001 taken and confirmed (duel_mm REC path appends our sells after
  M&M's hires: G1s seller reads biased against ours at dawn); astra-002 taken (executor credits cut sells to cash; probe running);
  astra-004 done (frozen builds with SHA256 manifests). Voided (network overwrite 09:06-10:03): my copy-arm league / probe / own60
  reads from that window; package reads stand.
- (Sep 30 10:58) h0 slots with slotcash (build_dc12v7, 12 games vs d3crop, days 10+): ss4 + slotcash places every h0 sell it keeps
  (copy + v5t 22.3 units, thin 8.8; package 13.8 / 4.6), ss4 alone 21.0 / thin 7.0 of 10.0; slotcashviol 0. Compile-time h0 estimate
  vs executor (240 dawns): the executor also sells wheat (88 / 71 dawns) and fertilizer (40 / 20) the estimate keeps; harmless with
  slotcash (the fit is at execution). For the DP seller dawn slots do not pay (package ss4 +5; Weaknesses' swap ss2 on the copy
  REJECT -450, SE 374).
- (Sep 30 10:58) Land-day seed cuts (copy keys, quick-60 = own60 worlds, copy in M&M's seat): NOT a live-vs-simulation gap (live cash
  >= the funding simulation's every hour); the accepted day-6 plan already cuts $300-1,000 of seeds in the simulation, and at every
  live cut the shed holds no sellable stock (free value $0). Day-6 flow, 60 games (tools/dayflow.py, runs/d6flow): copy dawn $502 vs
  M&M $393, yet strawberries planted 11.9 vs 14.1; next dawn $374 vs $73 (21 short games: $355 vs $71, seeds -$419). The copy's
  money arrives after the evening seed cuts: its workers carry collected fertilizer in pockets all day (1.4-3.9 units, deposited /
  sold at night; M&M deposits by h11 and sells 5.4 units h12-20) and keep 1.4 wool in pockets (runs/d6sell, DUEL_SELL_ALL). ~$300-550
  of the day's cash is in pockets when the seeds are cut. Test: shadowskip=1 (cashshadow variants with the horizon at the funding
  simulation's first seed cut) vs cashshadow=1 vs the copy, 21 short worlds (build_dc12v10, runs/skipshadow).
- (Sep 30 11:12) CORRECTION (astra pass 10:00 item 4): my executor probes before build_dc12v14 (DC12_SLOTLOG, SLOTDET, CASHCUT, SELLLOG,
  ROOMLOG) also printed the funding simulations' executor runs, so the h0 cut % (77-89%), the 177 cash-cut hours and the slotest /
  slotdet table mix simulated and live dawns. The mechanisms stand (source-level), the counts are retracted; live-only rerun:
  runs/slotlive (build_dc12v14).
- (Sep 30 11:12) Land-day fix: the binding limit on the copy's day 6 is the next-dawn reserve (tomorrow's feed x 1.2 + $30, ~$500 at
  day end), then late cash. From M&M's dawn 6 (runs/recd6, 60 worlds, same state for every arm), crops planted: copy 15.75, reserve=0
  16.42, + shadowskip=2 16.50, shadowskip=2 alone 15.68, M&M 18.63; next dawn $485 / $314 / $328 vs M&M $73. Own states (21 short
  worlds): reserveuntil=6 (reserve kept on days 1-5) 17.65, + shadowskip=2 18.80, copy 15.45, M&M 19.9; reserve=0 on all days spends
  days 1-5 cash (dawn-6 $285 vs $360) and gains less (17.85 with shadowskip). Full-60 own-state read to day 11 running (runs/own60ru).
  One plan inspected (DC11_PLAN): all deposits end by h9, later fertilizer rides to the night while seeds are bought h10-21.
- (Sep 30 11:20) Land days from M&M's own dawn (runs/recland, DUEL_REC="" DUEL_REC_UNTIL=D; plantings vs M&M's): day 6 copy 15.61, fix
  (reserveuntil=6 survivalfloor=1 shadowskip=2) 16.40, package 17.12, M&M 18.65; day 8 copy asks 12.1 / plants 8.97 (drops 2.1, cuts
  1.1), fix 10.21 (drops 0.17; land h5.8 -> h11.5), package asks 9.66 / plants 9.45, M&M 15.48; day 10 copy 20.5 (ask 20.7), package
  asks 12.3 / plants 12.2 (tomatoes 0.3 vs 3.8), M&M 19.46. The package's day-8 / 10 gap is its network's ask (BC); the copy's day-8
  gap is ask -3.3 + execution -3.2.
- (Sep 30 11:20) L2 seller: the margin term's opponent units are lround'ed per hour (market.cpp fill_flow: rival_units, future_rival)
  while the market path keeps fractions, so expected flows under 0.5 units / h vanish from the opponent-revenue term. G1s (8 worlds,
  d11-28, opponent units / world, doubles / lround / cumulative / actual): h12-20 strawberry 49 / 42 / 49 / 72, egg 21.5 / 8.2 / 21.1 /
  36.8, milk 41 / 33 / 40 / 48, wool 10.6 / 6.8 / 11.6 / 19.0. Fix rivalfrac=1 (cumulative rounding, totals kept; build_l2g for the
  G1s bed, dev tree too); G1s screen runs/g1s_rfrac (48 worlds, + / - rivalnight 0.5). The forecast is also ~30-40% low in the
  afternoon and high at dawn on this bed (the opponent sub's dawn sells are slot-capped).
- (Sep 30 11:25) rivalfrac=1 on the G1s bed (48 worlds, fixed order, build_l2g; baselines Imitation's mm_g1s_*): vs the package seller
  +801 (SE 241; own -112, opponent -913) PROMOTE; vs rivalnight 0.5 +60 (SE 290); vs M&M's own lots +355 (SE 347); rivalfrac +
  rivalnight 0.5 vs rivalnight 0.5 +76 (SE 167, REJECT). The bug fix reaches the weight change's gain; they do not add. Hour bands
  barely move (days 12-27); eggs are no gap (our egg revenue 8,262 vs M&M's lots 8,137 / world).
- (Sep 30 11:30) Land-day keys, copy's own states (58 quick-60 worlds to day 11, build_dc12v14 / v15; crops planted d6 / d8 / d10, M&M
  18.57 / 14.60 / 19.12): copy 16.52 / 12.36 / 15.83; reserveuntil=6 + shadowskip=2 17.48 / 12.55 / 14.86; reserveland=1 16.81 / 12.50 /
  14.91; reserveland=1 + shadowskip=2 17.48 / 12.52 / 15.47 (best: day-6 strawberries 11.72 -> 12.50, seeds cut 1.55 -> 0.55); + landcash=10
  17.48 / 12.45 / 15.07; landcash=10 alone 16.52 / 12.29 / 15.71. Day 10 loses because less cash is left for the Q4 (bought h11.9 vs
  h10.9). Dawns below $20 on days 1-11: 77 (copy) vs 78; min $11. build_dc12v15 reproduces Imitation's copy base (plant60_l3_base) 3 / 3.
- (Sep 30 11:45) rivalfrac in full games: Improve Agent's league (package m3cma + rivalfrac, 38 games) -0.85k (SE 0.58k; opponent
  +0.45k), Imitation's G3 218 pk_rn05 (rivalnight 0.5, same G1s gain) -0.46k (SE 0.45k; opponent +0.78k): the G1s gain (+0.8k, M&M's
  farm fixed) does not carry to full games. One path found on the copy's own states (60 worlds, crops d6 / d8 / d10, day-8 seeds cut):
  land keys 17.52 / 12.63 (0.32) / 15.50; + rivalfrac 17.27 / 12.58 (0.67) / 15.93; + saleslots=4 slotcash=1 17.43 / 12.60 (0.32) / 15.48;
  all three 17.22 / 11.98 (1.28) / 15.78: the seller's later / denial lots delay land-day seed cash (seller -> cash -> farm). A seller
  change needs a full-game (or day-limited continuation) Stage-1 check, not only the G1s bed. rivalfrac stays in the builds, off.
- (Sep 30 11:45) L1 hand-over: m13 patch on Imitation's src_dc12i (experiments/v10/sep29_dc12/m13_sep30_landkeys_rivalfrac_on_src_dc12i.patch:
  shadowskip, reserveuntil / reserveland, rivalfrac; all off by default), build work/sep29_fund/build_m13f; copy key line `reserveland=1
  survivalfloor=1 shadowskip=2`. Identities: keys off = plant60_l3_base 3 / 3; keys on = v15 own60 rows 3 / 3. Own states (60): crops d6 /
  d8 / d10 16.62 / 12.48 / 15.87 -> 17.52 / 12.63 / 15.50 (M&M 18.63 / 14.72 / 19.2). Land-day compile time +5-18% (day 10 max 914 -> 1,054
  ms at load ~20), far under the bridge deadline. Astra 10:30 triage: 014 taken (trace the scenario vs deterministic branch before any
  rivalfrac full-game test), 015 done (timing above), 016 BC's area.
- (Sep 30 11:50) astra-014 (DC12_BRANCHLOG, build_l2h, 12 G1s worlds): with scen=16, 77-79% of our thin product-hours with stock are set
  by the scenario solves (integer Poisson paths: per-hour and cumulative rounding coincide, the expected flow is kept), 21-23% by the
  deterministic DP (tonight's room charge > 0), where alone rivalfrac acts. The live executor also switches scenarios off while the day's
  purchases are uncovered (land-day hours). rivalfrac and rivalnight 0.5 are different mechanisms (rivalnight changes every scenario solve).
- (Sep 30 12:05) Day-10 Q4 cash, copy's own states (runs/own60ru/cp_af, 60 worlds; M&M = its own recorded day 10): melons harvested
  h4-7 19.8 (M&M 23.2); sold h4-7 / h8-11 / h12-15 / h16-23 1.6 / 14.7 / 3.5 / 12.0 (M&M 4.9 / 19.0 / 5.6 / 0.6): the copy holds ~12
  melons ($3k) into the evening while its Q4 is bought at h10.9 (h11.9 with the land keys, $208 less dawn cash). No shop drains melons
  (town 1 / day), so holding them buys nothing for a lone seller; M&M sells them by h15. Cause = pockets again: the copy's workers
  carry 10-12 melons from h10 to h22 (M&M's pockets fall to 1-2 by h13); routes value a deposit the same at any hour (gain 0 unless
  the night room overflows or a cash / shadow credit is set), and the general early-deposit credit (depcredit) was closed (760 -0.8k).
- (Sep 30 12:35) astra-019 (DC12_DECODECAP, runs/deccap, from M&M's dawns, 30 worlds): the site cap barely moves the ask (package d8
  8.93 -> 8.77, d10 12.23 -> 12.03; copy d8 13.03 -> 12.40, d10 20.93 -> 20.60). Root cause of the package's late quadrant planting: its
  network asks land on day 10 in 33% of worlds, `.decode v219 10 0 0 0 0` forces buy_land on day 10 (n = 0 extra tomatoes), and the
  crop heads never see the forced land (no .landcond) -> the Q4 is bought (100%, h9.1) but only 12.0 crops asked (tomatoes 0.29 vs M&M
  3.8; M&M 19.46). Fix options (BC): v219 n > 0 with M&M's day-10 mix, a re-decode at a higher crop quantile when v219 fires, or landin.
- (Sep 30 12:35) Closed: cashrival=1 (no denial terms while purchases are uncovered: day-8 cuts 0.67 with and without it; land keys +
  cashrival = land keys), shadowskip=3 (+0.22 crops from M&M's dawn 6, ~0 on own states).
- (Sep 30 12:40) Deposit timing is NOT the denial driver (runs/pockfull, 30 worlds, full games, copy in M&M's seat vs the package, days
  12-27, tools/deposit_timing.py): share of each day's output in the shed by h5 / h11 / h17, ours vs M&M: strawberry 0/2/12% vs 0/3/24%,
  egg 14/29/37% vs 16/17/22%, milk 17/35/43% vs 26/32/42%, wool 14/36/51% vs 24/30/40% (both farms mostly deposit at night and sell the
  next day). Sold h0-2 / h3-11 / h12-20 / h21-23: strawberries 37/25/66/64 vs 50/38/75/23, milk 19/30/42/45 vs 47/32/40/25, wool
  10/25/36/32 vs 24/37/28/18: our seller carries the dawn stock to the evening. Pockets matter on cash-bound land days only.
- (Sep 30 14:25) Faithful M&M-rule seller (mmfaith, Imitation's request; table fit on 594 M&M games: P(sell | hour, price quintile,
  stock bucket) + lot | sell, deterministic hash draws; G1s 48 worlds, fixed order): v1 (eligibility = old shed > 0, unit lots) -15.0k vs
  M&M's own lots (volumes -5-8%, hour shapes off; it dropped every hour M&M sold arriving stock: strawberries h12-20 83 vs 42 units /
  game); v2 (avail = shed + this hour's deposits, share lots, build_l2j) -5.3k (own -3.8k, opponent +1.5k) with volumes and hour
  bands matching M&M (strawberry 207.5 vs 209, wool 114.3 vs 114.2), eggs h21-23 101 vs 131. The own loss is price per unit (wool
  $128.9 vs $138.6, milk $106.4 vs $110.1, strawberry $129.2 vs $131.7), although open-loop on M&M's rows the model sells at M&M's
  prices (finer price / momentum features < $1). Independent draws with M&M's marginal frequencies sell into the wrong books
  closed-loop. The DP seller is within -0.42k of M&M's lots on the same bed. Line closed (stop list).
- (Sep 30 14:28) Day 10 is not a compiler bottleneck (runs/t10, teacher_day, 60 quick-60 worlds, M&M's own day-10 intent from M&M's
  dawn 10): package keys plant 18.10 of M&M's 19.20 crops (Q4 at h12.0 vs M&M ~h9; landcash=10 -> h11.2, 18.05), copy keys 17.58; next-
  dawn value -180 vs M&M (we sell $1.5k more, hold 33 fewer units, spend $0.8k less). The day-10 gaps are the asks (package 12 via v219
  without crops, copy 16.2 on own states); the copy's land keys cost ~0.4 day-10 crops through less dawn cash.
- (Sep 30 14:45) BUG (every line): compiler.cpp funded() lets a partial hire wave pass. For each planned hire it reads the first
  hire order's fill (1 = want) unless all hires filled, so a wave the dawn cash covers only partly (worst quick-60 day-6 world: $20
  dawn, 9 hires planned, 6 filled) passes as funded; the unhired workers' routes (most of the day's plantings) vanish in the simulation
  and live, and achieve reports "skip 0" (2 of 16 planted). Fix hirecheck=1 (m14 patch on src_dc12i; repair = the crew the cash
  covered). Teacher replays (M&M's intent from M&M's dawn, 60 worlds), crops d6 / d10: package 16.17 / 18.10 -> hirecheck 16.55 / 17.53
  -> + latehire=1 17.48 / 17.77; copy + land keys 17.05 / 17.58 -> 17.23 / 17.70 -> 18.03 / 16.95 (M&M 18.63 / 19.20); day 8 unchanged.
  Own states + full-game count running (runs/own60hc, runs/hirefull).
- (Sep 30 14:59) hirecheck = protective, zero-cost: G3 first 24 (package + fc2 + hirecheck=1 latehire=1 vs fc2, build_m14f; identity 3 / 3)
  +36 (SE 311), 22 / 24 games identical; own states d6 / d8 / d10 within +/-0.2 crops; 1-4 short waves in 30 own full games. Crew
  invariant (DC12_CREWCHECK, teacher replays from M&M's dawn): without the fix 9-12 / 60 worlds per land day plan 262-461 unit actions
  for workers that never exist; with it 0. Recommended in every line (m14).
- (Sep 30 15:05) D5 closed (runs/t1824, teacher replays d18-24, M&M's intent from M&M's dawn, 60 worlds, package keys): our next-dawn cash +
  shed beats M&M's every day (+67..+400; see the 15:30 correction for the full state); plantings -0.3..-1.0 / day (late crops), harvests / feeds / waters within 1-5%, weeds +0.04..+0.54;
  drops 0.18-0.95 / day, mostly optional fertilizer collections; crew invariant 0 / 420. Compiler vs M&M's intents overall: d6 +194
  (land keys + hirecheck), d8 +26, d10 -180, d18-24 +67..+400: the remaining gaps are the asks and the seller hour / denial.
- (Sep 30 15:15) (b) end-of-day carry: teacher_day value_next prices stock at the unit quote; the new column value_next_m (stock sold into
  the next dawn's book, dc11::sale_value; build_dc12v25) gives day 10 ours - M&M -153 (value_next -180): not a valuation artifact, but
  small next to our +$1,473 same-day revenue and confounded by M&M's +$811 spend on items value_next does not count. Low priority.
- (Sep 30 15:20) CORRECTION (astra-031): teacher_day value_next / value_next_m are cash + shed only (they omit stored crop yield, animal-held
  product and standing crops' future output); "the compiler executes M&M's intents at least as well as M&M" is withdrawn. Full-state
  pairs (tools/teacher_state.py; ours - M&M per world, paired SE), cash + shed + stored yield: d6 package -28 (89), + hirecheck latehire
  +140 (45), copy + land keys + both +25 (14); d8 +20 (10); d10 package -166 (118), + both -6 (146); d18-22 +369..+598 (53-90); d23 -63
  (130; M&M leaves 4.0 more units on its animals, SE 0.7); d24 +171 (111). Irreversible missed output ranked: day-6 plantings (package
  -2.5 -> -1.1 with hirecheck latehire, mostly strawberries, ~$0.6-1.5k / game at ~$600 each), day-10 plantings (-1.1..-2.2, wheat /
  tomato), late plantings (-0.2..-0.7 / day d18-24), weeds (+0..+0.5 / day), animal-held product (d20 -2.9, d23 -4.0 units).
- (Sep 30 15:24) Package + achieve=3 shadowskip=2 (on top of hirecheck latehire) closed: from M&M's dawn 6 the missed plantings go -1.1 ->
  -0.6, but on own states (runs/own60a3, d2-11) day-6 seed cuts double (1.02 -> 2.07; planted 15.43 -> 15.02), a day-2 cut appears and
  dawn cash falls on days 7-11 (the line-B pattern). The package's own-state land-day gaps are its asks (asked / planted / M&M: d6
  16.45 / 15.43 / 18.63, d8 6.12 / 6.12 / 14.72, d10 8.65 / 8.65 / 19.20): BC's v219 / decode fix is the lever.
- (Sep 30 15:15) Kaggle time (runs/deadline, 24 games vs d3crop, FULL_BUDGET=2.5, FULL_DEADLINE on / off, build_m14bf): package / + fc2 / +
  fc2 + hirecheck latehire: 0 dawns past the bridge deadline, worst step 4.55 / 5.08 / 4.43 s, overage min 31.2 / 31.0 / 31.7 s, 0 / 24
  games changed by the deadline.
- (Sep 30 15:24) Lead (package + fc2) on the two beds (tools/bedpair.py, existing .days rows): SWAP 98 worlds own -376, opp -1,434, margin
  +1,058: fc2 moves our strawberries from h21-23 to h0-2 (50 vs 36 at dawn) and milk to the morning; we give $1.6 / unit, the opponent
  loses $2.7 (strawberry -922, milk -380): a denial trade, positive margin. G3 56 worlds own +1,933, opp +526: milk / wool prices up for
  both (ours $108.4 vs $103.5, wool $121.9 vs $116.0) as the opponent moves milk to h21-23: a shared price-level gain. Forecast audit of
  both beds running (runs/fc2audit).
- (Sep 30 15:31) fc2 forecast audit (runs/fc2audit, 16 swap + 16 G3 worlds, identity 64 / 64 with the judge rows; DC11_AUDIT rival units
  at h0 vs the opponent's actual sales, d11-28): fc2 mainly lowers the forecast of the opponent's dawn selling (strawberry h0-2 60 -> 44
  swap, 55 -> 43 G3; milk 42 -> 30 G3), so our DP sells more at dawn; the live opponent (our sub) then sells less at dawn (swap 37 -> 24)
  and later: on swap both prices fall, the opponent's more (denial, margin +1.06k); on G3 the opponent moves milk to h21-23 (52 -> 80,
  fc2 forecasts 78) and milk / wool prices rise for both (own +1.9k). Both forecasters over-forecast dawn by 30-60% and under-forecast
  the afternoon by 20-45%. The swap own loss is the cost of denial, not a seller error. fc3 / fc3n audit to follow.
- (Sep 30 15:39) Forecast bias vs REAL opponents (runs/livefc: our 75 live d3crop games, agent replayed vs the recorded opponent, DC11_AUDIT
  h0 forecast vs the trace's actual sales, d11-28, tools/livefc_read.py): the forecaster under-predicts real opponents' volume (package:
  milk 129 / 170, wool 76 / 101, strawberry 191 / 218, egg 105 / 127 per game vs rank 31+; milk / wool h3-20 at 55-70%); dawn is NOT
  over-forecast (top-10 milk dawn 51 / 63). The dawn over-forecast on G1s / G3 / swap comes from our subs' slot-capped dawns, so
  calibrations fitted on those beds move the wrong way for live. fc2 improves strawberry / egg totals, not milk / wool.
- (Sep 30 15:46) CORRECTION to the live forecast read: DC11_AUDIT rows exist only for products we hold that day, and the reader summed actual
  sales over all days, inflating the shortfall. With DC12_RIVALFC doubles (runs/livefc2, dev build_dc12v26, identity 75 / 75), package
  forecaster vs the trace (per game, d11-28; doubles / per-hour lround / actual): top-30 opponents strawberry 214 / 195 / 218, egg 205 /
  187 / 216, milk 155 / 137 / 160, wool 78 / 66 / 86; rank 31+ milk 177 / 154 / 171, wool 110 / 94 / 101: the model is within -2..-10%,
  the per-hour rounding drops another 9-15%. rivalfrac on these pinned replays: margin -841 (SE 596; top-30 -257, 31+ -1,079). The same
  reader artifact affects the bed audits (fc2audit), where it only strengthens the bed dawn over-forecast. Running: pkq_fc2 +/- rivalfrac,
  fc3v / fc3nv doubles audits (runs/livefc3).
- (Sep 30 16:11) Clean forecast check (astra-021; tools_dc11/forecast_trace = exact recorded replays of our 75 live games, a fresh agent's
  dawn forecast d11-28 vs the opponent's actual hourly sales; runs/fctrace, tools/fctrace_read.py): every forecaster is ~unbiased vs real
  opponents on exact states (package rank 31+: strawberry 216 / 218, milk 179 / 171, wool 108 / 101; top 30 strawberry 213 / 218,
  milk 157 / 160; bands close); the earlier live shortfall was state divergence + the audit-reader artifact (withdrawn). Per-hour lround
  drops 7-13% (rivalfrac restores it). Hourly MAE: fc2 / fc3v / fc3nv 5-12% better than big2. CORRECTION (astra 15:15): "equal to each
  other" is not established (top-30 fc3nv - fc2 MAE +0.28 per day, SE 0.06, episode-paired; most top-30 episodes are in some head's
  training data; on the 5 unused by all heads +0.14, SE 0.07), and totals are not universally within 5-7% (top-30 wool: package -3%,
  fc2 -11%, fc3v -8%, fc3nv -10%).
- (Sep 30 16:50) Crew invariant on the final bundle (astra-030; runs/crewbundle, teacher replays from M&M's dawn, the 60 crew worlds, days 6
  and 10, build_dc12v24 with DC12_CREWCHECK / DC12_HIRELOG): packages/m3fc2ens_hc model (m3 keys + hirecheck=1 latehire=1, fc2 x3) has 0
  violating hours, 0 short waves, no parse abort. The running judge arm pkq_fc2ens (sidecar e098ddc8, no hirecheck) has 163 / 311
  violating hours (400 / 353 lost unit actions), 8 / 15 short-wave days. So the fc2ens / fc3vens judge results measure "no partial-hire
  fix" stacks. A repaired arm needs a new frozen dir.
- (Sep 30 ~17:40) Forecaster confirmation. Set A = 25 live pkm1 games after 07:30 UTC. Label: training-excluded, previously scored.
  Confirmation use: arms pkg (big2) / fc2 / fc2ens / fc3vens, metric hourly MAE + band bias (runs/fcconf, exact forecast_trace, build_dc12v27,
  tools/fcconf_read.py); pinned margin pkg vs pkq_fc2ens (runs/fcconfpin, build_m14bf, tools/pinpair.py). Days 11-28, 4 products:
  - Learned heads vs big2: hourly MAE -0.030 (SE 0.006) = -6.6%; resolved in ranks 11+; top 10 -0.010..-0.015 (SE 0.02, n 5).
  - Among the learned heads, equal: fc2ens - fc2 -0.000 (SE 0.001), fc3vens - fc2ens -0.000 (SE 0.001).
  - big2 under-forecasts h12-20 by 42 of 235 units / game and over-forecasts h21-23 by 32 of 169. The learned heads fix h12-20;
    fc3vens also halves h21-23. The fc2 heads under-forecast dawn: -12..-18 of 175 overall, -23..-37 of 267 vs top 10.
  - Pinned money read (first-order; the pkg arm reproduces the recording, own -3, SE 11): fc2ens - pkg median +1.9k, trimmed mean +2.3k,
    17 / 25 positive. One pinned collapse (115782305, +108k) is an artifact.
- (Sep 30 ~17:40) Divergence triage C1 / C2 / C3b (MAP.md, user priority 1):
  - Counting artifact: duel_mm's DUEL_OPS count_ops skipped every h23 step (the night update clears the flags), so our waters / cares were
    undercounted vs M&M's recording. Fix: build_m13o (h23 water / feed / care from actions + unit tiles), then build_m13x (exact: the h23
    unit phase replayed without the night update, all 5 kinds).
  - C1: water ongoing crops only when dry or when production is tonight with fertilizer active. A standing strawberry is watered every
    other day. Teacher replays (M&M's intent from M&M's dawns): copy keys water -1.7..-8.4 / day (d13-22), package -1.4 / -7.7 (d6 / d10).
    Standing yield is equal (+/-0.3 units). The only yield loss is wheat water gain, -0.5..-2 units / day. Dry tiles next dawn: +1.6..+8.3
    (changes the network's input). The "field + deposits" gap is wheat that M&M buys (bought wheat counts as a deposit in that column).
  - C2: the compiler executes M&M's care intent (-0.03..-0.38 / day, d6-24); any own-state care gap is the network's care share.
  - C3b: copy keys on M&M's intent d13-22: feeds equal, 0 extra escapes in 600 world-days. Own states (runs/escown, 12 g3w worlds, identity
    12 / 12): all 32 escapes (12 cows, 20 sheep) follow a network feed ask of 0 for an unfed-yesterday group. Prices at release are mostly
    crashed in this bed (milk $1-42, wool $1-93); keepfed's current-price rule would keep 8 of 31 groups. Running: waterdaily (runs/wd),
    keepfed (runs/kf) on the copy, 30 worlds, full games.

- (Sep 30 ~20:10) M&M's extra day 8-11 spend vs the copy (runs/spend, tools_dc11/spend_trace, build_dc12v29; our traces replayed without
  shop pins, mean |day spend - duel| $114): +1,726 / world over 4 days: bought wheat +1,122 (65%, the no-gain churn), land +533 (Q4 on
  d10 in 30 / 30 vs ~27.5), cows +213, seeds +131; offsets geese -200, strawberry seeds -100, hires -125.
- (Sep 30 ~20:10) Forecaster money read without full-game divergence (runs/fcday25, teacher_day TEACHER_NET=1 on set A: one day from each
  exact recorded dawn, days 3-26, vs the opponent's recorded actions; pkg reproduces all 600 recorded game-days within $1; value = cash +
  shed at marginal next-dawn prices + standing yield; tools/dayread.py; confirmation use of A: arms pkg / fc2ens / fc3vens, this metric):
  fc2ens - pkg margin +738 (SE 311; own +232, opp -507 SE 162); fc3vens - pkg +589 (SE 194; own +159, opp -430 SE 132); fc3vens - fc2ens
  -149 (SE 295). Mechanism vs real opponents: strawberries move from h0-2 (-8..-11 / game) and h21-23 (-5..-7) to h3-11 (+13..+16).
  One-day effects only: no compounding, the opponent cannot react within the day.

- (Sep 30 ~20:40) Forecasting ceiling on the same exact one-day harness (runs/fcdayor): oracle (the opponent's actual sells for the day)
  - pkg margin +2,114 (SE 459) = own +62 (SE 434), opponent -2,052 (SE 274); fc3vens +589, fc2ens +738. The margin DP turns opponent
  knowledge into denial. About 1.4-1.5k / game one-day headroom beyond the learned heads (upper bound: the recorded opponent cannot react).

- (Sep 30 ~22:00) CORRECTION (astra-031) to the two one-day entries above: tools/dayread.py subtracted only the opponent's cash while our
  side counted cash + shed + standing yield. Denial that leaves the opponent holding stock was counted as its loss, so fc2ens +738 /
  fc3vens +589 / oracle +2,114 are provisional, not margins. The oracle split runs/orsplit was invalid: my xargs wrapper shifted
  one argument too many and dropped DC12_ORACLE_MODE, so every mix ran as the full oracle (NOT a build defect; an earlier note here
  blamed v24 / v25 wrongly). Rerun with symmetric assets: runs/sym (pkg / fc2ens / fc3vens valid) + runs/sym2 (oracle arms, same bug fixed).
- (Sep 30 ~22:00) Deposit timing (Imitation's request; runs/gainfc2, build_m13y; runs/hcurve, build_m13z): the copy's forecast sees the
  opponent's early wool / milk (wool d6-9 forecast 9.1 / 12.8 / 5.1 in h3-5 / h6-8 / h9-11 vs actual 8.0 / 16.7 / 1.0). The router's
  deposit value already prices arrival hour with the margin DP (wool 93 / 85 / 60 per unit at h0 / h4 / h8). Wool still arrives at h5-7:
  it rides a long trip (collect fertilizer, feed, care) on the land day. The hire search counts deposit gains, but the gains saturate at
  9 hires (extra routes stay empty). timing=0 sharpens the gains without moving deposits. The key Imitation asked for exists; the limit
  is route construction.

- (Sep 30 ~22:55) earlydep=1 (router compound move: a thin product's output stops first, then a shed stop; build_m13ed) on the copy,
  24 worlds to day 10 (runs/ed): deliveries barely move (wool deposited h3-5 8.3 -> 9.4), dawn-10 cash arm -22 / opp -57 (noise). rounds=12
  does not move them either (runs/gainr12). The router's objective does not value early delivery enough: Imitation's free-transfer
  bound (copy +1.94k, package +3.09k) is mostly a compounding chain (copy: opponent cash at dawn 10 -$213 -> final -$1.5k), which a
  one-day margin with rival weight 1 cannot see. Testing an opening rival weight (rivalearly / rivaluntil, runs/pkopen).
- (Sep 30 ~22:55, Imitation) Late game: given early stock, our DP dumps at dawn (milk h0-2 51 vs 32) and the opponent, reacting, sells
  later into the drained book and gains; free early delivery on days 10-29 costs -1.38k. Our within-day opponent forecast has no
  reaction term (first mover pays with few shops, last mover with many). Logged; no key.

- (Sep 30 ~23:20) Symmetric one-day margins (runs/sym, teacher_day build_dc12v31: both farms' cash + shed at marginal next-dawn prices +
  standing yield + animal-held product; set A days 3-26; pkg reproduces all 600 recorded days): fc2ens - pkg +574 (SE 264) = own +601
  (SE 411), opponent assets +27 (SE 494); fc3vens - pkg +308 (SE 252) = own +280, opponent -28. With the opponent's unsold stock counted,
  the learned heads' one-day gain is our own value, not denial (the cash-only read showed opponent -430..-507).

- (Sep 30 ~23:55) Package opening arms (runs/pkopen, 24 G3 worlds, full games, build_m13rf; base = g3w_pkg 24 / 24 identical):
  earlydep=10 -692 (SE 979); rivalearly=2 rivaluntil=10 -1,767 (SE 751); rivalearly=1.5 +2,268 (SE 1,441). Opposite signs for the two
  weights: noise at n = 24, no dose-response. Not pursued (two values only, no knob search).

- (17:05 UTC, system clock) Note on times: the labels "(Sep 30 16:50)" to "(Sep 30 ~23:55)" in the entries above ran ahead of the clock;
  they were written between ~15:50 and 17:05 UTC. Order is right. Labels from here on come from `date -u`.

- (17:07 UTC) Symmetric oracle split (runs/sym2, teacher_day build_dc12v31, set A days 3-26, base fc2ens; margins per game): full oracle
  +1,908 (SE 289; own -810, opponent assets -2,718: denial even with the opponent's stock counted); true day totals +1,012 (SE 276) vs
  true shape +680 (SE 295); per product milk +543, wool +521, strawberries +289 (eggs pending); per band h12-20 +1,197 (SE 293), others
  +0.27..+0.45k; days 3-10 ~0, days 11-26 all of it. One-day, non-reacting opponent.
- (17:07 UTC) Forced early deposit (earlyforce=10, runs/pkforce) was a no-op: every wool route already started with its pasture stops and a
  shed stop; margin -683 (SE 757) is noise. Layout (runs/layout, DUEL_LAYOUT, 24 worlds d3-9, package / M&M): animals' shed distance
  similar (sheep 1.79 / 1.52, cows 2.13 / 2.09); M&M has ~2 more hires a day (hire-days 40.3 / 54.1) and its hires collect WOOL first
  (h0-2 10.0 vs our 4.7) and milk later, ours milk first (h0-2 6.8 vs 0). Early-wool gap = crew size + first-wave product order.

- (17:31 UTC) Forced wool-first probes on the package (runs/pkwool, build_m13wf, 24 worlds, full games): woolfirst=10 (same crew) -850
  (SE 521), wool collections unchanged (hires h0-2 4.9 vs 4.7; M&M 10.0); + woolhires=2 -2,646 (SE 810), wool later (new hires start
  h1-2, farmer stops collecting). M&M's early wool needs its whole opening pattern (hires at the sheep in their first acting hour).
  Deposit timing closed on the compiler side.

- (17:34 UTC) Scenario dispersion (BC's residual tables; exact one-day harness, symmetric margins vs fc2ens): day-level Gamma factor
  scendisp 0.5 / 1.0 -448 (SE 140) / -779 (SE 157): over-wide day totals make our DP sell more, the opponent gains more (BC: day totals
  are near Poisson, v ~0.01-0.1). Calibrated band-level samplers running (runs/sband: scennb=1 = BC's nb_bands.hpp; scenlump=2).

- (17:38 UTC) Band-level scenario samplers (runs/sband, same harness): scennb=1 (BC's nb_bands.hpp) -1,085 (SE 170), scenlump=2 -225
  (SE 98); both raise own value and the opponent's more. All dispersion variants lower the margin: hedging gives denial away. Closed.
- (17:38 UTC) splitfert=10 (build_m13sf; days 3-9 milk / wool / egg harvest stops without the fertilizer collection, which becomes its own
  optional stop; after Codex's early-products audit): the package's early deliveries move with no forced order (24 worlds to day 10,
  days 3-9 per world: milk deposited h0-2 3.2 -> 5.5, wool sold h3-5 8.2 -> 12.5; pocket unit-hours wool 113 -> 89, milk 30 -> 20; same
  crew). Full games running (runs/sf/full).

- (17:48 UTC) splitfert=10 full games (package, G3; base g3w_pkg): first 24 +1,118 (SE 1,133); fixed 96 +308 (SE 469), own +708 (SE 371),
  opp +401 (SE 455); W/T/L 38/24/34 -> 54/0/42 but decided flips 21 / 20 (the base has 24 mirror ties): no win-rate signal.
- (17:48 UTC) splitfert=10 + nearanimals=2 (Codex's placement ported onto the bundle tree: build_m14sg, patch m16_sep30_splitfert_nearanimals_on_src_m14.patch):
  gate on 24 worlds to day 10, days 3-9 per world: wool into the shed by h5 12.6 -> 29.5 (M&M 20.0), milk later (like M&M). Full games
  on the fixed 96 running (runs/sfna/full).

- (18:09 UTC) Placement arms with splitfert=10 (package tree builds m14sg..m14sj; 24 worlds to day 10, days 3-9 per world, wool into the
  shed by h5 / milk by h2; M&M 20.0 / 6.0; splitfert alone 12.6 / 5.5): nearanimals=2 (sheep first) 29.5 / 0.0, full 96 +198 (SE 507),
  own +1,797, opp +1,599; =1 (nearest only) 21 / 24 games identical to splitfert (the router already gives cows the near sites); =4
  (alternate) 13.5 / 5.5; =5 (M&M census order: one cow, the sheep, the other cows) 22.2 / 5.0; =6 (=5 on days 0-9 + cows / sheep
  before geese) 22.4 / 5.0. Full 96 of =5 and =6 running (runs/sfna/full_na5, full_na6).

- (18:16 UTC) splitfert=10 + nearanimals=5 (M&M's census order; build_m14si, patch m18) on the fixed 96 (package vs g3w_pkg): margin +955
  (SE 517) = own +1,583 (SE 528), opp +628; 63 / 33 better / worse; decided flips loss->win 26 vs win->loss 10; vs splitfert alone
  +647 (SE 598). About 8 opening variants were tried on these 24 / 96 (selection bonus ~1 SE); confirmation on the 122 untouched G3
  worlds running, then the bundle stack (runs/bunstack). Caveat (Imitation): our local opponents sell wool at h6-8, real ones at h3-5,
  so early-wool denial is overstated on these beds.

- (18:24 UTC) splitfert=10 + nearanimals=6 (na5 order on days 0-9, cows / sheep before geese; build_m14sj, patch m19) on the fixed 96:
  +1,165 (SE 402) = own +1,323, opp +158; decided flips 24 / 10; vs na5 +210 (SE 513). Confirmations of na5 and na6 on the 122
  untouched G3 worlds running (runs/sfna/full_na5, full_na6).

- (18:33 UTC) CONFIRMED: splitfert=10 + nearanimals=5 on the 122 untouched G3 worlds (list_g3w218_mix minus the fixed 96; package vs
  g3w_pkg; build_m14si): margin +910 (SE 429) = own +1,039 (SE 373), opp +128; better / worse 80 / 42; decided flips 29 / 20. All 218:
  +930 (SE 330), flips 55 / 30. Against real opponents: Imitation's opponent-replay bed +0.48k (SE 0.49k, n 48); splitfert alone one-day
  on our live games (set A days 3-9) +320 (SE 101). Deployable: patch m18 on src_m14 (bundle tree). na6 confirmation + bundle stack running.
- (18:33 UTC) Placement all game (gate, 24 worlds to day 15, days 10-14): nearanimals=7 = na6 (cows > 3 steps 1.62 vs 1.64; few placements
  after day 9); sitelife=1 (lifetime site weight for crops and animals) worse (cows > 3 steps 3.62: crops take the near tiles earlier);
  animals-only variant (sitelifea) gate running.

- (18:39 UTC) Placement gates on na6 (24 worlds to day 15, days 10-14, > 3 steps ours / M&M): sitelifea=1 (animals-only lifetime site
  weight) sheep 1.10 -> 0.90, cows 1.64 -> 1.57; animalring=2 animalringday=13 (crops reserve ring 2 while the herd grows; build_m14sm,
  patch m22) geese 1.08 -> 0.25 (M&M 0.29), sheep -> 0.84, cows unchanged 1.66 (far cows come from day 0-5 placements); plantings -1.6
  per world over days 0-14. Not yet in full games.

- (18:48 UTC) CONFIRMED: splitfert=10 + nearanimals=6 (build_m14sj, patch m19) on the 122 untouched G3 worlds: margin +1,237 (SE 396) =
  own +1,397, opp +160; decided flips 30 / 15. All 218: +1,205 (SE 283), flips 54 / 25. vs na5 (218): +275 (SE 313). Candidate package
  key set; real-opponent bed read (Imitation) and the bundle stack pending. BC calibration: walking per extra step 0.13-0.15 turns per
  animal-day, so a calibrated sitelife would be ~0.07 of my sitelife=1 (which was ~14x too strong).

- (18:53 UTC) Bundle stack (runs/bunstack, fixed 96, build_m14si): the local bundle m3fc3vens_hc vs g3w_pkg +1,304 (SE 373); bundle +
  splitfert=10 nearanimals=5 vs the bundle -315 (SE 490), decided flips 13 / 24: the opening key does NOT add to the fc3vens bundle on
  this 96 (gains may overlap). Bundle + na6 running (runs/bunstack/bun_na6). BC (live replays, days 3-29): our far animals' product reaches
  the shed at ~h22 (milk from r4+: ours 66 units / game vs M&M 26); the far-cow cost is delivery time, not walking.

- (19:02 UTC) Bundle + splitfert=10 nearanimals=6 vs the bundle (fixed 96, build_m14sj): +961 (SE 502) = own +512, opp -449; flips 18 / 15.
  na6 stacks on the fc3vens bundle; na5 did not (-315). User decision: no more confirmation of small keys.

- (19:20 UTC) Intent v2 slim executor (src_v2 = src_m14 + m19 + v2; builds build_v2a..c): binding crew waves (no hire loop, <= 1 extra
  hire), courier deliveries for products with a deadline, fertilizer mode per species, DC12_V2LOG ledger, teacher_day TEACHER_V2 labels.
  Rung 1 (M&M's intent + M&M's v2 labels, 60 worlds, days 3-9): hires 7.47 vs 6.05 without (M&M 7.67), planned deliveries meet the
  deadlines, next-dawn value vs M&M +199 vs +146 (SE ~15). openscript=1 / 2 (BC's opening script: binding / crew floor + deadline drop)
  on the PR agent (m19-fc2-m68): crews reach the script (day 5 4.7 vs 6); deliveries were already close to the script (splitfert + na6).
  Real-opponent bed (runs/osrec, 48 worlds vs Imitation's pr_m19): openscript=1 -615 (SE 457), 16 / 32; openscript=2 -202 (SE 331). The
  opening crew does not pay; line stopped.
- (19:20 UTC) Dawn-seller diagnosis (runs/dawnseller, DC12_DPPLAN probe, build_dc12v35): on M&M's own stock our DP sells tomatoes /
  strawberries / milk / wool later than M&M (tomato dawn 1.34 vs 5.07, evening 7.24 vs 2.34) for equal same-day revenue. Forecasts are
  right (tomato opponent ~1.2 / day). Cause: stepped shop drains (tomato 3 at h0, then 2 every 4 h) make the h21-23 hour best for a
  lone seller; the DP's horizon is today + held units (hold 0.95) and ignores what an evening dump leaves in tomorrow's dawn book for
  tomorrow's new harvest. Next: measure the carry-over on 2-day continuations, then a two-day terminal value.

- (19:45 UTC) Dawn seller, part (a) DONE: evening dump vs holding to the next day, 2-day continuations from our exact live states
  (set A, 25 games, start days 6-26 = 525 game-days, package d3crop_m68_m3 vs the opponents' recorded actions; teacher_day TEACHER_DAYS=2,
  DC12_EVECUT probe, build_dc12v36 / v37; runs/evecut, tools/evecut_read.py). Symmetric values at dawn day + 2, arm minus base, per
  game-day (SE across games). Hold the h21-23 units of the product overnight (the next day's seller then sells them, mostly h0-20):
  all four -189 (SE 29) = own -21, opp +168; tomato -63 (SE 15); strawberry -57 (SE 18); milk -33 (SE 11); wool +4 (SE 8). Losses grow
  with the day (start days 18-23: all four -387). Hold + sell the carried shed stock over h0-2 next dawn: all four -303 (SE 45), milk -77.
  The evening dump is a denial move: a unit we hold lets the opponent's night / morning units sell before it at the better price.
  CORRECTION of my 19:20 entry: the "dawn book left for tomorrow's new harvest" is not a missing cost. The market is linear with fixed
  drains, so a unit sold tonight and a unit held and sold tomorrow shift tomorrow's book for our new harvest the same way. Only the
  unit's own price (drain in between minus others' sales in between) and the order vs the opponent differ. Part (b), a two-day terminal
  value, would push toward holding, so it is dropped. Open: a teacher-like share of the DAWN stock sold at h0-2 (BC: teachers sell ~30% of milk at
  h0-2, ours 13%), no evening hold. Probe next (after the deadline runs).

- (19:55 UTC) Dawn seller CLOSED. A teacher-like share of the dawn stock sold at h0-2 (no evening hold; BC's bands: teachers sell ~30%
  of milk / ~23% of wool at h0-2, ours 13% / 19%):
  - 2-day live-state continuations (runs/evecut, build_dc12v38, per game-day): milk 50% -7 (SE 4), milk 100% -16 (SE 5), wool 50% +1
    (SE 9), all four products 50% -13 (SE 11). Both farms lose money (all four: own -65, opp -52).
  - Imitation's hybrid-opponent bed (real farm + the learned seller that reacts to prices; runs/hybds, build_m14or7ds = build_m14or7 +
    dawnshare keys, patch m23; identity with runs/hybopp/h_pr exact on 2 worlds), PR agent + dawnshare=50 from day 10, per game:
    milk + wool -603 (SE 303, n 28), own -1,171, opp -568; all four -907 (SE 199, n 30), own -1,840, opp -933. Stopped at the
    first look (REJECT).
  A dawn lot sells our stock into the dawn book below the DP's own timing, and the opponent's units then sell lower too. Our late
  selling is not the gap on either bed. Together with the evening-hold probe (19:45): no seller-timing lever left on our states.

- (20:05 UTC) Wheat d10-27 (BC: teachers plant ~20-40 more wheat tiles): our compiler plants what the network asks (set A, 450 own
  game-days, DC11_INTENTLOG vs plantings: days 10-14 ask 7.08 / planted 7.07, 15-19 5.22 / 5.13, 20-27 7.33 / 7.20; no seed trims). The
  wheat volume gap is the network's ask, not the compiler (runs/wheatask).

- (20:25 UTC) Local-LB push (user direction via Imitation: beat Local-LB #1 m19-fc2-m68 with keys on m19). Lineage league
  (runs/lblg, tools/lblg_read.py; work/sep29_fund/build_m19 = src_m19 full_games_dc11; opponents = the Local-LB roster runnable on
  src_m19: m19 mirror, m14-fc2-m68, m3-d3crop-m68, mmpq-policy-v2; seeds 701-703 both seats, SHOP_CRN; 24 games per arm, paired vs
  pr_base = W/T/L 17/0/7, margin +2,657). Seller-order keys vs an identical evening seller:
  saleslots=4 +348 (SE 749), W/T/L 18/0/6 -> Weaknesses' exact LB replay (arms/pr_ss4); rival=1.5 -1,109 (SE 920); hourdisc=0.98
  -1,748 (SE 934, own -3,068); leader=0.5 -2,113 (SE 691, opp +3,150). Selling ahead of the mirror costs our own price more than it
  denies. LB data (round_robin.json): #1 is 15/15 vs rl-v1643, 19/11 vs m14-fc2, 20/10 vs mmpq-v2, >= 22/8 vs the rest.
  Wheat check: the compiler plants the network's wheat ask (99%; runs/wheatask), so the teachers' wheat edge is on the network side.

- (20:55 UTC) Exact C++ Local-LB replay + loss diagnosis of #1 (m19-fc2-m68). full_games_dc11 (work/sep29_fund/build_m19, no SHOP_CRN,
  seed = LB seed) reproduces the LB: mirror 977000 exact; #1's official games vs m14-fc2 (968000-14) and mmpq-v2 (971000-14) 55 / 60
  to the dollar (runs/lbrepro; compare.py, extract.py; ledger via Weaknesses' ledger_yield; tools/lbsplit.py, lbask_read.py).
  - #1 leads after d0-9 (+1.1k) and d10-17 (+0.9-1.4k) in all games; its 21 losses are decided on d18-29 (us - opp -2,939 vs +1,987
    in wins): strawberry -1,648 (-12.7 units), melon -1,221, wool -1,179, wheat -1,071 (fed more), tomato -833; eggs +1,834.
  - Cause = the network's plan, executed as asked (the compiler plants the asks on both sides). #1 and m14-fc2 share model.bin
    (689fbf06); #1's opening keys make it richer early, so it asks more strawberries on d6-9 (29.9 vs 27.0) and more geese (+3 on
    d6-13), and fewer on d10-13 (10.6 vs 15.2). Strawberries yield at age 10-16 and die (sim.hpp: max_yield events), so d6-9
    plantings are gone by d24; the d10-13 cohort feeds the end game (standing d24 11.7 vs 14.1, d28 6.4 vs 8.7), where the loss worlds'
    strawberry price is high (137 vs 104). Sent to BC (decode-side timing shift is theirs; input dumps runs/lbrepro/ask/indump_*).
  - Candidates on the same 60 seeds (paired vs #1): c2_rec110 +464 (SE 699), wins 39 -> 38; f1_fc2ens +723 (SE 585), 39 -> 40 (late
    wool for both farms). Neither moves the late-crop channel.
  - BC's block swap on the input dumps (reports/lb_pair_swap.txt, 21:15 UTC): the main network alone asks strawberries d6-9 4.07 vs 3.80
    per dawn and d10-13 1.70 vs 2.01, with a slightly HIGHER total crop ask on #1's states (15.20 vs 14.77): a share shift, amplified by
    the ensemble / decode. No single input drives it (global 41%, own grid 36%, animals 29%, plants 27%): the network reacts coherently
    to #1's richer farm. The only cheap lever is a decode timing bias. Under the user's end-of-competition rule (no tuned knobs, no
    lineage / LB-seed tuning) it is NOT run tonight; after the deadline, fix plant lifetime on the plan side, validated off LB seeds.
  - The C++ replay is deterministic run to run (rerun of 968002 identical). CORRECTION (21:10 UTC): the 5 / 60 misses are NOT all
    LB-side. Weaknesses' Python judge (LB code, real opponent bridges) reproduces mmpq 971000 s1 exactly, so the C++ harness diverges
    there. Candidates: no bridge soft deadline in full_games_dc11 (the bridge cuts at 0.9 s + overage / days left), and opponents run in
    src_m19 rather than their own bridges. Package identity: Weaknesses' Python judge on official games; the C++ replay stays a fast
    approximate bed (55 / 60 exact). All 5 misses are harness-side: Weaknesses' Python judge reproduces every one exactly (the 4 m14 misses
    after a seat-numbering mix-up; the LB's own runs 9 / 9 reproducible). The harness diverges on lineage opponents too (~1 game in 12).

- (21:25 UTC) Overfit audit of #1's opening keys on REAL-opponent states (user ask via Imitation). One-day continuations of our live games
  (set A, 25 games, recorded opponents; build work/sep29_fund/build_m19td = src_m19 + symmetric teacher_day; runs/keyaudit). Base = live
  56714867 keys; per game, days 0-9 summed: nearanimals=6 -106 (SE 74), own -102; splitfert=10 + nearanimals=6 -150 (SE 136), own -184;
  splitfert=10 -70 (SE 136). 5-day spans (dawns 0-5): na6 -66 (SE 45); #1's keys +80 (SE 87) = own -70, opp -150 (open-loop denial).
  Reconciliation: the recorded package (d3crop_m68_m3) + splitfert=10 = the old +320 (SE 101), identical games, so the build is fine and
  splitfert's sign depends on the forecaster (same network 689fbf06 in all three). honest1 (#1 - na6, clean fc3nvens) vs f1 (#1 +
  fc2ens), days 0-27: +185 (SE 262), every window slightly +. Reading: na6 has no gain on real-opponent states; its Local-LB gain is
  lineage-specific. Key provenance (which keys were checked on real data) sent to Imitation 21:17.
  m3_fc3nv (m3 + clean forecaster fc3nvens) vs m3, days 0-27, identity 700 / 700: +611 (SE 237), own +574, every window + (21:33 UTC);
  confirms BC's +585 (SE 241). The clean forecaster holds on real-opponent states; the opening keys do not.
  Old Kaggle sub 56653866 model (old_d3crop_v59: earlycrop 4 6 6 2, no m-keys) vs m3, days 0-27: -926 (SE 253), own -340, opp +585;
  days 3-10 +297, 11-18 -827, 19-26 -332; negative in every rank group (21:45 UTC). Today's build plays v59's logic (v60-v62 identity).
  Main-seed soups vs m3_fc3nv: soupG4 -307 (SE 139), soupG4E5 -213 (SE 187): keep the main. honest1 vs m3 +878 (SE 250); honest1 vs
  m3_fc3nv +267 (SE 139, days 3-10 only: splitfert + hirecheck / latehire) (22:30 UTC).
  Pick 2 (22:50 UTC), set B = 81 live games of 56714867 (pkfc2hc; untouched; runs/pickB): honest1 vs m3_fc3nv +224 (SE 70), vs m3 +256
  (SE 210); m3_fc3nv vs m3 +32 (SE 198) (set A +611: the forecaster step does not replicate). 5-day spans honest1 vs m3_fc3nv (dawns 3-9):
  set A +232 (SE 94), set B +187 (SE 66, top 10 +663 SE 230) (runs/pick5, tools/span_pair.py). Recommendation sent: honest1.

## Stop list (proven not promising)

- Statistical copies of M&M's hourly selling (mmfaith v1 / v2 on G1s -15.0k / -5.3k vs M&M's own lots with matched volumes and hour
  bands; Imitation's mean-share table -26.9k): independent draws with M&M's frequencies sell into the wrong books (Day compiler 14:25).
- Daily watering like M&M (waterdaily=1, copy line, 30 g3w worlds, full games): margin -5,450 (SE 809), 29 / 30 worse. The extra
  P_EXTRA water stops raise hires and drop real stops (optlate off). The asks move toward M&M's, but the labor cost dominates
  (Day compiler Sep 30 ~18:30). With optlate=1 the waters become optional and the router skips them (no state change): vs optlate
  -517 (SE 1,059); optlate alone -614 (SE 1,174). The watering state cannot be bought cheaply (runs/wdol, Sep 30 ~19:15).
- Day-6 seed trims on M&M's intent (package + hirecheck latehire, runs/cash6): no repair passes the stress-market funding check, so ~25 / 60
  worlds run the first expected-market-funded plan; live cash arrives in the evening, after the h18-21 plantings (~1 seed / world
  trimmed). Documented only: buyahead / cashshadow / seedlag / cashseeds already closed.
- Land-day cash timing on M&M's intent (runs/cashhour, TEACHER_HOURLY, tools/cashhour.py, package + hcl, 60 worlds):
  - Day 10: M&M sells early (h6 1.3k, h9 2.7k, h11 1.5k) and buys Q4 at h9. We sell steadily and dump 9.2 units at h23 (1.8k).
    Cumulative revenue at h12: ours 5.4k vs 8.3k; by the day end 10.8k vs 9.3k. Ours ends +2.4k cash, Q4 at ~h12, ~1 crop fewer.
  - Day 6: ours sells ~1.2k later in the morning and ends ~$200 richer.
  Cash + shed + stored crop yield next dawn is at parity (d6 +140, d10 -6, SE 146; animal-held product and standing crops' future
  output not included, astra-031), so selling earlier to buy land earlier is a measured trade-off, not a shown free gain. No key.
- Early-delivery route keys (Sep 30 16:30-17:30 UTC, package / copy, 24 G3 worlds): earlydep (compound collect-deposit-first move,
  rarely accepted, -0.69k), earlyforce (no-op: routes already started with pasture stops), woolfirst (-0.85k, SE 0.52k; no earlier wool),
  woolhires=2 (-2.65k, SE 0.81k; wool later), rivalearly 2 / 1.5 (-1.77k / +2.27k: noise, no dose-response), timing=0 / rounds=12
  (deposits unchanged). The free-transfer bound (+1.9k / +3.1k) is not reachable by route order or crew size.
- Scenario dispersion (one-day harness, symmetric margins vs fc2ens): scendisp 0.5 / 1.0 -448 / -779, scennb -1,085, scenlump=2 -225.
  More spread in the scenario opponent paths always lowers the margin (Day compiler Sep 30 17:38 UTC).
- keepfed=1 on the copy (override the network's animal releases): -325 (SE 182), 8 worse / 2 better / 20 equal. The releases are value-rational
  at the bed's crashed milk / wool prices (Day compiler Sep 30 ~18:30).

Caveat (Weaknesses, 19:20): every dawn-selling item below (dawnfirst, racing, lot caps, M&M's hourly table, dawnwait) was judged
while the h0 order cap was in force on BOTH sides (dc11 opponents capped at h0 on 94-100% of days 10-27). A "sell at dawn" probe could
not sell at h0 on 12-hire days. These items are not evidence against dawn selling once h0 slots work; re-judge dawn selling on a
contested bed (clones with saleslots) or live games, not on uncontested clone / league beds.

- Time blocks / schedules overriding models: morning animal block (duels -3.0k); + hold (-3.3k / -19.4k); M&M hourly-share
  sale table (-6.2k, 47 worlds); regime M (-4.0k); dayhold; dawnwait; racing after shop ticks; thin-product lot caps.
- Terms bolted onto the current seller model: hold multipliers, deny (-0.94k), dawnfirst (-0.8k duels; -2.4k with the
  morning block), rival 1.5.
- Funding patches: late crew, feed-cash guard, soft animal purchases, nightbuy, forced bigger early crews, day-10 land
  financing, bigger compile budgets.
- Required daily watering (-10 gate plantings), worker-turn value lean routes (-0.5k).
- Judging seller changes on recorded-opponent beds (pinned replays, teacher_day continuations); reactive rule opponents in
  pinned replays.
- Seller keys on the M&M copy in the local league (36 games each, paired vs the copy): hourdisc 0.98 -0.02k, hold 0.99 +
  hourdisc 0.98 -0.94k, lotcapthin 4 +0.05k, fitted M&M table -5.98k; herd-match decode (abias geese -0.6 d6-11, cows +0.6
  d8-16) -0.42k (Imitation). Plan transplant of M&M's daily new entities into the copy: no change (6 worlds).
- (BC) Network-input "fixes" for compiler habits: dryblind on the M&M main (clone bed -959, SE 352; with members -1,587).
- (BC) Ensemble land-mixing fixes: solo 8 8 (duel bed -0.37k, SE 0.42k). landcrop (step 61) built, untested in games.
- (BC) Macro dials / guidance / median-path or mixed dial plans (best dial arm -1.9k vs hyb_nolp; mixed plans -3 to -4k); 10-game
  dial screens were noise.
- (BC) Team-conditioned opponent forecaster FCT7 (true-label oracle -142, SE 344, 440 pinned games); forecaster shape losses
  (cumloss 0.3 / 1.0, thin x3: sale_score +1.4 / -1.8 / +3.2 vs big2, SE ~2).
- (BC) Planting-financing pushes seedfin / expbest (full games -200, SE 433); fundtries / delaytries (-3 to -4.6 plantings d2-4).
- dc12 seller rules on the current opponent assumption: reservation-level race (+3 / -2 levels, with or without excess
  dumping): -2.7k to -9.5k in duels (Day compiler 18:30).
- Animal funding patches: buyahead, affordanimal (cows -2.80 / -3.02 vs -1.68) (Day compiler 18:50).
- Every-day dawn slots (a slot per stocked product): pinned -1.2k, uncontested league -1.72k, contested clones -1.28k; the cause is
  lost production from the smaller first wave (Day compiler / Weaknesses / Imitation 19:45).
- Forecast input fixes on the hyb_nolp line vs M&M states: DC12_LATEHARVEST 0.0, stockcap -2 / day (rung 1) (Day compiler 19:45).
- BC's shed-estimate gate on the forecast (opponent flow 0 while its shed estimate is 0): rung 1 -63 / day (lower MAE, worse
  decisions: the removed volume arrives unforecast later) (BC 20:00).
- Seller-decided dawn slots without pricing the crew (saleslots=2): pinned +34 (ns), own -430 (Day compiler 20:20).
- Leader seller with an omniscient follower opponent (it sees our committed schedule): rung 1 -141 / day, sells later (Day compiler 20:55).
- cashroute (deposit credit for the cash need, purchases kept at their hours): no change on days 1-11 (Day compiler 20:35).
- affordanimal on top of cashsell + wheatcash: day 2-4 plantings and cows worse again (Day compiler 20:40).
- Leader seller v1 (opponent re-plans hourly on the actual inventory with a history forecast of us), w=0.5: rung 1 -28 / day; full
  games vs our DSM / DECEM clones -4.0k (SE 0.8k) / -1.6k (SE 1.0k): the clones take the denial it gives up (Day compiler 21:35).
- downprobe (short probes in the hire search's step down): -25% routing time but -16 / day (fewer crew reductions) (Day compiler 21:15).
- Funding: affordall (per-purchase earliest affordable hour, animals first): days 1-11 plantings collapse (day 6: 3.5 of 20); the
  ladder accepts funded variants despite drops. trimnet on the v3 copy (live): sheep drop unchanged, -0.33k (Imitation).
- cashseeds=0 (cashsell without its seed tolerance): wide bed -1.09k, no better than m1 (Day compiler).
- Seller hold discount 0.98 (hold more at the noon reference): M&M-world bed margin -903 (SE 593), the follower gains (Day compiler 23:55).
- holdown (tomorrow's own supply = today's output): wrong for 2-3 day pulses; rung 1 -37.7 / day (Day compiler 23:30).
- Seller hold value variants that hold more: holdsteps=20 (pinned -652, SE 622; league -0.35k), holdbest (M&M bed -764), hold 0.98
  (-903). They raise both farms' income, not the margin (Day compiler 00:20).
- collectall (collect every animal holding product each day, with nighttrim): pinned live29 -2,556 (SE 358), own -4.2k; M&M bed
  arm -2.4k. The network's batched collection is better than smoothing by collecting everything (Day compiler 00:58).
- rivalnight=1 (the margin term counts the opponent's forecast sales from tonight to the hold step): pinned live29 -553 (SE 1,278),
  median -783, own -1.3k for opponent -0.8k; the extra denial costs more than it takes (Day compiler 01:42).

- fieldflow=f (forecast the opponent as taking f x each drain): G1s f=0.7 -4.54k, own -3.5k (the DP dumps stock) (Day compiler 02:40).
- hourdisc 0.995 / 0.99 as the seller fix: G1s -1.22k / -1.15k vs base -1.10k; denial bought with own revenue 1:1 (Day compiler 02:36).
- seedlag (release seeds 4-8 h after the animals in the funding variants): weeds +1.0 / day (Day compiler 02:43).
- skipwater alone (drop the WATER of skipped plantings): plantings 0.94 -> 0.48, the first funded variant skips half (again, 02:35).

- rivalnight (denial over the hold horizon): see above. landpull: the land hour does not limit plantings. land_push 8 8 10 (decode):
  pinned 200 +3 (SE 109) (Day compiler 03:20).
- ticksell / tickdawn / tickegg (M&M's clock rule copied): G1s -4.85k / -5.27k (Day compiler 03:05).

- cashshadow (deposit bonus before the cash-bound hour as achieve variants): G1 d2-5 +0.001 plantings (Day compiler 04:10).
- latehire (hires the funding sim cannot pay ordered later): no change on day 1; our 1-hire crew is the router's cost choice.
- Holding evening units to the next day (tomato / strawberry / milk; wool neutral) or dumping the carried stock at dawn: 2-day live-state
  continuations -189 / -303 per game-day (runs/evecut; Day compiler 19:45 UTC). A two-day terminal value for evening units: dropped.
- Dawn share (dawnshare=50, at least half the dawn stock at h0-2, days 10+): hybrid bed -0.60k (milk + wool) / -0.91k (all four);
  2-day harness -7 to -16 per game-day (Day compiler 19:55 UTC).
- On m19 vs our lineage (24 games): hourdisc=0.98 -1.75k, rival=1.5 -1.11k, leader=0.5 -2.11k (Day compiler 20:25 UTC).

## Network-side issues seen from the compiler (for BC; Sep 30 01:10)

- Collect intent batches cohorts: days 18-24, 28% of cow holders collected a day, 3.4 units each (router drops none); our farm
  creates evenly (CV 0.68) but collects in pulses (1.16; M&M 1.02), and the seller sells the pulses through. Collecting
  everything instead lost big (collectall, stop list), so the batching saves labour; only a smarter split would help.
- Early asks vs cash on the copy / hybrid lines: with m1 funding, the day 2-5 animal asks are bought, dawn cash on day 6 is
  ~$30, and the land head (money is an input) no longer asks on day 6 (seed 504 copy vs E: logit -2.5 vs +13.4 without m1); the
  day-10 landpush then buys Q3 instead of Q4. The live d3crop line is not affected (pinned: Q2 / Q4 days identical).
- Feed intent mid-game (pinned, d3crop_m68 + m1): 16.7 of 21.9 animals fed a day on days 18-24; bonus due but unfed 0.26 / day;
  capped product lost 0.37 / day (small).
- Router deposit values from the learned seller's rollout (Day compiler 05:00): the rollout cannot reproduce the model's inputs;
  best case = the DP seller.
- Forcing the day-8 land ask (D6, decode land_push 8 8 10): pinned 200 +3 (SE 109); live slip 18% vs M&M's own 11% (Day compiler 03:20,
  Weaknesses 05:00).
- feedvalue=1 (feed stops valued at the banked product): no feeding recovery under feedcost; alone -1.03k on the 40-game F2R slice
  (Day compiler 05:32).
- achieve=1 / feedcost=1 on the package (line B): 760 -439 (af), -718 (afrn) (Weaknesses / Day compiler 05:32).
- More seller work on the package line: G1s 216 worlds, our DP = M&M's selling (-102, SE 180); scenopen +164 (SE 171) (Day compiler 05:46).
- Package funding keys (reserve=0 survivalfloor [+ dropany saleslots nighttrim]) on the copy: league -1.31k / -1.50k vs copy_or_v5d_af (96 games); judging funding keys on rung 2 / M&M's asks (Day compiler 06:21).
- scenopen (non-anticipative scenario seller) on the package: G1s 216 worlds +164 (SE 171), league 288 games +38 (SE 231) (Day compiler 06:42).
- animalfirst=1 on the copy (Imitation's league, 132 games): -399 (SE 354) vs copy_or_v5d_af, though it cut day-9 drops (Day compiler read 06:54).
- Early-deposit credits on the package (depcredit 0.1 / 0.2): league +0.36k / +0.20k (480 games) but the 760 -0.82k / -0.92k and G3 quick level; the DP holds the earlier stock for the evening (Weaknesses' firing check) (Day compiler 07:55).
- depcredit=0.1 + scenopen=1 on the package: the 760 -1,179 (SE 385, 120 games; own -927) (Day compiler 08:23).
- entityvalue=1 (new animals at expected net production, BC audit item 5): alone -206 (SE 217), with feedvalue -327 (SE 183) vs feedvalue (Day compiler 09:09).
- saleslots=4 / 2 on the package (DP seller): league +5 (SE 263, 192 games); with feedvalue -311 (SE 273) vs feedvalue (Day compiler 09:54).
