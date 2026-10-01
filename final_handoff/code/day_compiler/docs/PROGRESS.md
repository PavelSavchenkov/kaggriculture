# sep29_dc12 progress

- (18:30) Folder created (user: a clean folder with only the relevant material; the old one stays linked). Moved in DESIGN.md
  (from sep25_compiler_overhaul/DC12_DESIGN.md) and REWORK.md; copied the beds' scripts into tools/. Router read in full:
  AUDIT.md R1-R12. Main router assumption: a $2 cost per worker turn inside a paid day (R1). Audit parts requested from BC
  (bind), Weaknesses (funding) and Imitation (market + executor seller).
- (18:35) R1 tested: router turn cost 0 is worse (hires +0.6, moves +21 / day, value -140 / day, waterings unchanged): keep. Imitation's
  seller audit: M1 (point forecast, no opponent response) is the core fault, measured on 72 M&M worlds. BC: tomato waterings
  -15 to -17 per game come from dc11's ongoing-crop watering rule. Weaknesses: dropany=6 level on d3crop_m68 (760 games +34,
  wide bed +189).
- (18:45) All audits in (router: me; market / seller: Imitation M1-M11; funding: Weaknesses 11 items; bind: BC 10 items).
  DESIGN.md "Synthesis": reuse sim / router / realize / executor shell; rebuild the seller (reservation-level opponent
  response), the funding (sales-scheduled purchases, stock counted as cash, network drop order) and the values (production-
  based service values, yield-valued waterings). Starting the seller.
- (18:30) dc12 seller v0 (race=1, dc12 tree work/sep29_fund/bc_dc12; History records market inventory; the opponent as a
  reservation-level seller at L = last day's mean + 3; sell at post-drain hours down to L plus the excess that does not fit
  tomorrow's drains). Days 12-17 on M&M's intents: evening sales 34 -> 28 units / day, dawn 27 -> 31; days 2-4 unchanged (the
  morning cash needs early fertilizer collection). Duels (6 pinned M&M worlds): arm (copy in M&M's seat) margin -2.8k (own -3.7k,
  opponent -0.9k); our sub's seat -2.7k.
- (18:30) v1 (L = mean - 2, no excess dumping): arm margin -9.5k (own -2.8k, opponent +6.7k); sub seat -0.7k (copy +6.9k).
  Reading: in this linear market every unit we withhold is revenue for the opponent, and every unit sold early costs us price
  unless sold high. M&M does both (own revenue = our copy's, opponent -2.8k); a reservation rule tuned on M&M's averages
  reproduces neither. The seller needs the opponent's response learned, not assumed. Both closed (stop list).
- (18:50) Day-3 cow cash path (q335 bed, d3crop_m68 line + cwc; cow short in 32 / 40 day-3 cases). Traced 114739423: the dawn
  wheat is sold at h0 and partly bought back at h2-h3 for feed; the variant with a deposit bonus before h6 does collect and sell
  fertilizer early ($438 by h6-8), but the cow order sits at h9, after ~$200 of other purchases. Ranking animals before seeds in
  the purchase deferral (probe DC12_ANIMALFIRST): cows -1.68 -> -1.75, no change. The cow's order hour follows its placement in
  the routes (after the pasture / placement stop), not the cash; M&M buys it at h8 when the morning cash is there.
- (18:35) Offline check (Imitation, 72 worlds): the reservation assumption over-predicts the opponent by +12 to +21 milk / day;
  the learned forecast predicts M&M's schedule (corr 0.6) but not a reacting opponent (0.3). New structure: forecast base path x a
  fitted U-shaped response to the price level our sales leave (quasi-static in the DP; key response=1; bc_dc12 market.cpp
  build_response / inventory_at / rival_at). With a placeholder curve (M&M milk units by level): days 12-17 barely change
  (evening 33 vs 34 units); duels: copy seat -2.6k (own -3.3k), sub seat own +0.4k, margin -0.9k. Waiting for Imitation's fitted
  curve (per product, M&M-like vs lineage opponents). Every change to the copy's seller read negative on the 6 worlds (-0.8k to
  -3.3k): next seller variants go to the 48-world bed.
- (18:40) affordanimal (a short animal purchase moves to its first affordable hour, found by replaying the plan with the animal
  ordered every hour before the later purchases; generalizes the land repair): cows -1.68 -> -2.80, plants -1.48 -> -1.85, dropped
  +1.52 -> +3.02 (fallbacks 0.70 -> 0.18). The plain plan's cash path is late, so the cow is placed late and dropped while the plan
  counts as funded. Closed. Imitation: the response-ratio form fails its offline gate (our and the opponent's deviations co-move:
  confounded, no substitution); a causal response curve needs duel forks (DUEL_PERTURB, Imitation). Clean forecast fix found by
  Imitation: zero / cap the opponent forecast by the stock it holds (MAE strawberry 0.61 -> 0.41, milk 0.36 -> 0.29, wool 0.25 ->
  0.19 vs M&M; mirror milk 0.71 -> 0.57). Building it as key stockcap.
- (18:50) Funding patches for the day-3 cow, all closed: buyahead (animals bought at the first hour the cash covers them) cows
  -1.68 -> -3.02 (plants -1.48 -> -0.78); affordanimal (above) -2.80. Weaknesses: the cow fails on cash, not placement: our workers
  collect ~as much fertilizer by h8 as M&M but carry it (deposit 0.9-2.3 of 3.6-5.4 by h8), and sell little dawn wheat. For our
  own agents the herd gap is ~$0.2-0.6k / game vs a thin-book price gap of -9 to -10k vs the top four: seller first.
- (18:50) Seller: Imitation found the scenario seller's information-relaxation bias (each scenario's continuation is solved in
  hindsight, so waiting is overvalued every hour; ~25% of the planned lot sold per hour, the rest rolls to the evening). But scen=0
  in the league: +0.06k (SE 0.56k): the deterministic plan also schedules late. Built scenopen (non-anticipative: the mean-path
  policy scored on every sampled path): days 12-17 evening 34 -> 28 units, dawn 27 -> 33 (M&M 34.5). Duels (6 worlds): scenopen copy
  seat -2.9k, sub seat margin +2.8k; stockcap copy seat +1.1k, sub seat -0.4k; both -1.8k / +1.4k. 6 worlds cannot separate these:
  league arms requested from Imitation.
- (18:50) Weaknesses (54 live games vs ranks <= 10): the thin-book price gap sits at DAWN (h0-2 $/unit ours vs theirs: milk 84 vs
  113, strawberry 98 vs 142, wool 64 vs 110; ~-3.3k / game); evening prices at parity or better. Days 10-27 the top teams sell 29.8
  strawberries / game at h0 ($148, before the town drain), we sell 2.3, then sell at h1 at $95 after their lot. The race is for the
  day's first turn, from stock carried overnight. BC (87 unseen M&M games): two intent-independent execution gaps, fertilized
  production-night waterings (~14 units, ~$1.3k / game) and care drops (~$1.1k / game); classification of the cause requested.
- (18:52) stockcap on pinned real games (200, same dc12 binary; the dc12 tree without keys reproduces the package exactly):
  trimmed -80 [-388, +202], median 0: flat. Weaknesses (54 live games vs the top 10): our dawn stock equals theirs (strawberry 9.3
  vs 10.6); they sell a 4-7 unit h0 lot on 15-25% of days (16% of dawn stock), we on 2%, and we sell a big lot at h1 after theirs.
  The h0 decision is the seller's target; asked Imitation for the DP's h0-vs-h1 valuation from the audit logs.
- (19:00) The h0 decision, measured (days 12-17, M&M's intents, thin units per game-day): ours h0 0.15 / h1 13.3 / h21-23 20.4 vs
  M&M h0 5.2 / h1 5.7 / h21-23 8.5. scenopen: h0 0.18 / h1 16.4 / h21-23 16.4; + lumpy opponent sampling (the opponent sells with its
  historical frequency per hour, lot = forecast / frequency): h0 0.26. Our DP never sells at h0: h0 sales execute before the day's
  largest drain, so waiting one hour always looks better in its model. M&M takes the pre-drain price to stay out of the crowded h1,
  where our big lot sinks the price for everyone: a game effect (value from the opponent crowding h1) the model does not represent.
- (19:00) BC: (1) their care / yield-watering gap numbers were a probe artifact (h23 actions invisible); rerun pending, fix on hold.
  (2) A real History bug: an ongoing crop's harvests after its last production are not counted, so the opponent's strawberry
  stock reads 3.9 vs 8.6 on M&M worlds (fixed: 8.3; P(true 0 | inferred 0) 0.47 -> 0.96). It biases the forecast and stockcap. In
  the dc12 tree as probe DC12_LATEHARVEST (History is shared by both duel seats, so a one-seat test needs BC's per-model key).
- (19:00) ROOT CAUSE of the dawn gap (Imitation's audit, fixed in dc12): the engine takes 10 orders a turn and each hire is one
  order; realize ordered up to 10 hires at the first hour, and the executor gives sales only the free slots. On 9-10-hire dawns the
  seller's h0 lots were dropped silently (strawberry decided 1.80 / executed 0.18 per dawn) and sold at h1 behind the opponent's
  lot. A hidden assumption in the router / realize (first wave = 10) that ignored the seller. Key saleslots=1: bind counts one
  first-hour slot per product with dawn stock (not fertilizer) in realize's first-hour check, so the first wave shrinks and the
  rest start at h2. Days 12-17 (M&M intents): h0 thin 0.15 -> 9.43 / day (M&M 5.24), h1 13.34 -> 5.38 (M&M 5.67), evening 20.4 ->
  17.5; one-day value -24 / day, hires 11.89 -> 12.04. Duels (6 worlds): copy seat -1.45k (sub +1.4k), sub seat margin +1.3k.
  League / 48 worlds (Imitation), 760 + wide (Weaknesses), pinned running.
- (19:00) BC retracted the care / yield-watering gaps (probe could not see h23 actions): dc11 matches M&M there. Closed.
- (19:25) saleslots=1 judged: pinned real games (200) trimmed -1211 [-1540, -905], own -1695, rival +673; Imitation's league
  (their <= 4-slot variant, uncontested) -1.72k (SE 0.31k), contested (opponents with slots too) +0.37k (SE 0.67k). Ledger on
  live29 (tools/ledger.sh, ledger_cmp.py; per game, arm - base): wheat / egg / fertilizer move 50-60 units h1 -> h0 at a small
  gain; milk / strawberry / wool sell at h0 (~$87) what used to sell in the afternoon / evening (~$150); days 0-9 identical.
  Weaknesses (54 live games vs the top 10): the top teams sell at h0 only on flooded days (strawberry P 0.45 when the book rose
  >= 5 since yesterday's h0, 0.02 when it fell > 10), 1-2 slots, 8.1-8.3 h0 hires.
- (19:30) saleslots=2 (slots only for the thin products the seller's own dawn decision sells): M&M states days 12-17 h0 5.96 /
  h1 9.18 thin units per day (M&M 5.24 / 5.67), one-day value -14 / day vs no slots.
- (19:35) ROOT-CAUSE step (user: isolate the compiler on M&M states; find root causes, not symptoms). Executor probe
  DC12_SELLLOG (dc12 tree): the DP decides the same h0 lots with or without slots (strawberry ~228, milk ~177 per game-log, days
  10+); the cash-cover and overflow loops add ~0 at h0. So base's "no h0 sales" was the 10-order cap silently dropping a DP
  decision, a hidden "wait for h1" rule; the slot fix exposed a wrong DP decision. First cause inside the DP: the denial term
  (rival=1 subtracts the opponent's revenue on its FORECAST units; every early unit is credited for the forecast units it makes
  cheaper; held units get no such credit). M&M states days 12-17: rival=0 cuts the saleslots h0 lot 9.43 -> 6.08 and the slots
  stop costing one-day value (vs M&M +229 with slots vs +226 without; with rival=1 +159 vs +183). Running: the true opponent flow
  (TEACHER_ORACLE / REPLAY_ORACLE, dc12 build_dc12d) with and without slots, to split the rest into forecast vs objective.
- (19:50) Rung 1 margin (tools/margin.py; margin = our value_next - opponent money, both vs M&M's actual day): line +90 / day
  (own +183, opp +93). With M&M's intent the compiler reproduces M&M's production (plants per crop, harvests, field units 87.6 vs
  87.5, field value within $5), ends +$1.2k richer with 22 fewer shed units; waterings 54.5 vs 60.6 (extra dry plants are
  one-day-safe), idle 5.7 vs 0.5. True opponent flow (TEACHER_ORACLE): +86 / day (SE 15). Split: true volume + forecast shape +51
  (SE 14), forecast volume + true shape +47 (SE 13); per product milk +36 (SE 12), wool +16, strawberry +10, melon +7, egg +3,
  carrot / tomato 0. DC12_LATEHARVEST 0.0 (the tf forecaster reads its own History), stockcap -2. rival=0 -32 / day (SE 16):
  the denial credit is right against a fixed opponent.
- (19:50) Rung 2 (5 days from M&M's dawn at days 10 / 15 / 20; our network decodes days 2-5): margin vs M&M -428 / +18 / +716
  (SE ~300); true flow +467 / +464 / +718 better; our network's day-1 intent instead of M&M's: no worse; saleslots=2 -37 / -208 /
  -110; all arms end ~3 plants behind M&M (the network's later intents; the compiler plants exactly M&M's day-1 intent).
- (19:50) Root cause of the dawn-slot production loss (TEACHER_REASON compile reports): the extra drops sit on 13-hire days (the
  max_hires cap; the 14th hire costs $377). A 13-hire crew needs all 10 first-hour slots + 3 at h1; each sale slot moves a hire to
  start at h2 (one worker-hour), and on cap days the lowest-priority stops fall off: output waterings (op 9), new plantings (op 8),
  fertilizer collects. saleslots=1 reserved slots without pricing this. Fix direction: a joint slot / crew decision: a dawn sale
  takes a hire's slot only when the DP's h0 gain beats the router's cost of the later start and nothing is dropped.
- (20:05) saleslots=3 (dc12 tree): the crew is routed as before; if the seller's dawn decision wants more h0 sales than the free
  slots, the router runs again with the smaller first wave, kept only if the crew is the same and no required stop is dropped.
  Rung 1: h0 3.15 thin units / day (M&M 5.24), plantings +0.05 / day, harvests +0.00, dropped 0.10 vs 0.18, margin +4.8 (SE 7.6).
  Pinned live29 (75): +462 (SE 177), trimmed +405 [+70, +714], p 0.014, own -31; saleslots=2 +243 (ns); saleslots=1 -3,570.
  Imitation's contested read on M&M's recorded play (48 worlds) for the every-day variant: -0.63k (SE 0.26k).
- (20:05) Day-10 land (Q4) root cause (rung 1 days 6-11 + rung 2 from day 10; TEACHER_REASON reports with land hour): M&M sells
  after the shop drains at h4 / h6 / h8 / h9 (~$4.6k by h9), buys the land at h9.9 and plants the new quadrant in the afternoon.
  Our seller holds for the evening (~$2.4k by h9, $3.3k at h21-23); the funding check then finds land + goose + seeds short at h10
  and the ladder takes F14 (22 / 40) -> 5 dropped plantings, 13.9 vs M&M 20.1 plantings. The stress scenario is not the blocker
  (stress=23: 19 / 40 at h14). Selling and purchasing are decided separately. cashsell (+wheatcash): land at h9-10, dropped 0.25,
  plantings 16.9; one-day margin -530 (investment not valued by the one-day metric), 5 days +311 (SE 349). Prior full-game reads
  of cwc for our line: +0.16k / +0.02k (760), so the revenue-bonus form helps the land day but not overall.
- (20:05) Rung-2 planting gap from day 10 (5 days): M&M created 70.0 plants, our network intended 64.8, our compiler created 60.3
  (half network, half compiler; almost all wheat). BC's shed-estimate gate on rung 1: -63 / day (lower MAE, worse decisions).
- (20:20) saleslots=3 pinned (live29 + live28, 200 real games, vs dc12base2 = hybp, same tree): +362 (SE 109), trimmed +302
  [+124, +492], Wilcoxon p 0.0012, 57% up, own -31 (the gain is the opponent's lower prices behind our dawn lots). saleslots=2
  +34 (ns; own -430 from the production loss). First root-cause fix with solid evidence; contested bed (Weaknesses) and M&M
  recorded-play bed (Imitation) running.
- (20:20) Early days on M&M's intent (rung 1, days 1-11, tools/fidelity.py): plantings day 2 / 3 / 4 2.7 / 1.5 / 1.9 vs M&M 4.7 /
  3.4 / 2.7, hires 3.5-4.3 vs 5.6-6.0, day 3 at fallback level 1 in every game (the cow short under stress at every deferral hour;
  cropfirst trims crops). Same coupling as day 10: M&M harvests wheat at dawn, deposits and sells it at h1 ($163), runs a steady
  fertilizer collect-deposit-sell loop ($373 by h5) with 7 workers; ours 4.8 workers, $77 by h4, cash at h8-9. The router's
  deposit values do not see the cash need. cwc recovers part (day 2 plants 3.3, no drops). Judges: 5-day continuations and
  continuations to the game end cannot value early investments (5 days: money + shed ignore plants / animals; to the end: SE 3-4k
  on 40 games); early funding needs full-game beds.
- (20:35) cashroute=1 (dc12 tree: when the plain plan is short at hour t, route again with every unit deposited by t earning
  1 x its price on top, purchases kept): no change on days 1-11 (day 3 fallback 0.78, same cash path). The re-route runs and still
  fails: the cow is short at h4-7 even with full deposit credit, because by then the day cannot raise $500 + seeds. M&M's day 3
  order is different: sell collections all morning, buy the cow at h8 ($664 cash), plant in the afternoon (seeds h11-12), place
  the cow in the evening. Ours plants in the morning and places the cow at h5-7: purchases are tied to the hour before first use
  and the router orders stops without cash. Root cause: a router with no cash in its stop order. The deferral variants approximate
  it by delaying purchases (then other stops break). Moderate lever (~1 planting on day 3); waiting for full-game reads first.
- (20:35) Imitation, M&M recorded-play bed (48 worlds, d3crop +- saleslots=3): margin -0.00k (SE 0.19k), own -0.45k, M&M -0.45k
  (the every-day variant -0.63k). Pinned (all real opponents) +362 (SE 109).
- (20:55) Leader seller v0 (dc12 tree, key leader=w; Imitation's framing: our DP is a follower, M&M a leader, worth ~1.4k / game vs
  our lineage on M&M's farm). Per thin product: the DP plan vs "sell down to level l" schedules (l = last day's mean inventory
  -6..+10), scored on the joint path with the opponent as a follower that best-responds with our DP to our committed schedule
  (its stock now + visible output arriving over the day), mixed with the DP's fixed-forecast value (weight 1 - w). Rung 1
  (fixed opponent): w=1 -141 / day (SE 29), w=0.5 -51 (SE 14); both SELL LATER (h21 lot moved to h23), the opposite of M&M.
  Log (DC12_LEADERLOG): the follower sees our whole schedule and front-runs each committed lot, so the model punishes commitment
  and prefers holding. That is not M&M's first-mover gain: M&M sells in the first post-drain slot (h1) after the overnight
  production, which nobody can front-run (h0 is pre-drain), and our DP followers forecast from past sales, not from our plan.
  A correct leader model needs an opponent that reacts to prices (not to our plan): the reservation-level family, which lost
  under the h0 cap (to be re-judged only on contested beds). Shelved; code stays behind the key.
- (21:00) MILESTONE m1 = dropany=6 cashsell=1 wheatcash=1 reserve=0 saleslots=3 (each key a root-cause fix, see above). Frozen tree
  work/sep29_fund/bc_dc12_m1, build work/sep29_fund/build_dc12_m1. Pinned real games (200, hybp line + m1 vs hybp, same tree):
  +1,720 mean, trimmed +1,495 [+866, +2,081], Wilcoxon p < 1e-4, 70% up (live29 trimmed +1,276, live28 +1,662), own +1.7k;
  without saleslots=3 trimmed +1,005 [+355, +1,667]; dropany alone +533. Kaggle-time harness (20 F2 games, overloaded machine):
  dawn step mean +16%, soft-deadline dawns 65 vs 73, overage left mean 48.2 s (worst 21.9). Sent to Imitation (league,
  contested, 6 / 48 worlds) and Weaknesses (760, contested). Live packages carry none of cashsell / wheatcash / reserve=0 /
  saleslots=3 (d3m68_v94_drop has dropany=6).
- (21:15) Compile time (m1, M&M days 1-17, TEACHER_REASON profile; loaded machine): routing is 80-90% (m1 326 of 386 ms mean,
  day 10 708 of 783); realize and the funding simulation < 25 ms. Inside the router (DC12_ROUTEPROF, days 6-17): 9.9 route_day
  calls / 296 local-search rounds per compile; the hire search's step down (dropany: up to 7 dissolution candidates, each a full
  6-round search) is 71% of routing; passes relocate 31%, tails 24%, entity 21%, swap 17%. downprobe=1 (short probes pick the
  candidate, one full search on it): routing -25%, margin -16 / day (SE 8): fewer crew reductions found; =2: -18% time, -11 / day.
  m1 is inside the Kaggle budget (overage left mean 48 s), so speed does not buy quality now; kept off.
- (21:15) Day 10 under m1: the label's intent has 17.9 new crops, M&M created 20.1 (replantings the label conversion misses), we
  create 16.85: the compiler is ~1 planting short of the intent.
- (21:15) Leader v1 (the opponent re-plans every hour on the actual inventory with a forecast of OUR flow from our last 3 days;
  own_flow recorded in History): rung 1 w=1 -137 / day, w=0.5 -28 (SE 11) with own +17 / day and opponent +45: against a fixed
  opponent the DP's denial is real and any reacting-opponent model gives some of it back. Reacting bed (m1 vs m1 + leader=0.5, full
  games vs the DSM / DECEM clones, seeds 500-519 both seats) running.
- (21:25) Oracle value by hour band (rung 1, true flow only inside the band): h0-1 -9 (SE 12), h2-11 +28 (SE 12), h12-20 +30 (SE 13),
  h21-23 +25 (SE 13) (all bands +86). The dawn forecast is not the loss; mid-day and evening flows matter equally.
- (21:25) m1 on reactive beds (Imitation's judge, paired, keys-off build = their baselines): v3 copy + m1 +0.61k (SE 0.43k, 72
  league games, wins 43% -> 53%); d3crop + m1 +0.55k (SE 0.80k, 36 so far); recorded-M&M bed (48 worlds) +0.69k (SE 0.43k); melon68
  + m1 +0.38k (SE 0.51k). All positive, ~1-1.5 SE each; pinned (fixed opponents) +1.5k.
- (21:35) Leader v1 (w=0.5) on a reacting bed (full games vs the DSM / DECEM clones, our own DP followers; seeds 500-519 both seats,
  paired with m1): DSM clone -4,016 (SE 816), own +137, opponent +4,153, wins 34 -> 26; DECEM clone -1,630 (SE 1,002). The clones
  do not react as modelled; they take the denial the leader gives up. Both leader versions CLOSED (code behind the key). The
  follower-DP's denial credit (rival=1) holds against fixed opponents (rung 1) and against our reacting clones.
- (21:45) m1 on reactive beds (Weaknesses, d3crop_m68 + m1 keys vs d3crop_m68, build_dc12_m1): contested (clones also on m1's keys,
  160 paired) +930 (SE 418), own +464, opponent -466; standard 760 interim (350 games) +1,201 (SE 223), own +534, opponent -666;
  saleslots=3 alone contested +366 (SE 254). Imitation (league vs the 3 strongest, 72): v3 copy + m1 +0.61k (SE 0.43k), d3crop + m1
  -0.31k (SE 0.57k); recorded-M&M bed +0.69k (SE 0.43k). m1 is the first compiler milestone positive (or level) on every kind of bed.
  Late game on rung 1 (days 18-28): +261 / day above M&M, m1 +7 (ns): no compiler gap there. Imitation: M&M's selling on our stock
  timing (mmpolicy) -6.2k; we collect as early as M&M but deposit late (milk 2% vs 21% by h2); early deposits pay only with early
  selling, and each half alone loses on our farms.
- (22:00) Day-3 variant failures traced (DC11_FUNDDEBUG now names the failing unit): the h10 deferral variants fail on WATER of an
  empty tile: with cashsell a short seed order is tolerated (the executor buys what cash covers and skips unseeded plantings), and
  the skipped crop's same-day water then fails the funding simulation. Dropping those waters (tried) removes the fallbacks (day 3
  0.78 -> 0.10) but plantings fall (day 3 2.28 -> 0.92, day 4 2.35 -> 0.88, day 6 17.7 -> 15.6): plans that buy the animal first and
  leave little cash for seeds now pass, and the executor skips most plantings; the level-1 fallback (trimmed intent) planted more.
  Reverted. Root cause: the funding check accepts the FIRST funded variant (with the seed tolerance, "funded" can mean most
  plantings silently skipped) instead of comparing variants by what they achieve. Next: score variants by realized plantings /
  purchases (value of the executed plan) rather than first-funded.
- (22:15) Imitation's full m1 battery (paired, keys-off = their baselines): contested league (opponents with saleslots=1) d3crop
  + m1 +1.58k (SE 0.71k), v3 copy + m1 +0.57k (SE 0.61k); uncontested league v3 copy +0.61k (SE 0.43k), d3crop -0.31k (SE 0.57k);
  48 M&M worlds (v3 copy) +1.33k (SE 0.59k); recorded-M&M bed +0.69k (SE 0.43k); 6 real worlds -1.72k (SE 1.72k). Weaknesses: 760 at
  530 games +1,246 (SE 197). Wide bed (53 worlds vs our sub; the bed that matched live): -1,243 (SE 863), own +603, sub +1,846:
  per-key split and ledgers running (Weaknesses). Pinned own / rival split: dropany own +974 rival +312; + cashsell / wheatcash /
  reserve=0 own +1,716 rival +345; + saleslots=3 rival -327 (m1 own +1,738 rival +18).
- (22:15) Early days: lower turn cost (wage 1 / 0.5) does not raise early hires (3.65 on day 3) or plantings: the turn cost is not
  the crew limiter; extra workers only pay if earlier cash has value. The early-game gap (day 2-4 cow placed 20-40% vs M&M 75-100%,
  Weaknesses) needs purchases, deposits and sales planned on one cash path; every one-piece fix lost or did nothing.
- (22:15) depcredit=lambda dephour=H (dc12 dev tree, for Imitation's both-halves test with mmpolicy): thin-product deposits by h8
  earn lambda x price on top. Rung 1: 0.5 -> deposits by h8 2.69 -> 3.96 / day, thin sold h0-8 +1.1, margin +4 (SE 13); 1 -> -17.5.
- (22:25) Evening lot vs night room (rung 1, m1, DC12_SELLLOG nightlog): at h20-21 the room is ~32 units, the charge binds on 1% of
  evening decisions, the shed holds ~19 thin units and the workers' pockets ~64 units carried overnight (auto-deposited at night).
  The h21 lot is the DP's own choice (the h21 price beats 0.95 x tomorrow's), not the night room. No compiler fault on rung 1.
- (22:35) Wide bed (53 worlds, our arm vs our capped sub) m1 -1,243 (SE 863), explained by ledgers (Weaknesses' d3m68ref rerun,
  identical to Imitation's): the sub gains +$1,884 / world on strawberries at the same volume. Bands: our h0-5 strawberry price
  $126 vs $104 (+$644: the dawn race works); the sub (a capped evening-selling DP follower that forecasts us) moves ~20 units from
  its morning to the drained evening (+13.6 evening units at $134 vs $126); our evening price $137 -> $130. A game effect, not a bug,
  consistent with Imitation's league split (d3crop + m1: contested +1.58k, uncontested -0.31k). Per-key wide runs pending (if it is
  saleslots=3: m1 without it is the lineage-safe variant, saleslots=3 a defence against dawn sellers).
- (22:45) Live opponent mix (Weaknesses, our live games vs ranks 1-30, 163 opponent games, days 10-27): 94% are dawn sellers
  (h0 sale on > 15% of days; typical 40-50% of days, 22-36% of thin units at h0-2); ours h0 on 5% of days, 46% of thin units in the
  evening: we are the capped evening follower of the field. The contested reads represent live; the wide bed's capped-sub opponent
  is 1-6% of it. HANDOFF.md written (m1, evidence, closed, open, tools).
- (22:55) depcredit with fertilizer (depfert=1; early days, m1 line): day-3 cash by h8 $332 -> $525 (M&M ~$664), hires 4.0 -> 4.65,
  but plantings / cows unchanged (day 3 2.33 vs 2.28; fallback 0.70 vs 0.78). The day-3 cow stays mostly unfunded; not pursued.
  Imitation: build_dc12_m1 aborts on hyb_v3m68_ms (BC's patcher step 64 ".decode mainshare" missing): hybrid + m1 packages must be
  built in BC's tree with m1's compiler delta merged (told BC).
- (23:05) m1 final 760 (Weaknesses): +1,097 (SE 174), own +338, opponent -759, all eight seed sets positive. Clean patch
  m1_saleslots3.patch (102 lines vs bc_v95 = v95 + BC's local additions); bc_v95 + patch (bc_m1min / build_m1min) reproduces
  build_dc12_m1 exactly on 1,280 rung-1 day rows. Sent to BC for packaging (the user's decision; no submission).
- (23:15) Router audit additions: R13 no fixed farmer assignment in dc11 (route 0 filled by the same search); R14 the step-down
  searches 52 full local searches per compile (2.3 levels per route_day, 2.3 candidates per level); most of the cost is the final
  level confirming that no candidate fits. A safe prune needs an exact infeasibility bound; speed is not binding (m1 inside budget).
- (23:25) Animal-first purchase ranking with m1 (DC12_ANIMALFIRST; +depcredit/depfert): day 3 plantings 2.08 / 2.30 (m1 2.28),
  fallback 0.92 / 0.75: no gain. On day 3 the animal is usually bought (cows next dawn -0.02 vs M&M); the loss is ~1 planting on
  days 2-4 plus a later cow placement. Early-game funding stays open (tested: ranking, deposit credits, deferral, turn cost, crew).
- (23:35) trimnet (network's trim order) with our own intents (TEACHER_NET, days 1-9, m1): identical to m1 (day-3 trims 1.12 vs
  1.08, margin -0.3): the network's order equals the cost order on these days. Fertilizer on rung 1 (m1, days 12-17): ours 15.5 sold
  / day, 40% at h0-2, 23% at h21-23; M&M 10.6 / day, 89% at h0-2 (carried overnight, sold at dawn). No measured loss vs a fixed
  opponent; not pursued.
- (23:45) Wide bed per-key split (Weaknesses' runs, paired vs d3m68ref, 53 worlds): m1 -1,243 (SE 863); m1 - wheatcash -594 (794);
  m1 - cashsell +30 (851), own +852, opp +822; m1 - saleslots=3 -951 (837). The channel is cashsell (earlier funding sales feed the
  capped evening follower), not saleslots=3. Candidate live variant m1b = dropany=6 wheatcash=1 reserve=0 saleslots=3 (no cashsell):
  asked Weaknesses for its 760 and contested reads.
- (23:55) Wide split complete (Weaknesses): dropany alone +189 (SE 620), same as its earlier v94 read. m1 vs m1 - cashsell ledgers:
  with cashsell we fund more geese (eggs +18.7 units +$774, spend +$450) and produce 7.4 fewer strawberries; the sub's strawberry
  prices rise (+$1.5k, evening band days 10-19 and afternoon days 20+). An investment-mix effect on the product the sub competes in,
  not early selling on ordinary days (on M&M's states, m1 and m1 - cashsell sell identically on days 12-17). Rung 1 m1 - cashsell:
  day 10 -1.95 plantings / +4 dropped (land late again), day 2 cow -0.25. Imitation: M&M's fitted schedule on our farm -4.9k /
  -5.6k (with depcredit), depcredit alone +0.22k: M&M's operating point is not transferable piecewise (closed). Imitation's 6
  real worlds: M&M's recorded farm actions + our seller +4.5k over the copy; answered with the rung-1 caveat (their copy decodes its
  own options; rung 1 on M&M's full labels shows per-day execution parity) and pointed them to the hourcsv logs.
- (00:10) Multi-day M&M-label continuation (teacher_day TEACHER_LABELS=1: later dawns compile M&M's label when our groups match its
  schema): 0 of 4 later dawns match. After one day our groups split differently (groups key on type, age, yield, dryness,
  fertilizer, so one M&M group becomes two of ours or the reverse: e.g. c3a7 n20 vs n8 + n14), and label options are per-group
  counts. Transferring them needs an approximate translation layer; not built. Rung 1 per-day parity on the full label stays the
  execution evidence.
- (00:25) cashseeds=0 (dc12 dev tree): with cashsell, a short seed order fails the funding check again (the tolerance can let
  animal purchases crowd out seeds silently; wide ledger: geese +, strawberries -7.4). Rung 1 days 1-11 ~identical to m1 (day 6
  16.7 vs 17.7 plantings, day 10 equal). Wide bed m1 + cashseeds=0 running (build_dc12d reproduces Weaknesses' m1 wide games, 3 of
  3); pinned m1b (m1 - cashsell) live29 trimmed +1,397 (m1 +1,276), live28 running.
- (00:40) m1b (m1 - cashsell) pinned (200): +1,213 mean, trimmed +1,085 [+502, +1,675], p 0.0002, 64% up; vs m1 trimmed -279
  [-838, +289] (ns). m1b keeps most of the pinned gain and is level on the wide bed.
- (21:49) Note on times: the entry labels from (20:05) to (00:40) were estimates that ran ahead of the clock (the (00:40) entry was written at ~21:45 real time); the order is right, the clock times are not. Labels from here on use the system clock.
- (21:51) cashseeds=0 on the wide bed: -1,090 (SE 772), own +1, opp +1,091 (strawberry +1,048 for the sub): the seed tolerance
  is not the channel (m1 -1,243; m1 - cashsell +30). Imitation's league (hyb_v3m68_ms line, 72 games): cashsell inside m1 worth
  ~+0.8k (m1 +0.38k SE 0.52k; m1 - cashsell -0.39k SE 0.50k), the opposite sign; each ~1-1.5 SE. cashsell unresolved: both m1 and
  m1b go to the user. Pinned cashseeds=0 run stopped (not a candidate).
- (22:04) Full-game losses vs the DSM clone (m1, 20 games, FULL_EVENTS probe in full_games_dc11): escapes 1.65 / game before
  day 25 (most late, h23), discards 9.8 units / game at night on days 21-29 (half wheat; strawberries 31 of 196). The traced mid-game
  escapes are the network's feed 0 on unfed sheep while wool sells at $31 then $1 (book crashed) and wheat $31-34: value-rational.
  keepfed=1 (feed an unfed animal when its remaining production beats every-other-day feeding) never triggers there: correct.
  Discards (~$400 / game) are an execution loss: the night room uses the plan's night_carried, not the actual pockets.
- (22:13) Imitation's m1 vs m1b split: every dawn-selling / reacting bed favours cashsell (contested league d3crop +1.0k,
  v3 copy +0.4k; recorded-M&M +0.6k; hybrid league +0.8k); only the wide bed goes the other way. Recommendation: m1 (with cashsell)
  as the candidate, m1b as the fallback. Discards: base line 8.8 units / game, m1 11.9 (vs the DSM clone).
- (22:13) Discard root cause (DC12_ROOMLOG / DC12_OVERFLOWLOG): on end-game nights the plan itself carries more than the shed
  holds (day 27 planned 143 units in pockets, actual 148; day 25 138), after the router's 30 overflow repair rounds (3-39 units still
  over per plan): almost every worker ends the day with 5-20 units and several routes run past h23. Output stops (harvest / collect)
  have a must-do priority tier even when their product cannot be stored. nighttrim=1 (router): after the repair rounds, remove the
  pure output stops (harvest / collect only, no entity, not survival) that cut the night cargo most until it fits; the product
  stays on the animal / crop. vs the DSM clone (20 games, paired with m1): discards 9.8 -> 2.6 / game, margin +213 (SE 364), own
  +185. Pinned (200) and 60 more clone games running. (A first version segfaulted: manual route edits left the stop -> route map
  stale; fixed with erase_stop + dead.)
- (22:16) Weaknesses: m1 - cashsell 760 final +437 (SE 174) vs d3crop_m68, -660 (SE 170) vs m1: cashsell worth ~+0.7k on the 760
  and the contested bed, -1.27k on the wide bed: full m1 is the main candidate, m1b the hedge. nighttrim rung 1 late days: -3.8 / day
  (SE 7), discards 0.31 -> 0.07 / day. Model dir for Weaknesses: triage/models/m_d3m68m1nt (d3crop_m68 + m1 + nighttrim).
- (22:21) nighttrim clone games (paired with m1): DSM 40 +290 (SE 349), DECEM 40 -23 (SE 383), pooled 80 +133 (SE 258);
  discards 11.9 -> 2.9 / 11.0 -> 1.6. Weaknesses running its 760 + contested on build_dc12d (identity OK); pinned 200 running.
  Trim order (for Imitation's herd split: the copy places 1.9 of 2.6 asked sheep, 3.4 of 3.8 cows on days 4-14): animals are trimmed
  sheep -> cows -> geese (most expensive first). trimnet=1 (the decoder's least-log-likelihood order) is empty on hyb_nolp (the
  decoder fills trim_order only in its counts-head branch), so the earlier trimnet test was a no-op; suggested an arm on the copy.
- (22:24) nighttrim pinned (200, vs m1): +9 (SE 67), median 0, trimmed -35 [-133, +61]: no harm (end-game overflow is rare in
  the pinned games); vs base +1,730 / trimmed +1,448 (as m1). Candidate m2 = m1 + nighttrim pending Weaknesses' 760 / contested.
- (22:28) Other execution losses in full games (m1 vs the DSM clone, 10 games, FULL_EVENTS lossstat / orderfail): failed unit
  actions 2.6 / game (negligible); failed order units 6.0 / game, almost all day-29 liquidation sell orders above the shed stock
  (harmless); plants turned to weeds 2.1 / game (~$150). Apart from the night discards (nighttrim), execution is clean.
- (22:32) Animals asked vs planned in our own full games (hyb_nolp + m1 vs the DSM clone, 20 games): cows 100% planned; sheep
  4.95 -> 4.65 per game, the shortfall on days 8-10 (the $500 sheep short at every funding variant on land days). Imitation: trimnet
  is live on the v3 copy but leaves its sheep drop (2.5 -> 2.0) untouched (margin -0.33k, SE 0.30k): not budget trims. Same funding
  coupling (~$200-300 / game); not pursued now.
- (22:34) QUALITY GATE on the live lineage (Imitation, build_dc12_m1): d3crop_m68 + m1 vs d3crop_m68, league vs the 3 strongest (72
  games) +1.50k (SE 0.53k), margin +1.02k (60% wins) vs -0.48k; contested (36) +0.53k (SE 0.65k). With the 760 +1.10k, pinned +1.5k,
  contested clones +0.93k: m1 beats the original agent on every representative bed.
- (22:36) affordall=1 (each purchase its own earliest affordable hour on the plain plan's cash timeline, animals before seeds):
  CLOSED. Days 1-11 on M&M's intents: day 6 plants 3.5 vs 20 (15 dropped stops), day 1 loses its cows, days 2-4 plantings 0.3-0.6.
  The release hours come out late (the timeline counts only dawn stock + deposits), the router cannot fit the dependent work, and
  the ladder accepts the "funded" variant despite its drops. Two findings for any future joint planner: (1) variants must be
  compared by what they achieve (drops, plantings), not accepted at the first funded one; (2) a cash timeline without the seller's
  own schedule under-states morning cash.
- (22:37) bestvariant=1 (the funding ladder keeps the funded deferral variant with the fewest dropped required stops, not the
  first funded): days 1-11 on M&M's intents nearly unchanged (day 9: dropped -0.2, sheep +0.1, geese +0.08, plantings +0.08; other
  days ~0): the first funded variant is usually already drop-free. Not pursued. Funding area closed for this session.
- (22:38) nighttrim on the contested bed (Weaknesses; m1 + nighttrim vs m1, clones on m1 keys, 160 paired): +268 (SE 129), own +436,
  opp +169; F2R +235, top-clone +301. With clone games +133 (SE 258) and pinned +9 (SE 67): a modest real gain; 760 pending.
- (22:40) m2_nighttrim.patch (82 lines on top of m1_saleslots3.patch, against bc_v95): bc_m2min / build_m2min reproduce
  build_dc12d's m1 + nighttrim full games exactly (4 of 4).
- (22:44) Loss audit, full games (m1 vs the DSM clone): weeds 2.1 / game, 2.0 from dryness, mostly end-of-life ongoing crops
  (tomatoes age 11 / strawberries age 15-16 with yield 0, days 21-28: rational) plus a few ripe wheat on day 27; production lost to
  the held cap 0.2 / game. Execution is clean apart from the night discards (nighttrim).
- (22:44) Imitation (6 exact worlds): our arms earn what M&M earned; the margin gap is the sub's extra income against us
  (+4.3k to +5.7k from day 12): M&M sells at dawn and after each drain, the book never recovers and the follower sub dumps at
  h21-23; against us the sub gets the day. depcredit=0.5 in the league: own -2.06k, opponent -2.27k (early deposits deny, at a cost
  equal to the gain); mmpolicy own -4.87k, opponent +0.03k. They are splitting depcredit's own cost (routes vs price).
- (22:44) nighttrim interim 760 (my paired read of Weaknesses' files, 610 games incl. the contested m1-clone sets): +74 (SE 61),
  own +219, rival +145, discards 10.5 -> 3.4 / game; standard sets ~0, contested sets +235 / +301. A small safe add-on.
- (22:49) nighttrim 760 (paired vs m1, Weaknesses' files): standard sets 630 games +47 (SE 64), own +183, rival +136; with the
  contested sets 790 games +92 (SE 57); discards 11.2 -> 3.4-3.6 / game. Small, safe, own +0.2k: m2 = m1 + nighttrim is the
  recommended package if a rebuild is made anyway; m1 alone is fine.
- (22:57) nighttrim final (Weaknesses): standard 760 -19 (SE 60), vs d3crop_m68 +1,078 (m1 +1,097); contested +268 (SE 129); pinned +9.
  Kaggle-time harness (10 games): m2 dawn step 1.67 s vs m1 1.72, overage left 33.6 vs 32.3 s. m2 = m1 + nighttrim recommended.
- (22:57) Deposit values vs Imitation's trace (sub's harvest rides in pockets to h20-21, the day's lots collapse into an h21
  dump): measured per-unit deposit value over carrying overnight, M&M days 12-17: strawberry h8 $18.4 -> h21 $10.4, milk $20.4 ->
  $9.9, wool $23.7 -> $12.0: late arrival costs ~$5-10 / unit in the model because late units can be held at 0.95 x tomorrow's
  price. The valuation matches the executor's choice (it dumps only when that beats the hold), so steepening the curve would be a
  patch; not built. Any extra real cost is in the price path (M&M takes each drain first): a seller / game question.
- (22:58) Hypothesis for the denial gap vs follower opponents: with rival=1 the DP already credits each unit sold during the day
  for lowering the price of the opponent's later units (vs a follower dumping 20-30 units at h21-23, ~$50-75 / unit), so if it
  still holds for the evening, the learned forecaster (trained on mostly top-team, early-selling opponents) likely under-predicts a
  follower's evening dump (Imitation: +0.5-1.2 units above forecast by the evening in the mirror). Test: the m1 arm without the
  learned forecaster files (falls back to the trailing 3-day profile of the actual opponent) on the wide bed (vs our capped sub) and
  on pinned (dawn sellers). Running.
- (23:03) m1 without the learned forecaster (trailing-profile fallback) on the wide bed: -3,977 (SE 742) vs m1, own -3,553: the
  learned forecaster is essential; the "under-predicted follower" hypothesis is not an easy lever. Closed; pinned run stopped.
  Weaknesses' live waste audit (206 games vs ranks 1-30): we discard less than the top teams live (5.1 vs 11.1); missed late
  fertilizer 20 vs 6.9 (days 20-29 at $10-13: ~$150 / game); decay-weeds 16 vs 8 (end-of-life, rational): no big waste item.
  Imitation: depcredit on 48 M&M worlds -0.15k (SE 0.59k), routes barely move: closed. New seller metric (Imitation): price lead per
  unit vs the same opponent on the M&M-world bed: M&M +3.7 / +7.0 / +15.4 (strawberry / milk / wool; ~$2.4k / game), copy + m1
  -2.7 / +1.9 / +1.1. M&M sells 12-16% at h21-23, the copy 27-36%.
- (23:04) Seller calibration on Imitation's price-lead metric (72 M&M worlds, arm in M&M's seat vs live d3crop; duel_mm ported into
  snapshot work/sep29_fund/bc_dc12s; tools/mm_bed.sh; reader sep29_mm_copy/scripts/sell_race.py): arms d3crop_m68 + m1 (reference),
  + rival=1.5, + rival=2 (margin weight on the opponent's revenue), + hold=0.9 (overnight hold value). Running.
- (23:20) Imitation: m1 failure mode on the hybrid line (startcomplete=6 cropfirst=6): days 0-5 spend all cash (dawn cash $2 / $32
  on d5 / d6 vs $145 / $212 without m1), the land slips (Q2 d9 / Q3 d11 / no Q4 vs d7 / d9 / d11), seed 505 -16.5k in both seats;
  land < 4 in 3% of that line's games, 0 of 168 on d3crop_m68 + m1. Root cause: the funding horizon is one day; nothing keeps tomorrow's
  land cash (the next-dawn feed reserve did so by accident). m1 stays the d3crop-line candidate; a hybrid package needs a land reserve.
  Imitation also: live, the field sells steadily through the day (M&M and its opponents sell ~0.6-0.75x the drain each; the book
  drifts up); our local beds have only evening sellers, so seller changes flip between local beds and live: judge seller changes on
  pinned (field) and the M&M-world bed (denial) together.
- (23:26) Seller calibration on the M&M-world bed (72 worlds vs live d3crop; paired margins vs d3crop_m68 + m1): rival=1.5 -74 (SE 471),
  rival=2 -572 (SE 609), hold=0.9 +95 (SE 499). Higher rival weight moves our sales earlier (strawberry h21-23 35% -> 30% -> 26%) but
  the follower moves later (45% -> 50% -> 51%) and takes the better evening prices: strawberry price lead +2.2 -> -1.1 -> -2.8.
  hold=0.9 sells more in the evening (40%). No gain: closed.
- (23:35) SAFETY BUG (Imitation / Weaknesses, seat-swap bed 114911253, d3crop_m68 + m1 in akmr's seat): day 8 buys Q3 and ends at $10
  (reserve=0 + cashsell); days 9-10 every level is unfunded and the agent runs an idle plan (no unit actions); all 13 animals escape;
  final $10 vs $182,403. 1 of 234 real-world games (0 of 1,840 clone, 0 of 318 wide) at -186k. Fix survivalfloor=1: when every
  level is unfunded, the SurvivalOnly level's routed plan (harvests, survival water, feeds capped by the cash budget) runs instead
  of idle. Reproduced and fixed with build_dc12s/duel_mm: d9 "status ok fallback 4 hires 4", d10 dawn $1,129, final $98,338 vs
  $106,218 (-7.9k; without m1 +3.6k: the d8 overspend is the second fix, a next-dawn cash floor). Pinned 200 (side effects) running;
  Weaknesses asked for the swap bed + 760.
- (23:35) Imitation (234 exact live games, real play): the top team's per-unit price lead over our sub is systematic: ranks 1-10
  +$3.1k / game (strawberry +9.5 = day mix +7.2 + hours +2.3; wool +15.0; milk +2.7), ranks 11-30 +$1.6k; every top-10 team positive;
  h21-23 share 13% vs our 39%. The hold value (which days) and after-tick selling (which hours) are the team's top priority.
  holdown (lowers the hold value): rung 1 -37.7 / day, more evening selling: the wrong direction. Testing holdsteps=20 (hold lot
  priced at tomorrow h20 instead of h12) on the M&M-world bed.
- (23:58) survivalfloor side effects: pinned 200 (hybp + m1 + sf vs hybp + m1): 0 of 200 games changed. Clean patch
  m3_survivalfloor.patch (39 lines on top of m1 + m2); bc_v95 + m1 + m2 + m3 (bc_m3min) = build_dc12d on 6 pinned games (identical).
- (23:58) Production timeline (new probe: duel_mm DUEL_SELL "field" column = product waiting on own tiles; tools/made_phase.py,
  detrended per game; Imitation: prices fall ~40% from days 12-17 to 18-24, read within blocks). M&M-world bed, m1 arm, days 18-24:
  - The high arm-vs-opponent output correlation is a twin artifact (arm and d3crop share the network): made units, arm vs d3crop
    milk / wool +0.65 / +0.69; arm vs M&M's real opponent +0.47 / +0.39; M&M vs d3crop +0.48 / +0.38.
  - Our farm CREATES more evenly than M&M's (CV milk 0.68 vs 0.82, wool 1.18 vs 1.54); our COLLECTIONS pulse (milk 0.68 -> 1.16,
    wool 1.18 -> 1.87; M&M 0.82 -> 1.02, 1.54 -> 1.45). Imitation, live herds: purchase phase equal, cows held at dawn 2.13 vs 1.70.
  - Collection probe (DC12_COLLECTLOG, per species; pinned live29, d3crop_m68 + m1): the router drops no collection; the
    network's collect intent collects 28% of cow holders a day on days 18-24 (3.4 units each), cohorts in batches.
  Root cause: collection timing is a network decision that batches cohorts, and the seller sells the batches through.
  Probe collectall=1 (every holder collected; the shed buffers, the seller times) queued on the M&M-world bed (build_dc12f).
- (23:58) Seller hold value on the M&M-world bed (72, paired vs m1): holdsteps=20 margin +789 (SE 596), arm +2,035 (SE 577),
  opp +1,246; hold=0.98 margin -903 (SE 593), arm +759, opp +1,662 (the follower takes the evening: strawberry h21-23 45% -> 50%).
  Model: in a linear book our units leave only through drains, so selling the held lot as one lot at tomorrow's lowest expected
  inventory is optimal; the fixed noon reference undervalued holding. Key holdbest=1 (dev tree): the lot's step = tomorrow's
  argmin of expected inventory. M&M bed running (build_dc12e); pinned for hs20 running (Imitation: holding tends to win on reacting
  beds and lose on pinned).
- (23:58) Imitation's base question queued on pinned 200: d3crop_m68 + m1 (pe_d3m68m1) vs the pure copy + m1 (pe_cpp5cm1).
- (00:20) Hold value closed. holdsteps=20: M&M bed +789 (SE 596), Imitation league -0.35k (SE 0.50k), pinned 200 -652 (SE 622,
  median +7). holdbest=1 (tomorrow's argmin step): M&M bed -764 (SE 619), arm +1,171, opp +1,935. hold=0.98: -903. Every variant
  that holds more raises both farms' income, not the margin (the follower sells into the prices we leave).
- (00:20) collectall=1 on the M&M bed: arm -2.4k after 12 worlds (extra collected wool / eggs dumped at low prices; no nighttrim in
  that model). Stopped; pinned with nighttrim queued (pe_dc12ntca).
- (00:20) survivalfloor final: swap bed 234 games, only the collapse game changes; clone identity 160 / 160; league 72 / 72
  identical. Package sent to BC: m1 + nighttrim + survivalfloor (patches m1 / m2 / m3).
- (00:20) Land slips, sizing: d3crop line + m1 (Weaknesses, swap bed): Q3 / Q4 delayed 2+ days in 1-2 of 234 games. Copy line
  (CPP5c_v3 vs + m1, 24 full games vs agent_sep23): identical land days. Hybrid + m1 seed 505 vs D (reproduced): m1 funds the
  day 2-4 cows (dawn cash $2-187), the day 6 / 7 land asks come with 15 / 11 strawberries and cows / geese, the stress funding fails,
  landtrim (4 / 12 / 30) cannot fund, and the ladder drops LAND first (NoLand); Q2 d8, Q3 d10, the network never asks Q4 after
  day 10: -16.5k. Key landfirst=1 (the land alone before NoLand): land 4 (Q2 d6, Q3 d9, Q4 d10), margin -10.5k / -11.8k. Sent to
  Imitation for the hybrid judge (build work/sep29_fund/build_dc12h = their src_dc12s + landfirst).
- (00:25) Kaggle-time harness (FULL_BUDGET=2.5, 10 games vs the DSM clone F2, machine load ~13-20), packages with landfirst:
  d3crop_m68 + m1 + nighttrim + survivalfloor + landfirst: dawn step mean 1.60 s (p95 3.37, max 4.68), no soft-deadline dawns,
  overage left mean 35.7 s (min 29.0); copy CPP5c_v3 + the same keys: 1.43 s (p95 3.20), overage left 40.9 (min 34.6).
- (00:25) m1 vs M&M's recorded play in M&M's 72 worlds (vs live d3crop; Imitation's mm_rec, open-loop): margin +1,129 (SE 810).
- (00:25) Early days on rung 1 (m1, M&M's intents, days 1-9): plantings 2.72 vs 4.68 (d2), 2.40 vs 3.40 (d3), 1.62 vs 2.72 (d4),
  17.4 vs 20.2 (d6); day 3 falls back on 88% of dawns: the cow / sheep purchase is short under the stress funding at every
  deferral hour. Cash path d3: M&M sells wheat at h1 and fertilizer ~1 / hour from h5 (a collect-deposit-sell loop) and buys at
  h6-h11; we sell wheat at h0, dump fertilizer at h9 and buy from h10. Probe nofund=5 (days 1-5 run the full ask unfunded; the
  executor drops what it cannot pay) = the value ceiling of better early cash planning: M&M bed + rung 1 running.
- (00:35) nofund=5 on the M&M bed (d3crop_m68 + m1; days 1-5 run the full ask unfunded): margin -206 (SE 357), 48 / 72 identical;
  rung 1: fewer cows (the executor drops the unaffordable cow order), plantings lower. The funding check is not what costs the early
  days on this line. Closed. (Imitation's opening transplant: M&M's days 0-5 in its own worlds are worth only +0.8k on the M&M bed.)
- (00:35) m4_landfirst.patch (48 lines on top of m3; bc_m4min / build_m4min) = build_dc12e on 12 pinned games with landfirst
  (IDENTICAL).
- (00:40) Where the true opponent flow pays (rung 1, M&M days 12-17, m1 keys; probe DC12_ORACLE_WHERE; fixed a dangling else in
  the oracle-mix probe: without DC12_ORACLE_HOURS the mix modes silently ran the full oracle): full oracle +118 / day (SE 16);
  true flow in the dawn plan only +43 (SE 14); in the hourly executor only +47 (SE 8). Half would need a mid-day re-plan with
  the intraday forecast (routes / deposits); not built: BC's true-label oracle on 440 pinned full games was -142 (SE 344), so
  one-day rung-1 gains from forecasts do not carry to full games.
- (00:50) landfirst BUG (fixed): the landfirst plan kept fallback KeepAll, so the agent's herd-reach / earlycrop tries (bigger
  intents) accepted it as "complete" with all their new entities dropped, replacing the day's funded plan (pinned d3crop line
  -287, SE 158: those reads are void). Fix: the plan is marked fallback NoNewEntities. m4 patch / bc_m4min / build_dc12e /
  build_dc12h rebuilt; pinned rerun. Imitation's pre-fix league (96 games per line): copy +0.09k, hybrid -0.05k, d3crop +0.39k.
- (00:50) Seed 504 (copy + m1 vs E, land 3): the network's land head, not the ladder (probe DC12_LANDLOG). Without m1 dawn cash
  d6 $782 and land logit +13.4 (land d6 / d8 / d10); with m1 the d2-d5 animal asks are funded, dawn cash d6 $32, logit -2.5 (the
  network reads money and does not ask); d7 the land alone is unfunded; Q2 d8, Q3 d10, no Q4. Not fixable in the ladder.
- (00:50) Pinned 200, base lines with m1 (vs the d3crop base): hybp +6,087 (trimmed +3,504), d3crop_m68 +3,751 (+2,615), copy
  +3,106 (+977); copy vs hybp -2,981 (SE 570). Imitation: hyb_nolp (= hybp's line) was weaker than d3crop live (Kaggle 56665619:
  4 / 16 wins vs the top 10) although clone / pinned beds favoured it; consensus candidate stays d3crop_m68 + m1 + sf.
- (00:55) LIVE: BC built packages/d3m68_m3 (d3crop_m68 + dropany=6 cashsell=1 wheatcash=1 reserve=0 saleslots=3 nighttrim=1
  survivalfloor=1; bridge from bc_m3min); Imitation submitted it on the user's approval (their session) as Kaggle 56690263
  (23:46 UTC); it retires hyb_melon68. BC's private kernel check of the same archive: 4 / 4 done, rewards = local, worst call
  2.67 s, overage left >= 46.9 s. landfirst is not in it.
- (01:05) Land timing on the live line (pinned live29, 75 games, DC11_COLLECTLOG quadrants per dawn; tools/land_days.py):
  d3crop_m68 vs + m1: Q2 by dawn 7 in 100% / 100%, Q3 by dawn 9 in 99% / 84% (the rest dawn 10), Q4 by dawn 11 in 100% / 100%.
  The land cascade does not reach the live package; landfirst matters for the copy / hybrid lines only.
- (00:58) collectall=1 (+ nighttrim) on pinned live29 (hybp + m1 + nt): -2,556 (SE 358), own -4,188, 75 / 75 games changed.
  Collecting every holder every day costs labour and floods our own sales; the network's batched collection is better. Closed
  (run stopped before live28).
- (00:58) Fixed landfirst on the live d3crop line, pinned live29: 0 of 75 games change (the main intent never reaches an
  unfunded land day on this line). It matters only for the copy / hybrid lines (Imitation's league running).
- (01:10) Fixed landfirst judged: pinned 200 on the live d3crop line 0 / 200 games change; Imitation's league (96 per line, vs all
  8 local agents): d3crop identical 96 / 96, hybrid + m1 +0.36k (SE 0.18k; vs E / D +1.42k; seed 505 fixed, no land < 4), copy
  +0.09k (SE 0.05k; seed 504 is the network's land head). Clean: m4_landfirst.patch can join the next package (inert on d3crop).
- (01:10) Weaknesses tracks the live 56690263 (live/m1_live.py vs ranks 1-30; baselines d3crop 25% wins vs ranks 1-10, 79% vs
  11-30). Read when it has 10+ games vs ranks 1-30 (~2.5-3 h).
- (01:20) Perfect forecast in full games (pinned 200, live d3crop_m68 + m1, REPLAY_ORACLE = the pinned opponent's true hourly
  flow): +2,792 (SE 1,292), median +1,789, trimmed +2,000, 72% up. Matches BC's Sep 29 dc11 probe (+8.8k, trimmed +4.5k;
  seller-only +5.1k, dawn-compile-only +8.1k). BC: real opponents are price-responsive, so pinned oracles overstate; no forecaster
  change has converted it on reacting beds (sharpen, retrains). The plan-only / executor-only split duplicated BC's: stopped.
- (01:30) Live package on Weaknesses' swap bed (our arm in the top team's seat vs our real sub, 234 exact games): d3m68_m3 vs
  d3m68 ranks 1-10 -1,386 (SE 657; own +335, our sub +1,721), 11-30 +320. Per product (tools/swap_products.py), ranks 1-10: the
  arm moves strawberry from h21-23 (-12.8 units / game) to h0-2 (+8.8) and h12-20 (+7.5); the sub sells +15.2 more at h21-23 and
  earns +1.0k on strawberry, +0.7k on milk. So a higher dawn share alone does not deny a follower; leaving its evening does the
  opposite. Same reading as m1's wide-bed -1.24k (a follower bed; live pool is 94% dawn sellers). The live A/B decides.
- (01:30) Imitation's proposal (sell like M&M: (a) the field takes each drain, (b) keep the dawn plan for carried stock). Both
  exist as keys: (a) = seller v0 race=1 (18:30: evening 34 -> 28, dawn 27 -> 31 units / day, duels -2.8k); (b) = BC's regime-M
  seller (regime=1 regimehold regimewait regimedawn). Sent both for their fidelity gate on the same-stock bed. At h21 the quote
  (137.7) beats the hold value (127.7), so a commitment alone does not create M&M's evening hold; any key that does gives up own
  price and can pay only through the follower's reaction.
- (01:32) New key rivalnight=W (dc12 dev tree, market.cpp night_cost / both hold values / evaluate). The market is a
  fixed-drain queue: a unit sold stays in the book until drained and lowers every later opponent sale. The rival term stopped at
  h23, so the opponent's forecast sales from tonight to the hold step were not credited to units sold today, and holding looked
  free of denial loss. Pinned 200 (package line + rivalnight=1, ref pe_d3m68m1lf = package) running (3 threads; machine loaded).
- (01:36) rivalnight's premise checked on live data (tools/dawn_react.py; our live subs pkrl / pkhyb / pkcma / pkm68 vs ranks
  1-30, 183 games, days 10-27, 7,679 opponent game-days): the opponent's share of its dawn stock sold at h0-2 does not fall after
  evenings when we sold more (within-game slope per unit of our h21-23 sales on d-1: milk -0.003, strawberry +0.009, wool +0.007;
  raw bins rise with our evening units, a shared-pulse confound). The field's dawn flow does not react to our evening sales, so
  the denial credit on it is real for these opponents (the pinned bed, which fixes their flow, is the right judge here).
- (01:42) rivalnight=1 on pinned live29 (75 games, vs the package, ref pe_d3m68m1lf): margin -553 (SE 1,278), median -783, 35% up;
  own -1,323, opponent -770. The denial is real but costs more own money than it takes; the model predicted a gain, so its
  forecast of the opponent's night / dawn units or their price impact is too high. CLOSED at W=1 (live28 and the M&M-bed runs
  stopped). Stop list.
- (01:47) Land hour (Imitation: M&M buys land d6 h5 / d8 h7 / d10 h10-12 and plants the quadrant the same day; our package later,
  fewer same-day plantings). Rung 1 (M&M dawn state + M&M intent, 40 games, days 6-10, package options; tools/land_hour.py): median
  land hour ours vs M&M Q2 h5 vs h4, Q3 h8 vs h6, Q4 h10.5 vs h9.5 (our tail h14-21). Same-day plantings 17.7 vs 20.2, but M&M's
  intent label itself has 18.6 new crops (the label misses same-day replants), so our own drop is ~0.9. Cause of the hour: the
  land's first affordable hour is simulated on the plain plan, routed with land at h0 (its crew plants, it deposits late; Q2: M&M
  sells $1,247 before h4, we $203), and the seller then raises cash only by that hour. Not the stress forecast (stress=23: no
  change). Key landpull=N (re-derive the hour on the funded plan, pull earlier if still funded): 48 pulls, hours -0.1 to -0.4,
  plantings identical, rung 1 +2.4 / day (SE 5.3). The land hour does not limit the day's plantings: CLOSED.
- (01:48) Options buffer: the bridge and tools read model.bin.dc11 into char[256] (Imitation); a line over 255 chars aborts the
  parser (a Kaggle crash). Live package 205 bytes. BC's step 67 (whole line) applied to bc_m3min, bc_m4min, bc_dc12h, bc_dc12;
  duel_mm / duel_dc11 buffers 4096 in bc_dc12 / bc_dc12h.
- (01:48) Running: pinned live29 on one thread with DC11_INTENTLOG (+ land ask, land hour, quads) to find why Q3 is bought after
  dawn 9 in 16% of package games (the ladder never drops it: NoLand 0 on day 8).
- (01:56) Q3 slip on the live package (pinned live29, one thread per chunk, DC11_INTENTLOG + land ask / land hour / quads;
  tools/land_asks.py): 12 of 75 games (16%) get the network's first Q3 ask on day 9, never day 8; the ladder never drops an asked
  land (Q2 / Q4: 75 / 75 on the ask day). Dawn money d8 $258 in slip games vs $568 (d6 / d7 equal: $700 / $240): day 7's spend-down
  under reserve=0 leaves the land head (money is an input) below its ask. Package minus the no-m1 base: slip games +2.9k mean /
  -0.3k median vs +3.6k / +1.6k on time: ~1-2k per slip, ~0.2-0.3k per game. Test: decode land_push 8 8 10 (the day-8 ask forced
  when missing; only slip games can change), pinned live29 running.
- (01:56) Imitation, same-stock bed (M&M's farm, our seller vs M&M's own selling, 24 worlds): base -1.10k; race=1 -7.96k (dumps at
  night); regime w0.01 -1.02k (evening share = M&M's, dawn overshoots, lower price per unit); w0.03 -2.88k. Hour shares alone carry no
  margin; lot size / price per unit next (Imitation).
- (02:08) Decode land_push 8 8 10 (BC's key: forces the day-8 Q3 ask when missing) on pinned live29: +86 (SE 137), 63 games
  changed; slip games +382 mean / +966 median (n 12), on-time +30 / 0. It recovers part of the slip. live28 running for power.
- (02:08) Imitation's gate-1 screen (sep29_mm_copy/runs/gates/g1.sh; teacher_day on M&M's label intents, pass / fail per day
  block): package 56 / 89 (20 games). Passes production, land, services, next-dawn value (+88..+244 / day vs M&M). Fails: sale
  shares, units per day (eggs 1.4-3.4x, wool 1.25x: we sell stock through, M&M carries), waterings 0.81 / 0.89, dropped stops
  0.38 / day on d18-24. Waterings: ongoing crops gain yield only from the fertilized day-before-production water, which dc11 always
  serves (P_OUTPUT, 10,000 if unserved); M&M's other daily waterings add no yield; asked Imitation to score yield waterings only.
- (02:12) Gate-1 "dropped stops" on d18-24 (rung 1, 40 games x 7 days, package; reason codes t<tile>p<priority>o<op>): per
  day-compile optional fertilizer collections 0.22 (P_EXTRA), new plantings 0.14, required output stops 0.17 (water 0.06, harvest
  0.05, feed 0.04, fertilize 0.02), unfinished routes 0.01. ~$60-120 / game in total: small. Imitation: gate 1 now scores yield
  waterings; sale timing stays pass / fail by the user's request ("similar decisions, especially sales timing"), paired with price
  on the same-stock bed; Gate 2: land-day crop asks are low (d10: v219 sets buy_land after the crop decode) - the network's.
- (02:10) Why M&M's dawn price is higher at the same hours (Imitation's same-stock bed, 24 worlds; tools/sell_price.py prices each
  lot as the engine interleaves it; tools/price_table.csv from the engine): per unit over the day, days 12-24, M&M's own selling vs
  our base: milk 110.1 vs 110.5, strawberry 138.5 vs 137.7, wool 134.2 vs 142.1 (ours not worse). By band they mirror: M&M dawn milk
  127.9 / evening 87.0, ours 103.5 / 110.5; inventory before the dawn milk sale 10,012 vs 10,024: M&M sells little in the evening,
  the book drains overnight, its dawn sells high. regime w0.01: dawn strawberry 147.9 but h3-11 83.8 (its big dawn lots keep the
  book high), day average 132.9. The margin gap on this bed (thin products): our revenue +0.75k, the sub's +1.18k (days 12-24 milk
  +0.46k, wool +0.59k) with equal totals made / sold: the sub shifts units into days 12-24 against our seller (prices fall ~40%
  late). Denial through the sub's day timing, not our price.
- (02:20) Gate 1 on my build (build_dc12e; teacher_day ported from Imitation with water_yield; 20 games, d2-24): package 59 / 93.
  New key fieldflow=f (market.cpp field_flow; the thin-product opponent forecast = f x each drain, sold the hour after it; in
  day_market, the Executor and the ported seller_hour): f=1.0 53 / 93, next-dawn value vs M&M d12-17 +46 (package +200), d18-24
  -388 (+244); f=0.7 58 / 93, +125 / -38. Evening shares fall (milk d12-17 +33 -> +4 pts) but dawn shares overshoot on d18-24 (+17
  to +32 pts) and units / day rise. Against the recorded (fixed) opponent the "field fills each drain" forecast is wrong and costs
  $75-630 / day. Same-stock bed (reacting sub) for f=0.7 vs the package running (runs/sf_ff.sh), the one bed where M&M's timing paid.
- (02:20) Gate 1 water_yield 0.94 on d6-24 (a real yield gap if it holds). Candidate artifact: the metric counts every fertilized
  day-before-production water, while dc11 skips it when the held yield would cap the gain (held + 2 > 4). To check.
- (02:22) Sep 30 M&M imitation plan (user-approved, Imitation coordinates; work/mm_handoff/PLAN.md, GATES.md, DEVIATIONS.md): I own the
  compiler items D3 (early funding), D4 (seller), D5 (drops / weeds d18-24), D6 (Q3 slip, with BC), D7 (yield waterings). Gates: G1
  (Imitation's teacher_day scorecard on M&M's label intents, sep29_mm_copy/runs/gates/g1.sh; my build work/sep29_fund/build_dc12e has
  their teacher_day + water_yield split), G1s (same-stock bed vs rec_s).
- (02:28) D4 fieldflow: G1s f=0.7 -4.54k vs rec_s (own -3.5k): closed. M&M's own selling (rec_s, d12-24) is a tick seller: ~1 unit per
  product in the hour after each drain (milk h1 1.6, h5 0.4, h9 1.0, h13 1.3, h17 1.2, h21 1.2), about a third of the drain, more with
  stock, less when the book is above the day's mean; a dawn lot of ~1/4 of the overnight strawberries.
- (02:36) D4 hourdisc (plain DP wait cost), G1 d12-24: evening shares reach M&M's for milk / wool at 0.99-0.995 but strawberry dawn
  overshoots and eggs invert (M&M sells eggs in the evening); next-dawn value +200 -> +70..109 / day. G1s: 0.995 -1.22k, 0.99 -1.15k
  (base -1.10k): 0.99 denies the sub 0.8k but loses 0.85k own. It front-loads at h1 and back-loads at h17-21 where M&M spreads evenly.
  Not a pass. Probe keys ticksell / tickdawn (M&M's rule copied) built, screen pending. I1 (BC's learned M&M sell policy) hook planned.
- (02:36) D7: yield waters d18-24 31.9 vs 34.0 / day (one-shot 21.2 vs 22.0, ongoing 10.7 vs 12.0, 0.4 of M&M's capped), but field
  units next dawn 1.001 of M&M's and harvests equal: no unit loss visible. Low priority.
- (02:45) D3 early funding rebuilt as key achieve=1 (dc11 compile_level; implies skipwater): the ladder evaluates the plain plan and every
  deferral variant (the executor's skipped plantings counted in the funding sim, Funding::skipped_seed) and keeps the one achieving
  most (animal value, then fewest skipped seeds, then fewest drops) instead of the first stress-funded one. G1 d2-5, 60 games,
  copy_pure keys: base plantings 0.804 / fallback 34.6% / 12 of 15; + achieve reserve=0: 0.827 / 1.7% / 14 of 15; + hourdisc=0.99:
  0.877 / 2.5%. seedlag (seeds 4-8 h after animals) closed: workers wait for seeds, survival waters slip past the day end, weeds +1.0 /
  day. reserve=0 needed: the next-dawn reserve rejected the dawn-wheat variants by $2-8 (M&M's day-3 cash: 7 dawn wheat, $198).
  Remaining: day 3 (29 / 60 the best variant skips 1-2 seeds for the cow; 15 / 60 sim-funded but the live day plants fewer).
- (02:55) D3 funding, second root cause: the router's feed-wheat accounting. Feeding harvested wheat forfeits its deposit value, but
  wheat picked up at the shed was free, so on wheatcash days (dawn wheat sold) routes fed early with BOUGHT wheat and sold the harvest
  (traced day 3: 5 wheat, $141, bought before the harvests; M&M feeds after its h7-10 harvests and buys none). Key feedcost=1: a shed
  wheat pickup costs its price on days whose shed wheat is bought (router.cpp pickups, Problem::wheat_buy_cost). G1 d2-5, 216 games,
  copy_pure keys: base plantings 0.806 / fallback 39.9% / cows -0.05; + achieve reserve=0 feedcost=1: 0.954 / 2.3% / -0.02 (13 of 15);
  + hourdisc=0.99: 0.967 / 1.6% (14 of 15). d6-24 (40 games): 46 -> 49 of 78, plantings pass in every block, drops d18-24 0.43 -> 0.14,
  next-dawn value d12-24 unchanged (hourdisc: +143 -> +18). Handed to Imitation: m5_sep30_funding.patch (all keys off by default).
- (03:00) G1s bug (Imitation): our seller's lot was capped at the shed before this hour's deposits (sellable the same hour); fixed in
  my duel_mm. Package seller vs M&M's own selling on M&M's stock: -0.38k (SE 0.56k) (was -1.10k). Tick rules (ticksell 0.3 / 0.5,
  tickdawn 0.25, tickegg 0.5: M&M's clock copied) -4.85k / -5.27k (own -5.3k): closed. I1 hook built (sellmodel=1 +
  <model>.sellmodel; m5_sep30_i1_hook.patch), BC's v1 running on G1 d12-24 and G1s.
- (03:10) I1 (BC's learned M&M seller, sellmodel_v1, sampled) on G1s (fixed bed): -3.08k (SE 0.46k) vs rec_s, own -3.43k, sub -0.35k.
  Hours match M&M's (after-drain lots), but thin-product revenue vs the DP seller: d0-11 -0.52k, d12-24 -0.94k, d25-29 -1.90k (holds
  stock into the last day: strawberry 5.2 at d28 h23 vs DP 0.7). Sent to BC (end-game cap, expected value vs sample).
- (03:20) D6 closed: decode land_push 8 8 10 on pinned 200: +3 (SE 109), median 0.
- (03:20) Integration (Imitation, src_dc12i = their tree + m5 patches): copy_pure + achieve reserve=0 feedcost passes G1 d2-5 as mine
  but loses in games: 6 exact worlds whole game -2.30k (SE 1.47k), from the day-12 hand-over -1.47k; the sub earns +2k. Ablations
  (achieve+feedcost / reserve=0 / achieve+reserve=0) running on Imitation's side. Note: feedcost also acts mid-game (any dawn with no
  shed wheat), not only on wheatcash days.
- (03:20) D10 deposit timing (M&M deposits animal products at h21-23 and dawn; ours mid-day). The router's deposit value per unit by
  hour (DC12_GAINLOG, M&M days 12-17): milk h0 27.2 / h8 16.5 / h16 10.0 / h22 7.1, wool 33.9 / 19.8 / 13.1 / 9.0: early deposits
  are worth $15-25 / unit more under our DP, so mid-day collection is value-driven. Screening regime=1 regimecarry=224 regimeeve=18 /
  21 (BC's evening-carry routing without its seller) on G1 d12-24.
- (03:20) I1: threshold decode added (sellmodel=2, BC's v2t; sellmodellast=D cap). G1s running: v2t threshold to day 29 / days <= 27,
  v2 sample to day 29.
- (03:25) reserve=0 root cause (Imitation's ablation: reserve=0 alone -4.99k in games; achieve+feedcost -1.11k, noise): "reserve"
  switches off the only use of dawn_reserve, the funding check's next-dawn reserve ((animals - shed wheat) x wheat price x 1.2 + 30,
  ~$900 mid-game). Without it days end without tomorrow's feed money and the next dawn fire-sells thin products (the m1 lesson).
  Day 3 only needed the $30 slack gone. Key reservenet=1: reserve = max(0, animals - own wheat) x price x 1.2, own wheat = shed +
  pockets + wheat harvestable tomorrow (M&M feeds from its harvest), no slack. G1 d2-5 (216), copy_pure +: achieve feedcost 0.903 /
  4.1%; + reservek=0 0.939 / 3.6%; + reservenet 0.953 / 2.5% (= reserve=0's 0.954 / 2.3%). Candidate copy_pure + achieve=1
  feedcost=1 reservenet=1; patch m6_sep30_reservenet_sellmodel.patch; Imitation runs G3 paired vs copy_pure_af.
- (03:25) I1 on G1s vs rec_s: v2 sampled to d29 -2.65k; v2t threshold to d29 -1.53k, to d27 -1.31k; days 12-24 only -2.13k, days 6-24
  -1.51k; DP -0.38k. No I1 variant beats the DP. Built sellmodel=3 (model hours, DP's forced lot >= 1) and 4 (DP lots masked to
  the model's hours) for BC; G1s running.
- (03:25) D10 hourly (M&M days 12-17, 12 games): M&M deposits 25% of the day's units at h2 (collect at h1-2 right after the hire
  wave); ours 8-9% at h2-3 and 14% only via the day-end pocket auto-deposit. Deposit values already favour h0-2, so the route
  structure is the question. Screen: depcredit (deposits by h2 / h3 earn extra) on G1 d12-17 deposit shares.
- (03:25) Fire-sale check (DC12_CASHLOG, live executor only; 6 exact worlds, copy_pure / af / afrn reproduce Imitation's 5,282 /
  4,171 / 2,945): cashsell acts only on days 0-11 (never 12-24); days 0-5 ~50-57 hours per game with an uncoverable purchase at the max
  bonus, selling only 6-12 thin units per game. Not the mid-game loss. afrn vs copy_pure: same land days, level money at dawn 12, then
  a product-mix shift (days 12-24 our spend +1.0k, milk +1.5k, eggs +0.7k, wool / strawberry / tomato -0.4..-0.5k each; the sub's wool
  +1.8k, strawberry +1.1k, tomato +0.9k). Per-world swings +-5k: the 6-world bed is directional; G3-wide decides.
  Built: nofire=1 (no max-bonus / cover fire-sale for uncoverable purchases; G1 d2-5 unchanged), achieve=2 (the variant search only
  when the first funded plan skipped seeds or none was funded; mid-game drop-only days keep their plan).
- (03:25) I1 timing modes on G1s: sellmodel=3 (model hours, DP lot >= 1) -1.10k (own -1.24k, sub -0.15k); sellmodel=4 (DP lots masked
  to model hours) -1.16k (own +0.06k, sub +1.22k); DP -0.38k. I2 grid (hold 0.95-1.05 x hourdisc 1 / 0.995 x rival 1 / 0.5 vs M&M's
  per-hour decisions, seller_diff 201 held-out games): distance 6.69-9.21 vs base 6.76, eggs dominate. The DP's parameters cannot
  make M&M's clock. Closed.
- (03:35) I3 on G1s (reacting follower, vs rec_s): rivalnight=1 -0.63k (own +0.55k, sub +1.18k); rival=2 -0.24k (SE 0.39k; own -0.07k,
  sub +0.17k), +0.14k over the package: noise. No margin objective beats the package seller on this bed. Compile time days 12-17
  (20 games): package / achieve=1 / achieve=2 median ~200 ms, max 300 / 367 / 425 ms: fine mid-game (Imitation's 3.5 s p95 is elsewhere,
  likely land days 6-11).
- (03:45) D10 root cause: on G1 (M&M's own dawn states, so M&M's layout) our collections by hour match M&M's (h2-4 8-16% each); the
  difference is the return: M&M drops at h2 (25% of the day's deposits), our workers carry products along the first trip and drop
  later (14% rides to the day end; our idle time is all at h21-23: 7.5 unit-hours vs 0.3). Deeper search (rounds 12 radius 6): value
  +20 $ / day, drops 0.14 -> 0.10, deposit shares unchanged. In our objective an early drop is worth what our DP seller makes of it
  (milk $27 at h0 vs $22 at h4), about an extra return trip's cost, so the router is near-indifferent. D10 follows the seller (D4).
- (03:55) achieve=3 (variants by value) G1 d2-5 (216): plantings 0.899, fallback 8.0% (achieve=1 0.903 / 4.1%, achieve=2 0.894 / 5.9%).
  Compile times days 6-11 (20 games, one thread): package / achieve=1 / achieve=2 max 1.11 / 1.24 / 1.10 s (land days): no achieve
  penalty here. I1 v4t (BC, in=56) on G1s: -1.42k (own -1.44k, sub level with rec_s), same through day 28 / 29; DP -0.38k.
- (04:05) Farm-side denial (G3 exact worlds, copy_pure vs M&M's recording in the same worlds): production volumes equal (d12-24 milk
  121 vs 117, wool 61 vs 62, strawberry 193 vs 195; eggs +26, tomato -12, melon -16), yet the sub's milk / wool prices are $12-25
  lower against M&M. Within-day collection: M&M collects earlier (d12-24 made by h0-2 / h3-11 / h12-20 / h21-23: milk 0.32 / 0.40 /
  0.24 / 0.04 vs ours 0.19 / 0.33 / 0.38 / 0.09; wool 0.35 / 0.35 / 0.21 / 0.09 vs 0.20 / 0.58 / 0.16 / 0.05; eggs 0.29 vs 0.19 at dawn).
  With the seller alone explaining ~0.7k (G1s), the farm's timing is the bulk of the sub's +4.5k.
- (04:05) Day 0-1 triage (Imitation's network drift item): M&M's day-0 "wheat sales" are a buy-5-at-h0 / resell round trip (+$8);
  our idle day-0 hire; day 1 M&M hires 3 and sells 5 fertilizer by collect-drop-sell (we hire 1, sell 3, 2 ride to day end), plantings
  equal. latehire=1 (hires deferred to the cash hour instead of cut): no change on day 1 (crew is the router's cost choice). I1 v5t
  (tracker wired, sellmodel=2 / 5 pure) running on G1s.
- (04:10) I1 v5t (level+cap, tracker) on G1s: -0.56k (SE 0.56k) vs rec_s, level with the DP seller (own -1.58k, sub -1.02k); pure
  model lots (sellmodel=5) -1.74k. cashshadow=1 (deposit bonus 0.5 / 1 x price before the cash-bound hour as achieve variants):
  G1 d2-5 0.903 -> 0.904, no gain (the day's total cash binds, not timing). League (Imitation, vs all 8, paired vs the package):
  copy_pure_af -0.92k, copy_pure_afrn -2.03k (reservenet dropped), copy_or_v5d_af -0.49k. Hypothesis for reservenet's game loss: the
  old reserve's failures trim new entities mid-game (fallback level), cutting some network asks by accident; funding them all shifts
  the mix toward animals and the sub gains (network question).
- (04:15) Reserve-trim hypothesis refuted (6 exact worlds, DC11_INTENTLOG, copy_pure_af own games): reserve failures only on days 0-11
  (1.2 / game per band), none on days 12-29; fallback 0 on every day. reservenet changes days 0-11 only; its mid-game mix shift is the
  network reacting to a different early state. v5t integration handed to Imitation (keys sellmodel=2 + sellmodel_v5t.txt;
  m7_sep30_seller_hooks.patch = delta since m6, all keys off by default).
- (04:15) Day-9 drops (Imitation: 0.85 stops / day on day 9 in own games): in my 6 worlds all day 5-11 drops are day-9 pasture builds
  (p2o14). Example: dawn $226, the intent asks 4 sheep ($2,000) + 7 crops; every early variant is short on the sheep, the chosen F10r
  buys them from h10 and the router fits 1 of 4 pastures. The deferral funds seeds before animals. Waiting for Imitation's 60-world
  logs to check how general.
- (04:20) BUG (mine) in the v5t hook: the funding simulations (funded / afford_hour) copied the options, so they ran the sell model and
  pushed simulated hours into the game's live Tracker, which then reset every hour ("step went backwards"): level inputs garbage in
  live play, compile max 3.0 s. Imitation's full-game v5t read (-3.84k vs the package, -2.7k vs the DP seller) is void; G1s (seller
  path outside simulations) stands. Fixed: simulations use the DP seller and never touch the tracker (m8_sep30_simfix_animalfirst.patch).
- (04:20) Day-9 drops, Imitation's 60 own-game worlds (my build reproduces their copy_or_v5d_af intents exactly after porting BC's step
  68 optrate): day 9 is a big sheep ask (3-6 sheep) funded after the day's seeds; animalfirst=1 drops per game d6 0.08 -> 0.22, d9 0.85
  -> 0.38, d10 0.30 -> 0.22 (d6-11 1.28 -> 0.91). animalfirst=2 (both orders as achieve variants) running. Exact worlds: noise.
- (04:20) Imitation, league vs all 8 on the PACKAGE (d3crop_m68_m3): achieve+feedcost+reservenet (reserve=0 dropped) +1.14k (SE 0.46k);
  reservenet alone +0.57k; achieve+feedcost +0.55k; additive. Exact G3 -3.07k (biased bed); swap / 760 / G3-wide to decide.
- (04:35) animalfirst=2 (both funding orders as achieve variants) on the 60 own-game worlds: d6-11 drops 1.92 / game (base 1.28,
  animalfirst=1 0.90); the animal-first variants win 9 times; early choices change later states (path divergence). animalfirst=1 is
  the only one cutting day-9 drops (0.85 -> 0.38); value needs the league. Line-B package candidate (reserve=0 -> achieve feedcost
  reservenet) on pinned 200 running.
- (04:50) Line-B package candidate (package keys, reserve=0 -> achieve=1 feedcost=1 reservenet=1; m_d3m68m3_lb) on pinned 200 vs the
  package: mean +16.6k is a bed artifact (the pinned opponent collapses in 42 / 200 games: money -33k, ours +40k; package vs d3crop
  base collapses 3%). Opponent-stable games (n 155): +2.28k (SE 0.55k), median +1.34k, own +1.68k, opponent -0.60k; vs the d3crop base
  +4.70k (package +2.71k). Live (to 03:45): 7 games vs ranks 1-30, 5 wins; vs ranks 1-10 2 / 4. Single-key pinned arms running.
- (04:55) Pinned live29 by key (vs the package): achieve+feedcost (reserve=0 kept) collapses 20 / 75, stable +2.12k (SE 0.84k);
  reservenet alone collapses 1, stable -0.34k (SE 0.31k); both 19, stable +3.02k. The collapses start on days 2-3 (both farms near
  broke; the replayed opponent's cash-exact seed / feed buys fail): pinned cannot judge early funding changes. Imitation: the package
  line-B candidate fails the 760 guard (-0.72k, SE 0.20k; own level, opponent +0.81k) and G3-wide quick (-0.66k): cancelled. I1 v5t
  (fixed build) on quick 60: -1.73k vs the DP seller (own -2.8k): M&M's selling pays only with M&M's stock timeline. Next: deposit
  values from the model seller's own decisions (consistent I1 integration).
- (05:00) Deposit values from the model seller (m9_sep30_model_values_closed.patch, reverted): a rollout of the v5t threshold decode
  over the day (synthetic hourly features, drains, forecast opponent flow, a tracker copy), same objective as return_value. On a G1
  day it never sells: the model fires on pockets / units sold today / money (live milk h3: pocket 8, shed 0 -> P(sell) 0.97; the
  rollout's shed-only or deposit-hour-pocket states: 0.08-0.30 vs thresholds ~0.4). A route plan cannot supply those inputs. Bound:
  on M&M's own stock timeline (G1s) v5t is level with the DP seller (-0.56k vs -0.38k), so a consistent timeline can at best reach the
  DP. Closed. D6 (Q3 slip) stays closed: Weaknesses live 18% of games on d9 vs M&M's own 11% (~0.1k / game).
- (05:18) Line-B small (package + achieve=1 feedcost=1, m_d3m68m3af) fails the 760 (Weaknesses interim -1.04k, SE 0.29k; own +0.48k,
  opponent +1.51k; F2R -2.5k). Root cause (runs/af760: the 40 F2R games Weaknesses traced for afrn, paired, intent logs): achieve=1
  ranks funded variants by animal dollars first, then skipped seeds. On day 3 (ask: 4 strawberries + 1 cow, unfundable as asked) it
  keeps the cow and skips ~2.8 strawberry seeds per game (10 / 10 games vs the M&M clone, 3 / 10 vs DSM; also day 6 in 5-7 / 10);
  the package drops the cow's pasture and plants them. Strawberry plantings d3 -1.3 / d4 -0.7 / d5 +0.6 / d6 +0.8 per game, so the
  first wave shrinks: d12-17 strawberry sold 50.4 -> 46.2 (-770), and the opponent's strawberry / wool / milk prices rise (+1.4k)
  at equal units (same channel as afrn's d3-4 strawberries). Screen: achieve=3 (a seed dollar = 4 animal dollars) and feedcost
  alone on the same games.
- (05:32) Line B on the package closed. Weaknesses' final 760 for m_d3m68m3af: -439 (SE 205), own +507, opponent +946 (F2R -1,055).
  On the 40 traced F2R games (dev build identical to build_dc12i, 10 / 10): feedcost alone -2.74k (SE 0.62k), achieve=3 + feedcost
  -3.34k, achieve=1 + feedcost -3.45k. feedcost cuts day-2 feed-wheat buys (-2.75 units in every game) and raises d6-11 unfed
  animal-days (+2.4 / game, +19%). New key feedvalue=1 (a feed stop worth the product it banks / protects instead of a flat $30):
  does not restore feeding (unfed +2.05), so the misses follow the plan's wheat buying, not the stop value; feedvalue alone -1.03k
  (SE 0.68k) with almost no behaviour change = this slice amplifies small perturbations (trust mechanisms, not sizes). Closed.
- (05:46) G1s on all 216 M&M worlds (runs/sf_so216.sh; identity: base2 = dcsf2_base): package seller -102 vs M&M's own selling
  (SE 180; own +1,391, sub +1,494); scenopen +164 over it (SE 171; own -396, sub -560), dawn shares toward M&M's. The 24-world
  -378 was noise: the seller is ~0.1k of the G3-wide gap, the farm ~1.6k. Seller work stopped. Next: M&M-label translation for
  multi-day continuations (does the compiler reproduce M&M's multi-day farm from M&M's asks).
- (05:58) Rung 2 built: teacher_day TEACHER_LABELS=2 translates M&M's later labels onto our groups (per-group rates; tools_dc11
  translate(); 719 / 720 later dawns translated). 5-day continuations on M&M's asks, 60 games (runs/md5.sh, tools/md5_score.py),
  total value ours - M&M at dawn D+5: package d6-10 -299 (SE 104; plants -5.2, geese -0.67), d12-16 +163, d18-22 +928; copy keys
  -165 / -89 / +614. No compounding loss from day 12. Days 6-10: day 10 drops 3.77 / game (TEACHER_PRODUCT=4 melon probe): Q4
  ($4,000) is funded by the ripe day-0/1 melons; M&M harvests ~20 units at h4-6 with several workers and sells ~24 by h11 (land
  h8-12); we harvest one plant every 2-3 h, carry them, sell $0.1-4k by h9, land h11-21. From M&M's exact day-10 state: land h12
  (M&M ~h9), 0.5 drops. Causes: flat melon deposit values (no drain); the deferral credit needs a combined move (harvest several
  melons, then a shed stop) local search does not find; defer_purchases ignores the day's harvests; cashroute's horizon is h0.
  Real games: package asks ~10 crops on day 10 (22 on day 11, drops ~0); copy (intent60_af) 16 crops, drops 0.30: a one-day Q4
  planting delay (M&M's day-10 fill), not a large loss.
- (06:12) Rung 2, compiler keys on M&M's asks (12 days from day 1, 60 games; plantings ours - M&M w/c/t/s/m): package keys
  -4.70 / -0.30 / -0.57 / -0.52 / -0.08 (wheat only, cheap); copy_or_v5d_af keys -3.87 / -0.28 / -0.48 / -1.90 / -1.18. Ablation
  on the copy keys: no achieve+feedcost melon -0.17 / strawberry -2.17; + reserve=0 -0.22 / -1.47; dropany / startcomplete=1 / no
  cropfirst: no change. Span sweep: the af melon gap is in days 1-5 (-1.05 at dawn 6): achieve trades a melon for a strawberry on
  cash-short days; reserve=0 funds both. The copy's live asks are 2 + 1 melons (pre = final): the live deficit is the ask; when BC
  fixes it, the funding keys decide whether it lands. Weaknesses (day mix): M&M's edge is production timing over the game, not
  harvest-day choice: no harvest-selection rule.
- (06:21) League (runs/league.sh, seed block 501, all 8 opponents, 96 games; identity 12 / 12 with Imitation's copy_or_v5d_af):
  the package's funding keys on the copy lose: copy_or_v5d + reserve=0 survivalfloor dropany saleslots nighttrim -1,307 (SE 533;
  own +309, opponent +1,616), + reserve=0 survivalfloor -1,502 (SE 419). Rung 2's early-planting gain on M&M's asks becomes an
  opponent gain in reacting games (as afrn / af on the 760). Rung 2 judges execution fidelity, not funding keys.
- (06:25) Why the package keys lose on the copy (runs/pkf_split: 36 traced games, seat 0, vs d3crop / hyb_nolp / E; -497 / game on
  this subset): saleslots=3 moves ~61 thin units / game from h1 to h0 and the opponent sells +38 units at h6-17 (+4.2k); dropany
  runs 5 fewer hires / game (tomato -7.1, melon -4.9 harvested). Key sets co-evolved with their networks; swapping them is not a lever.
- (06:31) scenopen on the package, league seed block 501 (96 games; pk reproduces Imitation's d3crop_m68_m3 league 12 / 12):
  +479 (SE 410), own -731, opponent -1,211 (G1s direction); by opponent E +3.3k, hyb_nolp -1.2k, hyb_m68 -1.3k. Blocks 507 / 513
  running for power.
- (06:42) scenopen on the package, league 288 games (blocks 501 / 507 / 513: +479 / -111 / -256): +38 (SE 231), own -426,
  opponent -464; negative vs our strongest subs (hyb_nolp -1.5k, hyb_m68 -0.9k, d3crop -0.5k), positive vs older (E +1.3k,
  gio_cma +0.9k). Closed (with G1s +164, SE 171). 760 request to Weaknesses cancelled.
- (06:47) landcash=T (new key + router cash_pass: on the first land shortfall the land goes to hour T, deposits before T earn
  0.5 x price, and the search tries moving a valuable harvest together with a shed stop; BC's audit item 6 rated the day-10 cash
  wall H). Day 10 from M&M's states (60): landcash=10 land h11.0 (base h12.0; games >= h11 40 -> 12), drops 0.50 -> 0.23,
  plantings vs M&M -0.98 -> -0.45, next-dawn value -46 (SE 121); landcash=8 mostly unfundable (h12.2). Day-6 continuations:
  copy keys plants -5.07 -> -3.08 (d10 drops 2.17), package keys no change (land h15 -> h13.3, drops 3.8 -> 4.1). Trace: 2 melon
  plants harvested h5, sold h9, land h10 (M&M h9); later melons still deposited at h23. League arms queued after depcredit.
- (06:48) Code record: m10_sep30_dev_vs_src_dc12i.patch = the dev tree vs Imitation's src_dc12i (it also carries older dev-only keys
  and lacks Imitation's DC11_AUDIT hooks). Today's parts: Options::feed_value / land_cash and their keys (compiler.hpp/.cpp),
  Problem::cash_until + SearchState::cash_pass (router.hpp/.cpp), teacher_day translate() (TEACHER_LABELS=2), TEACHER_PRODUCT and the
  water_gain / melon_* columns. All default off; build_dc12e with them off reproduces build_dc12i (league 12 / 12, F2R 10 / 10).
- (07:01) Small early-deposit credits on the package, league 288 games each: depcredit=0.1 +520 (SE 267; own +29, opponent -491;
  wins 74% vs 70%; blocks +1,345 / -71 / +284), depcredit=0.2 +440 (SE 258; own -69, opponent -509; blocks +528 / -62 / +853).
  depcredit=0.5 was own -2.06k / opponent -2.27k: a small credit keeps the denial without the route cost. Guards requested from
  Weaknesses (760, G3 quick); blocks 519 / 525 queued after the landcash arms.
- (07:13) landcash=10 in the league: package +333 (SE 271, 288 games; own -64, opponent -397; negative only vs hyb_nolp -1.3k /
  hyb_m68 -0.9k), copy +90 (SE 371, block 501). Mildly positive on the package; keep behind depcredit. depcredit=0.1 on M&M's
  d12-17 states (G1, 40 games): animal-product deposits move from h12-20 to h3-11 by ~0.5 unit / product / day (milk 3.6 -> 4.2,
  eggs 3.0 -> 3.8, wool 3.5 -> 3.9 in h3-11); no dawn drops (the credit is flat over h0-8).
- (07:25) Weaknesses' guards for depcredit, G3-wide quick 60 (vs the package): dc1 +52 (SE 963; own +1,069, opponent +1,017), dc2
  +351 (SE 753): level. Firing check (days 10-27): pockets h3-11 fall a little (earlier deposits), but sales move LATER
  (strawberry h0-2 share 0.25 -> 0.17 / 0.20, h18-23 0.37 -> 0.45 / 0.47; milk h18-23 0.33 -> 0.38): the DP holds the earlier stock
  for the evening. No M&M-like morning-sales channel; the 760 decides.
- (07:44) depcredit league over 5 blocks (480 games): 0.1 +359 (SE 197; own -217, opponent -577; blocks +1,345 / -71 / +284 /
  -403 / +640), 0.2 +199 (SE 198). Negative vs our strongest subs (hyb_m68 -0.7k, hyb_nolp -0.2k). Modest; the 760 decides.
- (07:55) depcredit CLOSED: the 760 (Weaknesses, vs m_d3m68m1nt) dc2 -916 (SE 203, 500 games; own -292, opponent +624; F2R -1,324,
  F3R -826, top-clone -392), dc1 -815 (SE 258, 270 games; every sub-bed negative). League +0.36k vs 760 -0.8k: the same bed
  conflict as afrn / af (lineage followers vs faithful clones); the clone opponents gain ~0.6-0.7k. Queued dephour / combined / copy
  arms cancelled.
- (07:55) The dc1 760 stopped at 420 games: -539 (SE 218; own -71, opponent +469). Mechanism (Weaknesses' firing check): early stock
  held for the evening leaves the mid-day book thin, where the price-selective clones sell. Test of "early stock sold early":
  depcredit=0.1 + scenopen=1 (pk_dcso) on the 760 (Weaknesses, then scenopen alone) and in my league after feedvalue.
- (08:00) feedvalue=1 on the package (feed stops valued at the banked / protected product; BC audit item 3), league 288 games:
  +992 (SE 268; own +508, opponent -484), positive vs all 8 opponents (+0.39k to +1.35k; hyb_m68 +1.17k, hyb_nolp +1.15k). The
  760 requested from Weaknesses on a frozen build copy (work/sep29_fund/build_dc12fv = build_dc12e now; feedvalue is new code).
  My league: blocks 519 / 525 and the copy (cp_fv, block 501); the dcso league stopped (its 760 runs at Weaknesses).
- (08:10) feedvalue mechanism: on M&M's own states (G1, 40 games, d6-17) no change (feeds are done anyway). Traced league games
  (runs/fv_split: 36, vs d3crop / hyb_nolp / hyb_m68, seat 0; +531 / game): unfed animal-days fall (d6-11 -1.2, d18-23 -1.8), lost
  bonuses fall (-0.2 / -0.2 / -0.7 per block from d6); our wool revenue +783, eggs +527, melon +216; milk -394 (d6-11 purchases:
  cows -0.5, geese +0.3 per game: new cows lose to the more valuable feeds when the day is tight). Opponent: strawberry -376,
  eggs -158, milk +438, wool +266. Part conceptual (bonuses kept), part herd mix.
- (08:23) 760 interim (Weaknesses): pk_fv -309 (SE 231, 320 games; own +327, opponent +636; F2R -530, F3R -55, top-clone -480),
  undecided, final ~09:10. pk_dcso -1,179 (SE 385, 120 games; own -927): stopped, closed. Packaging prep: m11_feedvalue_for_bc_m3min.patch
  applies to BC's package tree (dry run ok); a patched copy builds (work/sep29_fund/bc_m3fv, build_m3fv); identity check running.
- (08:33) feedvalue, package: league 480 games +747 (SE 204; own +621, opponent -126; positive vs 7 of 8, d3crop -0.18k);
  the 760 final -241 (SE 152; own +49, opponent +290; F2R -351, F3R -133, top-clone -257: every set slightly negative, 1.6 SE).
  Copy (cp_fv, block 501): -517 (SE 276). Package-tree build bc_m3fv (m11 patch on BC's bc_m3min) = dev build 12 / 12 with the key
  off and on. Tie-breaker: Weaknesses' seat-swap bed (234 exact live games vs ranks 1-30), running.
- (08:35) entityvalue=1 (new key, BC audit item 5): a new animal is worth its expected net production to day 29 (productions from its
  first-yield age at its interval x 1.8 units x price - a wheat a day), at least its cost (e.g. a day-10 cow ~$830 vs $400). With
  feedvalue the feeds rose while new animals stayed at cost, so tight days cut new cows (the traced -0.5 cows). League: pk_fvev
  and pk_ev, blocks 501 / 507 / 513.
- (08:47) pk_fv seat-swap bed (Weaknesses, 234 exact live games vs ranks 1-30, build_dc12fv identity 3 / 3): all +430 (SE 272;
  own -136, the real sub -566); ranks 1-10 (n 70) +1,294 (SE 484; the real top team -1,300); ranks 11-30 +61 (SE 325); positive vs
  Mother-Goose +3.1k, Vadim +4.7k, DECEM +1.9k, DSM +1.1k. Row: league +747 (SE 204, 480), swap +430 (top-10 +1,294), 760 -241
  (SE 152), G3-wide quick running. (v18 had top-10 swap +1.85k but failed G3 quick; afrn / af ~+1.05k but failed the 760.)
- (09:01) Task (d) from Imitation (dawn-race execution). M&M's dawn slots (tools/slot_dump + slot_sum.py, 216 games, d10-27): h0 ~10
  orders, sells always in slot 0 (1.4 orders, ~15 units), then 8.5 hires; h1 1.3 sells first, 3.5 hires. Our executor: `slots = 10
  - plan orders`: hires / purchases are guaranteed and sales get the leftover slots (placed first); a 10-hire first wave leaves
  no h0 sale. saleslots=3 (package) shrinks the first wave at compile time; the copy keys lack it. Probe DC12_SLOTLOG (wanted vs
  placed dawn sells) running in full games vs d3crop: copy + v5t, copy + DP, package (runs/slots).
- (09:09) entityvalue closed: alone -206 (SE 217) vs the package; with feedvalue -327 (SE 183) vs feedvalue alone (288 games). pk_fv stays
  the candidate. Task (d) probe (DC12_SLOTLOG, 12 games each vs d3crop, seat 0, days 10+): the h0 sells the seller wants are cut by the
  hire wave on 89% (copy + v5t: 26.8 units wanted, 2.2 placed), 86% (copy + DP: 24.5 / 2.0) and 77% (package, saleslots=3: 18.5 /
  6.9) of dawns; h1 sells all placed. Executor-level hire deferral is not possible (planned unit actions are index / position bound),
  so the fix is compile-time: saleslots=4 = saleslots=2 (the first wave always leaves the dawn sells their slots) with the dawn
  decision of the seller that sells (the learned model when on). League screen block 501: cp_v5t vs cp_v5t_ss4, cp_ss2 vs cp_af.
- (09:21) (d) saleslots=4 (m12_saleslots4.patch, applies to Imitation's src_dc12i): league block 501 (96): cp_v5t_ss4 vs cp_v5t +959
  (SE 263; own +330, opponent -629; + vs 7 / 8), but cp_v5t vs cp_af -4,349 (SE 538); cp_ss2 (DP seller + slots) vs cp_af -1,302 (SE
  481). The slot cut is a large part of v5t's loss, not all; for the DP seller reserved dawn slots still cost production. Running: the
  package with ss4 instead of 3 (+/- feedvalue), blocks 501 / 507 / 513; the slot probe with ss4 (wanted vs placed).
- (09:53) (d) thin-split probe: ss4 v1 counted only the model's thin sells, but the executor's h0 list is the model's thin lots raised to
  the DP's when tonight's room binds plus the DP's other products; v1 left plan orders at 9.1. v2 (union count; m12_saleslots4.patch
  = v2, m12b = v1 -> v2 for src_dc12i): copy + v5t + ss4 at h0 plan orders 9.6 / 9.1 / 7.9 (none / v1 / v2), all units placed 2.4 / 8.9
  / 19.2 of ~26, thin placed 0.5 / 2.8 / 5.9 of ~9.5 (thin cut 76% / 70% / 42%). Frozen binaries: work/sep29_fund/build_dc12g. Package:
  saleslots=4 (= 2 for the DP) interim 132 games -267 (SE 311) vs pk, with feedvalue -389 vs pk_fv. BC's copy gaps (d2 strawberry
  trim, d8 land-day drops of up to 18 new entities) are the reserve + late-land wall; landcash is the matching fix (next).
- (09:54) Package + saleslots=4 (= 2 for the DP) stopped at 192 games (CPU: load 34-43 on 28 cores): vs pk +5 (SE 263), with feedvalue
  vs pk_fv -311 (SE 273). Closed for the package line (dawn slots don't pay the DP seller, as before). Running: ss4 v2 on the copy +
  v5t (league, frozen build_dc12g), BC's own60 bed with the copy keys +/- landcash=8 (day-8 land-day drops).
- (09:58) BC's own60 bed (60 of our worlds, copy keys, M&M's seat vs d3crop, days 2-10; tools/askplant.py): copy_or_v5d_af d8 land h6.2,
  drops 0.78, wheat asked 8.9 / planted 7.9, 17 games short; + landcash=8: land h9.4, drops 0.27, everything asked planted (0 games
  short); d6 +0.2 strawberries; d10 drops 0.17 -> 0.43. The d2 strawberry is planted by this network (BC's sbg trim is its ask).
  League cp_lc8 queued. Weaknesses: pk_fv on the full G3-wide 278: +26 (SE 247): the quick-60 +706 did not hold; row = league +747,
  swap +430 (top-10 +1,294), G3-wide +26, 760 -241.
- (10:08) VOID: BC overwrote sep29_mm_copy/models/copy_or_v5d/model.bin 09:06:30-10:03:02 (restored, sha 83792a8db2a1, read-only); my
  copy arms link to it. Voided and moved to runs/void_0906_1003: the copy league 501 reads (cp_v5t_ss4 vs cp_v5t +959, cp_v5t vs cp_af
  -4,349, cp_ss2 vs cp_af -1,302), all copy slot probes (incl. the v2 / v3 copy numbers and cover = 0) and the own60 landcash=8 d8 read.
  Valid: package probes / leagues, cp_fv / cp_lc10 / cp_af block 501 (before 09:06). Rerun on frozen build_dc12v3 (ss4 v3 = the h0
  estimate adds the dawn pockets): runs/rerun_void.sh. Hash manifests: work/sep29_fund/build_dc12{fv,g,v3}/SHA256.txt, runs/MODEL_SHA256.txt.
- (10:08) Astra pass 1 (codex_ideas/astra/passes/2026-09-30_0905.md), my area: astra-001 TAKE (confirmed): duel_mm's REC path appends the
  arm's thin sells after M&M's remaining orders (and only while < 10), so on G1s our seller sits in slots 9-10 or is cut at h0 while
  M&M's own sells were in slot 0: every G1s seller comparison is biased against ours (fix: prepend, same slot budget; offered to
  Imitation). astra-002 TAKE: the executor credits all wanted sells to cash before the slot cap cuts them; probe DC12_CASHCUT running
  (package / copy + DP / copy + v5t); fix plan = one market-order list fitted to the slots, its cash credited, the same list emitted.
  astra-004 (frozen builds + hashes): done for my builds.
- (10:18) Clean slot probes (restored network, build_dc12v3, runs/slotsR): cp_v5t h0 plan orders 9.7, placed 2.7 / 28.6 units, thin
  0.9 / 10.7 (cut 88% of dawns); cp_v5t_ss4 v3 7.8, 20.7 / 26.2, thin 6.6 / 9.8 (thin cut 38%); cp_af 2.2 / 24.6, thin 0.3 / 7.2; cp_ss2
  17.5 / 23.0, thin 3.8 / 6.8; h0 cover sells 0 everywhere. astra-002 probe (runs/cashcut): kept purchases relying on cut sells' cash in
  177 / 67 / 124 hours (package / copy DP / copy v5t), all d6-11 (land days). Fix slotcash=1 (sells fitted to the slots the plan's
  orders leave before their cash is credited; the funding sims use the same executor). astra-001 ported to my duel_mm (sells first,
  DUEL_REC_ORDER=last, DUEL_REC_IDENTITY). Frozen build_dc12v4 (SHA256.txt). League: pk_sc x3 blocks, cp_sc / cp_v5t_ss4sc block 501, queued.
- (10:20) landcash v2: T applies only when the plan's own first affordable land hour is later than T (or never): it pulls late lands
  earlier and never delays one (v1 with T=8 moved day-6 lands from h6.5 to h8.3 on own60). Frozen build_dc12v5 (SHA256.txt).
  Queued after the slotcash league: own60 d8 for the copy (cp_lc8b), package league pk_lc10b (3 blocks).
- (10:27) VOID 2: copy_or_v5d/model.bin.style was 0 (no M&M pin) 10:03-10:25 after BC's restore (Imitation fixed it). Voided and moved
  to runs/void_1003_1025: the clean copy probes (slotsR), the block-501 copy league (cp_v5t -2,870, cp_v5t_ss4 -2,973 vs cp_af, cp_ss2
  -976; ss4 vs v5t -103), the copy cash-cut probes (package 177 hours stands). Clean: own60 (started after 10:25), the slotcash
  league. Hardened: my arm dirs hold real copies of all small sidecars (only model.bin / members / forecast nets link to read-only
  files). Rerun queued (runs/rerun2.sh) after the slotcash league; landcash v2 queue after it.
- (10:38) Team rules (Imitation, user-approved): max 6 game processes per session; Stage 1 = day-limited screens (G1, probes, own60),
  Stage 2 (league / G3 / swap) only for Stage-1 survivors, run by Imitation; manifests per run (tools/manifest.sh); frozen builds.
  Leagues stopped; Stage-1 queue (runs/stage1_queue.sh: slot probes rerun, own60 landcash v2). slotcash partial league (36-48 games):
  pk_sc -13, cp_sc +68 (inconclusive, handed over). own60 clean (copy keys): d6 strawberries asked 13.2 / planted 11.9 (21 of 60 games
  short), d8 wheat 9.7 / 9.1 (12 short); slotcash leaves them identical (not astra-002). Seed-skip probe (DC12_SEEDLOG, runs/own60seed):
  every short game skips planned strawberry plantings h10-h19 for want of seed = the executor's cash guard trims seed buys (cash -
  cost < $8) on the land day. astra-008 (pockets are zero at a legal dawn): v3 = v2; the v3 probe gain was contamination noise.
  astra-002 amendment: slotcash emits the credited sells first; probe build_dc12v7 checks the invariant (slotcashviol) and prints the
  compile-time h0 estimate (slotest) vs the executor's h0 decision (slotdet). Arm dirs now built with tools/mkarm.sh.
- (10:58) Compute rules (Imitation, user request): every game process through work/runq/slot.sh (18 slots machine-wide; -p hi for
  Stage 1), Stage 2 with seqwatch early stops; my deliverable = one frozen build (ss4 + slotcash + land-day fix) with Stage-1
  numbers. h0 slots: ss4 + slotcash places all kept h0 sells (copy + v5t 22.3 units, thin 8.8 vs ss4 alone 21.0 / 7.0); for the DP
  seller dawn slots don't pay (package ss4 +5; swap ss2 on the copy REJECT -450). Land-day seed cuts: not a live-vs-simulation gap
  (runs/cashpath, runs/seedtrim with build_dc12v9 probes: the accepted plan already cuts $300-1,000 of seeds in the simulation; no
  sellable stock at any live cut). Day-6 flow vs M&M (tools/dayflow.py, runs/d6flow, 60 worlds): copy dawn $502 vs $393, strawberries
  11.9 vs 14.1, next dawn $374 vs $73. Hourly (runs/d6sell): the copy's workers carry collected fertilizer (1.4-3.9 units) and 1.4
  wool in pockets all day, so ~$300-550 reaches the shed only at night, after the evening seed cuts; M&M deposits fertilizer by h11
  and sells it h12-20. Fix under test: shadowskip=1 (build_dc12v10: cashshadow variants with the horizon at the funding simulation's
  first seed cut) vs cashshadow=1 vs the copy on the 21 short worlds (runs/skipshadow, gated -p hi).
- (11:12) Land-day fix: the next-dawn reserve binds on the copy's day 6 (~$500 kept for tomorrow's feed; M&M ends at $73), then late
  cash. From M&M's dawn 6 (runs/recd6, 60 worlds): crops copy 15.75, reserve=0 16.42, + shadowskip=2 16.50, M&M 18.63. Own states
  (21 short worlds): reserveuntil=6 17.65, + shadowskip=2 18.80 (copy 15.45, M&M 19.9). New key reserveuntil=D (build_dc12v13+).
  Probes gated on the live executor (build_dc12v14; astra item 4: my slot / cash-cut counts mixed funding simulations). Running:
  runs/own60ru (full 60 to day 11), runs/recland (days 6 / 8 / 10 from M&M's states, copy / fix / package), runs/slotlive (live-only
  h0 slots). L2: build_l2f (copy of Imitation's src_dc12i + DC12_RIVALFC) reproduces mm_g1s_dp 3 / 3.
- (11:30) L2: rivalfrac=1 (cumulative rounding of the opponent forecast in the margin term; lround per hour dropped 15-60% of the
  afternoon flow) on the G1s bed 48: +801 (SE 241) vs the package seller (opponent -913), level with rivalnight 0.5, not additive.
  Land days on own states (58 worlds, d6 / d8 / d10 crops): copy 16.52 / 12.36 / 15.83, reserveland=1 + shadowskip=2 17.48 / 12.52 /
  15.47 (best), reserveuntil=6 variant loses more on day 10. build_dc12v15 = identity 3 / 3 with Imitation's copy base. Live-only h0
  (30 worlds, d10-14): copy thin 1.6 -> 0.2 placed, + saleslots=4 slotcash=1 1.2 -> 0.9. Combined L1 key screen: runs/own60l1.
- (11:45) L1 hand-over to Imitation: m13 patch on src_dc12i (m13_sep30_landkeys_rivalfrac_on_src_dc12i.patch), build
  work/sep29_fund/build_m13f, copy keys `reserveland=1 survivalfloor=1 shadowskip=2` (identities 3 / 3 both ways). Own states: crops d6
  / d8 / d10 16.62 / 12.48 / 15.87 -> 17.52 / 12.63 / 15.50. Timing +5-18% on land days. rivalfrac: full-game REJECTs (Improve Agent league
  -0.85k, pk_rn05 G3 -0.46k); with the land keys it doubles day-8 seed cuts (seller -> cash -> farm); off.
- (11:56) astra-014: with scen=16 the scenario solves (integer Poisson paths) set 77-79% of our thin product-hours on G1s; the
  deterministic DP (room charge > 0; in the live executor also every hour whose purchases are uncovered) sets the rest, and only there
  can rivalfrac act. At every live seed cut the shed holds no sellable stock (all arms), so the land-day loss with rivalfrac is lower /
  later revenue, not held stock. Running (build_dc12v16-18): deccap (astra-019: uncapped network decode vs site-capped intent, package
  and copy from M&M's dawns 8 / 10), ssk3 (shadowskip=3: K re-routes also get the collect-then-shed move before the first cut,
  fertilizer included), own60cr (cashrival=1: no denial terms while purchases are uncovered, +/- rivalfrac). Gate starts ~1 job / s
  (reported to Imitation), so short screens run slowly.
- (12:40) Closed: cashrival (no change), shadowskip=3 (+0.2 crops from M&M's dawn 6 only). astra-019: the package's day-10 crop ask is
  its network's (site cap ~0); `.decode v219 10 0 0 0 0` forces the Q4 buy after the decode without more crops (12.0 asked vs M&M 19.5;
  tomatoes 0.3 vs 3.8) -> BC. Full-game deposit timing (runs/pockfull): our thin output reaches the shed as early as M&M's; the gap is
  our seller holding dawn stock to h21-23.
- (14:25) Faithful M&M-rule seller (mmfaith; mmrule/fit_table2.py -> dc11/mm_rule_table.hpp in work/sep29_fund/src_l2, build_l2j):
  G1s 48: v1 -15.0k, v2 -5.3k vs M&M's own lots with matched volumes / hour bands; loss = price per unit (wool $128.9 vs $138.6).
  Open-loop the model sells at M&M's prices; closed-loop independent draws hit the wrong books. Closed; DP stays (-0.42k vs M&M there).
- (14:28) Day 10 (runs/t10, teacher_day on M&M's own day-10 intent, 60 worlds): package keys plant 18.10 / 19.20, copy keys 17.58,
  next-dawn value -180 vs M&M; Q4 at h12 vs ~h9 costs ~1 crop, landcash moves it to h11.2 without more crops. Day-10 gaps are the asks.
- (14:47) BUG fix hirecheck=1 (m14_sep30_hirecheck_on_src_dc12i.patch; build_m14f = src_dc12i + m14, build_m1314f = + m13; identities
  9 / 9): funded() passed partial hire waves (first hire's fill read as "want"), so unhired workers' stops vanished silently. Teacher
  replays d6: package 16.17 -> 17.48 with latehire=1, copy + land keys 17.05 -> 18.03 (M&M 18.63); d10 -0.3 / -0.6 crops (+$240 value).
  Full games on own trajectories: 0 partial waves in ~25 games per line (the bug bites cash-starved M&M-like dawns). Running:
  own60hc (own states), g3fc2hc (package + fc2 + hirecheck + latehire, G3 24, Imitation's request).
- (15:02) hirecheck: G3 24 (package + fc2 + hirecheck + latehire vs fc2) +36 (SE 311), 22 / 24 identical; own states ~0; crew invariant
  0 with the fix vs 262-461 phantom-worker actions per land day on M&M's dawns without it. Recommended as a protective fix (m14).
- (15:24) Kaggle-time check for the lead bundle (package + fc2 + hirecheck latehire): 0 dawns past the deadline, overage >= 31 s
  (runs/deadline; sent to BC). astra-031 correction: teacher_day value_next is cash + shed only; full-state pairs (tools/teacher_state.py)
  rank day-6 plantings as the main irreversible miss. Package + achieve=3 shadowskip=2: better from M&M's dawn 6, worse on own states
  (runs/own60a3): closed. m14b (hirecheck + DC12_CREWCHECK) patch and builds handed over; adopted into every line.
- (15:31) Lead (package + fc2) bed analysis (tools/bedpair.py + runs/fc2audit, identity 64 / 64): fc2 lowers the forecast of the
  opponent's dawn sales -> our DP sells more at dawn -> the opponent (our sub) sells later; swap = denial (margin +1.06k, own -0.38k),
  G3 = shared price-level gain on milk / wool (own +1.9k). Forecast bias (dawn over, afternoon under) unchanged.
- (16:11) Forecast vs real opponents, done cleanly (astra-021): new tool tools_dc11/forecast_trace (exact recorded replays, fresh
  agent at each dawn; build_dc12v27): all forecasters ~unbiased vs real opponents; per-hour lround in the DP margin term drops 7-13%;
  fc2 = fc3v = fc3nv, 5-12% lower hourly MAE than big2. Earlier live shortfall (pinned replays) withdrawn: state divergence + audit artifact.
- (16:55) astra-030: crew invariant on the final bundle model (sep29_bc_mm/packages/m3fc2ens_hc, hirecheck=1 latehire=1): 0 violations,
  0 short waves on the 60 teacher worlds, days 6 / 10 (runs/crewbundle); the running pkq_fc2ens judge arm (no fix) 163 / 311 violating
  hours. Correction to 16:11: "fc2 = fc3v = fc3nv" is not established (findings 16:11). Started for Imitation, on Weaknesses' reserved
  part A (25 live pkm1 games after 07:30 UTC): runs/fcconf (forecast_trace d1-28, pkg / fc2 / fc2ens / fc3vens; tools/fcconf_read.py)
  and runs/fcconfpin (pinned money read pkg vs pkq_fc2ens, first-order only, build_m14bf).
- (17:45) Forecaster confirmation on set A (training-excluded, previously scored; runs/fcconf, runs/fcconfpin): learned heads -6.6% hourly
  MAE vs big2 (SE 1.3%), equal among themselves; pinned fc2ens - pkg median +1.9k (first-order). Divergence triage (MAP.md C1 / C2 / C3b,
  user priority 1): duel_mm's DUEL_OPS skipped h23 (fixed: build_m13o, exact build_m13x); C1 = compiler watering policy (yield-neutral,
  changes dry inputs); C2 = network care share (compiler exact on M&M's intent); C3b = network releases at crashed prices (32 / 32
  escapes follow feed-0 asks; compiler exact on M&M's intent). Stage 1: waterdaily (runs/wd), keepfed (runs/kf).
- (19:20) Closed on the copy line (30 g3w worlds, full games, build_m13x; base identical to g3w_cp_fc2 30 / 30): waterdaily -5,450
  (SE 809; required waters raise hires and drops), waterdaily + optlate vs optlate -517 (SE 1,059; optional waters skipped, no state
  change), optlate -614 (SE 1,174), keepfed -325 (SE 182). C1 / C2 / C3b closed on the compiler side; day-6 trims on M&M's intent are
  expected-market-funded fallback plans (runs/cash6).
- (20:15) New exact one-day harness for our live games: teacher_day TEACHER_NET=1 on set A (runs/fcday25, build_dc12v25, tools/dayread.py):
  pkg reproduces all 600 recorded game-days within $1. fc2ens - pkg +738 (SE 311), fc3vens - pkg +589 (SE 194), mostly the opponent's
  money (denial; strawberries h0-2 / h21-23 -> h3-11). M&M's extra d8-11 spend = 65% bought wheat, 30% land (tools_dc11/spend_trace,
  build_dc12v29). Running: oracle-forecast ceiling on the same harness (runs/fcdayor).
- (23:40) Deposit timing (Imitation, opening triage): the copy's forecast sees the opponent's early wool / milk and the router's deposit
  value already prices arrival hour, but wool rides long trips; the hire search's gains saturate at 9 hires; timing=0 / rounds=12 do not
  move deposits; earlydep (compound collect-deposit-first move) barely does. Imitation's free-transfer bound (copy +1.94k, package +3.09k)
  is mostly a compounding chain. Running: package + earlydep / rivalearly (runs/pkopen), + earlyforce (runs/pkforce). One-day forecaster
  reads redone with symmetric assets (runs/sym: fc2ens +574, fc3vens +308, own value); oracle arms rerun (runs/sym2) after an xargs
  wrapper bug (dropped env), not a build defect.
- (17:05 UTC, system clock) Note on times: the entry labels (16:55) to (23:40) above, and the matching "(Sep 30 ~17:40)" to "(~23:55)"
  labels in work/mm_handoff/findings/day_compiler.md, ran ahead of the clock again; they were written between ~15:50 and 17:05 UTC.
  The order is right. Labels from here on come from `date -u`.
- (18:09 UTC) Early delivery (package): splitfert=10 moves early milk / wool by the router's own choice (fixed 96 +308, SE 469); with
  Codex's nearanimals placement ported (build_m14sg..sj): sheep-first 96 +198 (SE 507, both farms richer); M&M's census order
  (nearanimals=5) gates at wool by h5 22.2 (M&M 20.0), milk by h2 5.0 (M&M 6.0); its 96 and the geese-after variant (=6) running.
  Scenario dispersion closed (all variants lower the margin). Forced wool-first / extra-hire probes closed.
- (19:50 UTC) Since 18:09 (details in work/mm_handoff/findings/day_compiler.md 18:39-19:45): splitfert=10 nearanimals=6 (patch m19)
  confirmed on the untouched 122 (+1,237, SE 396; 218: +1,205, SE 283), stacks on the fc3vens bundle (+961, SE 502). Clean tree for
  bundles: work/sep29_fund/src_m19 (= src_m14 + m19; src_m14sf also has the probe keys m20-m22). Intent v2 slim executor (src_v2,
  build_v2a..c) passes rung 1; BC's opening script as binding fields loses on the real-opponent bed (-615 / -202): stopped.
  Dawn seller: evening hold / dawn dump of held stock lose on 2-day live-state continuations (runs/evecut, -189 / -303 per game-day;
  the opponent gains the units' place in the book); the "dawn book for tomorrow's harvest" idea was wrong (linear market). Next: a
  teacher-like share of the dawn stock sold at h0-2 (no evening hold): 2-day harness (build_dc12v38, ds arms) and Imitation's
  hybrid-opponent bed (runs/hybds, build_m14or7ds = build_m14or7 + dawnshare keys, patch m23), after the deadline runs free the slots.
- (21:00 UTC) Local-LB push (user direction via Imitation). Lineage league (runs/lblg, build_m19): saleslots=4 +348 (SE 749) sent to
  Weaknesses' judge; rival=1.5 / hourdisc=0.98 / leader=0.5 negative. Exact C++ LB replay (runs/lbrepro, 55 / 60 official games to the
  dollar): #1's losses vs m14-fc2 / mmpq-v2 are late crops from the network's planting timing (strawberry lifetime vs the game end),
  executed as asked; network-side, sent to BC with input dumps. c2_rec110 / f1_fc2ens on the same seeds within noise. User: end of
  competition, only safe general changes; my decode-bias probes stopped unrun. Details: findings 20:25 / 20:55 UTC.
