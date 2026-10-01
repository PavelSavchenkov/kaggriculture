# the BC session (kaggriculture-8b) findings

Written only by the BC session (kaggriculture-8b). Other sessions read it. Format: `- (time) finding. Number. Source.`

## Findings
- (19:10, BC) The M&M copy network reproduces M&M on M&M's states: held-out decode volume ratios 0.95-1.05 on every field; herd
  labels of all 5 M&M subs match its asks per day band (geese d6-9 5.9 vs 5.9, d10-12 1.8 vs 1.9; cows 2.8 vs 2.8). No data-mix
  issue: all five subs buy the same herd, and all 534 perspectives carry the same conditioning. BC scripts/herd_by_sub.py,
  reports/deep/herd_by_sub.txt; mm_holdout.
- (19:10, BC) CPP5c in pinned M&M worlds (teacher_day from M&M's dawn 1, 87 unseen games): geese at dawns 11 / 13 7.46 / 7.56 vs
  M&M 7.55 / 7.55; cows 7.22 / 7.51 vs 8.11 / 8.22. The goose excess seen in live worlds is not the net's policy on M&M-like states;
  the cow shortfall is the compiler's early-cow drops. BC reports/deep/cpp5c.
- (19:10, BC) Our lineage packages (hyb_nolp, hyb_melon68, d3crop) carry "reach_days 6 14 / reach 0.9 0.8 0.7" (herd reach). In the
  copy system it fired in 14 / 4 / 19 / 28 / 47 of 87 games on days 6-10 and added ~1.3 animals per game, mostly geese. Without it,
  dawn-11 geese 8.18 -> 7.21 (M&M 7.55) and value +$516 (pinned). M&M's network has no such push. BC reports/deep/noreach.
- (19:10, BC) Day-10 ask attribution (DC11_INDUMP input dumps + Python block swaps): the copy's lower day-10 ask in its own states
  (16.5 vs 20.2) is entirely the strawberry groups' dry / can-die inputs (dc11 waters ongoing crops every other day, M&M daily).
  Hiding them restores the ask, but dc11 drops the extra plantings at the Q4 cash wall. Clone bed -959 (SE 352). BC PROGRESS 15:05-15:45.
- (19:10, BC) Day-8 under-planting of our packages (6.1 vs the top teams' 12.8 on the Q3 day): the ensemble averages land-yes and
  land-no crop asks. Each network asks ~12 (M&M main) / ~9 (v17d) only when it itself buys land; the no-land members sw100 / mw2k
  pull the average down. BC reports/deep/members, mm_copy_handoff_sep29/analysis/results/day8_*.
- (19:10, BC) Opponent forecaster: sale_score must start at day 1 (from day 12 the learned forecaster silently switches off; the
  earlier -68/day was the fallback). With it on: DP - M&M's schedule +29/day; oracle +119/day (headroom ~90). Retrains (fresh fc29 /
  fc30, cumulative-shape loss 0.3 / 1.0, thin x3) are all within -2 to +5 of big2_stk_xs (SE ~2). fc_probe on 9 real M&M games:
  we under-predict M&M's afternoon (0.67-0.82x) and over-predict its evening (1.3-1.9x). BC reports/sellfc, reports/fcprobe.
- (19:10, BC) dc11 v94 portable bridge (trees/v94pkg): with dropany off it reproduces the v62 d3crop-m68 bundle's local-verify
  rewards exactly (87,602 / 240,904 / 250,505); with dropany=6 play changes (self-play 90,628), local worst call 0.79 s, overage 60 s.
  Kaggle kernel check running. BC packages/d3m68_v94_drop.
- (18:48, BC) d3crop-m68 + dropany=6 (v94 bridge): Kaggle kernel 4/4, rewards equal local, worst call 2.24 s, overage >= 50.3 s. Weaknesses'
  760: +34 (SE 135), wide +189 (SE 620) -> level, not a candidate; the v94 bridge stays the base for dc12 keys.
- (19:05, BC) CORRECTED (the 18:48 care and fertilized-watering gaps were probe artifacts: teacher_day read water / care / feed from
  tile flags that the night update clears in the h23 step, and dc11 does many of these at h23). With h23 actions counted (87 unseen M&M
  games, one-day continuations): fertilized production nights not watered per game: strawberry M&M 0.6 vs ours 0.1-0.2, tomato 1.8 vs
  0.1; care banks equal (cow young 40.0 vs 40.3, sheep young 25.7 vs 26.4, mature equal); melon waterings 68.3-69.0 vs 69.4 (yield-
  adding 57.4-57.9 vs 57.6). NO execution gap in watering or care. M&M waters ongoing crops daily (strawberry 179 vs our 114 on days
  6-16, tomato 64 vs 45), but the difference adds no yield. BC sep29_bc_mm/reports/prod2, care2.
- (19:05, BC) History (dc10 source/history.hpp) misses harvests of ongoing crops after their LAST production (needs max_lifespan_step
  < 0): on 40 M&M worlds (fc_probe vs the true opponent shed + carried, days 12-24) inferred M&M strawberry stock 3.9 vs true 8.6 (MAE
  4.7); milk / wool fine (MAE 0.3 / 0.2). With the fix 8.3, MAE 0.6. Used by dc11's market model (rival stock), blend, forecaster
  features. Key: patcher step 63, <model>.lateharv (main History only; the forecaster's exact History unchanged). Zero-stock gates on the
  intra-day forecast: the opponent's SHED stock (not shed + carried) is what matters (MAE milk 0.366 -> 0.333, strawberry 0.547 ->
  0.469, wool 0.195 -> 0.173); a gate on true or inferred shed + carried helps little (0.366 -> 0.358). BC sep29_bc_mm/reports/fcstock.
- (18:48, BC) SEED LEAK in the M&M copy: M&M buys seeds the day before it plants them (dawn stock: d2 melon 1.3, d6 strawberry 1.1, d8
  wheat 1.3, d10 wheat 3.4); the copy asks for those crops mostly when the seeds are there (single-slot swaps: d2 melon -1.10 / +0.77,
  d6 strawberry +0.72, d8 wheat +0.44, d10 wheat +0.68; the cash block -0.07). dc11 buys seeds on the planting day, so the copy
  under-asks in our states (d2 melon 0.47 vs 1.52 on M&M's dawn). Fix under test: seed-blind nets (train.py --gblind 22-26 + patcher
  step 62 <model>.gblind): own-dawn asks d2 melon 1.09, d6 strawberry +0.7, d8 total +0.4, d10 +0.5; equal on M&M's dawns. Arms in
  Imitation's league (CPP5c_v3 / _v3sb / _v3sbo) and BC's clone bed. BC sep29_bc_mm/reports/indump_d2, scripts/day_blockswap.py.
- (18:48, BC) New M&M data: M&M's newest subs 56658903 (LB 3089) / 56679033 + new DECEM 4Q games, 180 perspectives (sep29_bc_mm/data/
  mm3, same strict plan filter). m4q_v3_s1 (m4q_v2 recipe + mm3 x2): held-out on the new games 36.64 vs m4q_v2_s1's 37.57.
- (18:48, BC) The copy and our lineage run days 0-5 in DSM's style index (model.bin.opening "6 7", from v12). On M&M's dawns M&M style
  is slightly closer to M&M on d2-3 (d2 wheat 0.90 vs 1.24, M&M 0.78); small. BC sep29_bc_mm/reports/openstyle.
- (19:05, BC + Imitation league) Copy arms vs d3crop / hyb_nolp / giovanni cma (36 games each, paired vs CPP5c): CPP5c with main
  m4q_v3_s1 (new M&M data) +1.63k (SE 0.59k); seed-blind m4q_v3_sb_s1 +0.73k (SE 0.55k, ~-0.9k vs v3); seed-blind + M&M-style opening
  -0.04k. The fresh M&M data is the gain; the seed-blind fix of the day-2 ask does not pay in games (the extra plantings may not be
  fundable). Imitation sep29_mm_copy league.
- (19:15, BC) Q4 tomatoes erased by the ensemble (Weaknesses' d8-10 gap): on 87 M&M dawns the hybrid's M&M main asks tomatoes d8 / d9 /
  d10 1.67 / 1.45 / 3.23 (v3 main 2.52 / 1.96 / 5.01), the E members 0.0-0.2, and the logit-averaged share head keeps 0.08 / 0.35 /
  0.58 (geometric mean). d3crop's main itself asks 0.16 on d8. Key ".decode mainshare 8 10" (patcher step 64: the main's crop mix on
  those days, totals averaged) restores executed tomatoes on M&M's dawns to 2.13 / 1.77 / 1.75 (M&M 2.70 / 1.69 / 5.01). In tests:
  hyb_m68_ms, hyb_v3m68_ms (Imitation league, Weaknesses top-clone, BC clone bed). BC sep29_bc_mm/reports/members_tomato.
- (19:15, BC + Imitation league) Hybrids with the v3 main vs hyb_melon68 (36 games vs d3crop / hyb_nolp / cma): hyb_v3m68 -0.34k (SE
  0.65k), with style 1 on days 6+ -1.15k (SE 0.81k). The fresh M&M data lifts the pure copy only.
- (19:20, BC) Opponent SHED stock is inferable from public data: harvested units follow the harvesting unit's position until it stands
  next to the shed (deposit) or the night; sales take from it first. vs the true shed on 40 M&M worlds (days 12-24): MAE milk 0.43,
  strawberry 0.57, wool 0.24. Gating the intra-day opponent forecast to 0 while the estimate is 0 cuts per-hour MAE milk 0.366 ->
  0.335, strawberry 0.547 -> 0.469, wool 0.195 -> 0.173 (the same as a gate on the TRUE shed). Patcher step 65:
  History::opponent_shed_estimate() + <model>.shedgate. In test (BC clone bed); for dc12's seller. BC sep29_bc_mm/reports/fcstock/report2.txt.
- (20:00, BC + Imitation league) hyb_v3m68_ms (v3 main + mainshare 8 10) vs hyb_melon68, paired: all 132 games +0.30k (SE 0.30k);
  vs the 3 strongest (d3crop, hyb_nolp, cma) 72 games +0.98k (SE 0.42k); head to head -0.53k (SE 0.94k, 12). Clone beds lean negative
  (BC -678 SE 548 n 60; Weaknesses top-clone -448 SE 456 n 80). Local-LB branch pavel/hyb-v3-ms-sep29. Day-6 melons are erased the
  same way (main 1.62 -> averaged 0.48); mainshare 6 12 restores them to M&M's level (1.83 vs 1.62). Step 66 ".decode sharemix" = the
  mixture (probability mean) form. Shed gate on the BC clone bed -1.1k (SE 0.53k, n 30); lateharv alone ~0.
- (20:15, BC) Shed-estimate GATE CLOSED: on the Day compiler's rung-1 bed (one day from M&M's dawn, M&M's intent, 240 days) zeroing the
  opponent's intra-day forecast while its shed estimate is 0 loses: rest of day -62.6 / day (SE 14.3), next 1 h -5.7 (SE 3.9), 2 h -42.4,
  3 h -55.7; BC clone bed -395 (SE 305). Lower MAE, worse decisions (less forecast opponent volume -> our seller holds). lateharv changes
  nothing there (the tf forecaster reads its own exact History). The estimate itself is accurate and may serve as a forecaster input.
- (21:30, BC) Forecasters: on the Day compiler's rung-1 bed over all 216 held-out M&M games (1,296 game-days) the Sep 29 retrain
  c30_cum03pw beats the live big2_stk_xs by +25.3 / day (SE 5.3) (40-game list: +51); variants pw5 +22.1, pwmilk +17.8 (level).
  But in Imitation's league vs reactive own-lineage agents: hyb_melon68 + c30_cum03pw +0.05k (SE 0.42k), hyb_v3m68_ms + it -0.37k
  vs +0.98k with the live forecaster. Pinned gain, no reactive gain (same split as Sep 28). Weaknesses' 760 pending. BC reports/rung1.
- (21:30, BC) Weaknesses top-clone final (240 games): hyb_v3m68_ms -886 (SE 309, own loss), hyb_m68_ms -858 (SE 249, opponent +1.0k).
  The Local-LB candidate hyb_v3m68_ms rests on the league (+0.30k all, +0.98k vs the 3 strongest).
- (Sep 30 01:20, BC) Forecaster c30_cum03pw on hyb_melon68 CLOSED as level: contested bed (dawn-selling clones) +310 (SE 303),
  league +0.05k (SE 0.42k), 760 -481 (SE 146, capped clones), pinned rung 1 +25 / day (SE 5). No harm vs dawn sellers, no clear gain.
- (Sep 30 01:20, BC) d3crop_m68 + m1 / nighttrim / survivalfloor package (packages/d3m68_m3, bridge from bc_m3min) passed the Kaggle
  kernel check (4/4, rewards equal local, rebuilt same, worst call 2.67 s, overage >= 46.9 s); submitted by Imitation as 56690263.
- (Sep 30 00:30, BC + Imitation) hyb_v3m68_ms withdrawn: its league gain was seed-1 luck (v3 seeds 2 / 3 level vs hyb_melon68). The
  v3 data's gain holds for the pure copy only (+0.8-1.5k across 3 seeds).
- (Sep 30 02:30, BC + Imitation league) A 3-seed logit average of the v3 copy (mainweight 1/3 + seeds 2 / 3 as members) loses -1.45k
  (SE 0.43k) vs seed 1 alone, more than any single seed (-0.20 / -0.64k): averaging blurs the plan. CLOSED; keep single-seed mains.
- (Sep 30 02:30, BC, M&M plan) G2 (options_diff on M&M dawns, days 6-17, 60 of mm216): package d3crop_m68_m3 18/33 (new crops 0.72 /
  0.74, tomato 0.12 / 0.43, wheat 0.81 / 0.66); land-day arms lp10solo 18, lp810solo 19; pure M&M copy CPP5c_v3 27/33 (crops 1.03 / 0.99,
  land 99 / 100%, animals +-0.1; fails: tomato 1.10 / 0.79, strawberry d12-17 1.16, fertilize 1.12, options $ diff). D1 / D2 root cause:
  the package's networks are not M&M copies (v17g6ft5 asks 0.16 tomatoes on d8 itself). The copy decides land itself and asks M&M's
  waves. Caveats: mm216 is in the copy's training data (held-out list coming); options_diff values a collect difference at held x price
  (a one-day deferral costs ~0 unless capped). BC sep29_bc_mm/reports/g2_bc_*.txt.
- (Sep 30 04:45, BC, M&M plan) G2 network gate: the M&M copy with ALL decode patches removed (decode "mainweight 1.0", M&M style every day)
  passes every crop / animal / land check (held-out 117 M&M games); the package's networks fail (not M&M copies). Option heads: the per-group
  mode sharpens rates; step 68 ".decode optrate" (expected totals, MAP / largest-remainder allocation) matches M&M's harvest / collect / feed /
  care / watering rates: 70-72 / 73-75 on held-out G2 (copy_or_v4w3 with fresh M&M data 72 / 75).
- (Sep 30 04:45, BC, I1) A learned model of M&M's hourly selling (sell_rows + MLP, 594 games) reproduces M&M's units / sell-hour rate / lot
  sizes / hour shares on held-out games when SAMPLED (the mode under-sells ~50%). M&M sells thin products the hour after each shop drain,
  30-50% of stock per lot. I5: opponent inputs add only 1.6-2.5% likelihood (strawberry / milk / wool), the opponent's previous days ~0 ->
  M&M barely reacts to the opponent; its denial comes from its fixed schedule.
- (Sep 30 03:45, BC, I1 closed loop) On M&M's own recorded farm, the learned seller is within noise of M&M's own selling. The first read
  (-55k/game) was a replay bug.
  - Tool: sep29_bc_mm/trees/net/build/sell_rollout (117 held-out games). It replays M&M's workers, purchases and opponent; only the seat's
    SELL orders come from I1.
  - The seat's cash must equal the recorded cash each step, plus M&M's own sale revenue of that step (M&M sells wheat and buys seeds in the
    same step). With extra cash, orders that failed in the recording (a day-6 land buy, hires) succeed and the farm leaves the recording.
  - Result, I1 minus M&M's own selling per game: v2t -1.63k (SE 1.28k); v4t (+ total shed / pocket stock) -0.85k (SE 1.27k); v5t
    (+ trailing market levels, days 25-29 one-hot) -0.52k (SE 1.37k). Paired v5t - v2t +1.11k (SE 0.67k, 79/117 games better).
  - Teacher-forced, the threshold decode matches M&M's end game hour by hour (d29 h22 eggs 17.3 vs 17.9 per game).
  - The remaining closed-loop gap is production. I1 holds a bit more stock, and a full shed blocks some of M&M's recorded animal purchases
    (a buy needs shed room): 477 vs 500 animal-days per game. Feeding is identical.
  - A 24 h feed reserve (keep the wheat / fertilizer the recorded workers pick up later) made I1 hold until the 100-unit shed cap
    destroyed ~60 wheat per game (-5k). Any reserve on I1's lots must be just-in-time.
  - Day compiler G1s (24 worlds, DP layer): v2t -1.31k, v4t -1.42k (SE 0.60k) vs DP -0.38k. The gap between that bed and this closed loop
    is open.

- (Sep 30 09:45, BC, D11 own-state drift) The copy's asks drift on its own states because it reads two execution artifacts of M&M's play:
  - Seeds: M&M buys seeds the day before planting; our compiler buys them on the planting day.
  - Positions of animals / crops on the own tile grid: the day-0 herd is equal, only the router's placement differs.
  Method: DC11_INDUMP dumps of copy_or_v5d_af's network inputs, live in its own games (duel_mm, quick-60 worlds) and on M&M's recorded
  dawns in the same worlds; block swaps (sep29_bc_mm/scripts/d11_swap.py, reports/d11_dump). Share of the ask gap closed by swapping in
  M&M's block:
  - d1-2 melons (M&M dawns 1.88 vs own 1.29): own-grid animal-type channels 50%, seeds 46%.
  - d3-5 strawberries (1.85 vs 2.58): own grid 71%.
  - d6 geese (2.05 vs 2.61) and cows (1.80 vs 1.40): own-grid animal-type channels 43% each.
  - d12-19 tomatoes and carrots: seeds, plus the opponent's farm summary. The live opponent has 4 quadrants vs 3.25 in M&M's worlds;
    that's opponent shift, not an artifact.
  Fix: retrain v5d with the seed slots blinded (train.py --gblind 22-26, agent step 62 .gblind) and --grid-dropout 0.5, played with the
  grid off (.gridoff "0 29") -> models/copy_or_v5d_af_sbg. Build: scripts/d11_patch.py <tree> adds .gblind; without it the network
  silently sees the seeds.
  Own-state split (quick 60, planted vs M&M's label):
  - melons d1-3 3.0 -> 4.0 (label 4.0); strawberries d6 11.9 -> 14.4 (14.1).
  - d6 geese / cows 2.63 / 1.38 -> 2.07 / 1.77 (M&M 2.15 / 1.78).
  - tomatoes d8-19 17.6 -> 20.4 (19.6).
  - Margin vs the opponent at dawn 24: +1.08k (SE 0.58k).
  G2 held-out (133): 68/71 vs 69/73. Tomato on M&M's own dawns drops to 0.84 on d12-17 (the carried seeds predicted M&M's wave 2).

- (Sep 30 10:40, BC) RETRACTION: "I1 is within noise of M&M's own selling on M&M's farm (-0.52k)" was a replay artifact.
  - Cause (astra-001): my sell_rollout appended the model's sells after M&M's recorded non-sale orders; order position changes quotes.
  - Identity arm "rp" (M&M's own lots through the same replacement path), net cash per game vs the recording:
    - sells last: +1.6k (revenue +3.55k);
    - IN-PLACE (each product's sell in the slot of the recording's first sell of it; cash pinned): +$7 (SE 1), exact.
  - I1 v5t, net cash vs rp through the same path:
    - in-place with the recording's same-step revenue funded: -3.83k (SE 1.16k);
    - sells last: -4.42k;
    - in-place unfunded: -11.3k (M&M's same-step purchases fail when I1 sells less).
  - So the learned seller does not reproduce M&M's money even on M&M's own farm. Tool: sell_rollout ROLLOUT_INPLACE=1 [ROLLOUT_FUND=1].
- (Sep 30 10:40, BC) CORRECTION: blinding seeds / grid does not cost G2 tomato fidelity. On held-out 133 the true v5d also fails tomato
  d6-11 (1.14) and d12-17 (0.85). All blinded nets score 69/71 with the same two fails (sbg 68/71). The earlier "af 69/73, melon 1.23"
  row ran the sbg net without its blinds (see the next item).
- (Sep 30 10:40, BC) INCIDENT: 09:06:30-10:03:02 BST, BC overwrote sep29_mm_copy/models/copy_or_v5d/model.bin (+ .condition /
  .features / .style) with test nets (sb, sbg, sbc, v6, v6w).
  - Cause: arms built with cp -r kept symlinks to that file, and later cp calls wrote through them.
  - Restored (83792a8db2a1); Imitation made its model files read-only.
  - Void: any run of copy_or_v5d* / cp_* / cpk_* arms that loaded in the window.
  - BC's own clean runs: round-1 dumps (09:01-09:05), own60_sbg, round-2 dumps, own60_sbc, G2 sbg / sbc; the G2 af / sb / v6 rows were
    rerun.
  - Lesson: build arms with cp -rL; check `find <arm> -type l` before writing.
- (Sep 30 10:40, BC) net_i identity: net_i (= src_dc12i as of 09:10) reproduces build_dc12j exactly today (3/3 worlds). But build_dc12j
  today does not reproduce its own 04:49 plant60_intentfull games (e.g. 111233 vs 111816). The network, opponent files, traces and env
  are unchanged; the changed runtime input is unknown. Pair only runs made after the restore; BC reran the af baseline on net_i
  (reports/own60_af).
- (Sep 30 10:40, BC) Astra ideas taken / dropped:
  - astra-001: taken; done (above).
  - astra-005 (own-farm-only grid mask): tested and dropped. On v5d, swapping the opponent grid's type channels moves the d12-19 tomato /
    carrot asks by only +0.024 / -0.02 per day (< 0.1 cutoff). On d6 they carry 18-26% of the geese / cow gaps, an opponent-identity
    proxy that the both-farm mask removes.
  - astra-007: applied. mm6 = 40 new perspectives with no G3-wide world and nothing created after 07:30 UTC. mm6s79 re-weights sub
    56679033 (53 G3 worlds already in mm4). Models m4q_v6_sbc_s1 / m4q_v6w_sbc_s1; G2 69/71 each.

- (Sep 30 10:50, BC) Style pin: copy arms must carry .style 1 (M&M). BC's 10:03 restore wrote the training output's .style 0; that
  broke build identity and made v5d fail G2 tomato. With .style 1 restored, net_i reproduces the 04:49 plant60 games exactly.
  Style-1 results:
  - G2 held-out 133: v5d 71/71; v6w_sbc 71/71; sbc 70/71 (tomato d12-17 0.896); sbg 69/71.
  - Own-state split (quick 60, vs plant60_intentfull), planted vs label:
    - melons d1-3 4.0 in all blinded arms (label 4.0, af 3.0);
    - strawberries d6 13.8-14.3 (14.1, af 11.9);
    - tomatoes d12-19 9.5-9.9 (10.2, af 7.4); sbc tomatoes d8-11 9.6 = label;
    - carrots d12-19 still 16.2-16.7 (13.9): opponent effect.
  - Dawn-24 margin vs af: sbg +0.84k, sbc +0.77k, v6w +0.54k (SE 0.5-0.6k).
  - Stage-2 candidates handed to Imitation: copy_or_v5d_af_sbc, copy_or_v6w_sbc.

- (Sep 30 10:49 BST / 09:49 UTC, BC) Remaining own-state gaps on the quick 60 (style 1), split into network (ask - label) and compiler
  (planted - ask):
  - Carrots d12-19 (label 13.9) are the network's: +2.4 to +2.9, compiler about 0. M&M's own 651 games show no general 4Q-opponent
    carrot rule (14.0 vs 13.8), but its newest sub 56679033 plants 15.7 vs 10.3 carrots against 4Q / 3Q opponents. The quick-60
    opponent is always 4Q, so this is partly legitimate newest-M&M behaviour. Low priority.
  - Wheat d8-10 (label 28.3) is split: sbc network -1.3 (d10), compiler -1.4 (land-day drops; Day compiler's landcash fix).
- (Sep 30 10:49 BST, BC) I1 v5t on the corrected bed (in-place, funded, 133 held-out games) loses 3.8k vs M&M's own lots, mostly
  PRODUCTION.
  - It sells ~17% more wheat on d0-11, plus some fertilizer, that M&M's workers later pick up.
  - Milk and wool produced -7% on d0-11; strawberries produced -5% on d12-24 (-1.0k); end-game eggs -0.4k.
  - Just-in-time reserves of recorded pickups (3 / 6 / 12 h) do not fix it (-4.0 / -4.7 / -3.5k).
  - I1's lots ignore input needs that M&M's own selling respects; a revival needs the compiler's feed / fertilizer plan as an input
    or cap. I1 stays paused.

- (Sep 30 11:26 BST / 10:26 UTC, BC) CORRECTION, I1 attribution (astra-012): "I1 sells the wheat / fertilizer M&M's workers use" explains
  my 9-product assay, not the installed hook, which replaces only strawberry / egg / milk / wool.
  - Assay rerun with the four thin products only (sell_rollout ROLLOUT_INPLACE=1 ROLLOUT_PRODUCTS="3 5 6 7", build trees/net/build_r2,
    133 held-out games): I1 vs M&M's own lots through the same path = -1.97k net per game (SE 0.45k). Identity arm +$7.
  - The loss is spread: wool -0.59k (d12-29), end-game eggs -0.38k (d28-29 units 0.78), strawberries d12-24 -0.24k, milk -0.18k.
  - Production is barely changed (0.93-1.00), so for the installed model the loss is sale timing / end game, not input starvation.
  - It does not explain the -3.07k league loss by itself (M&M's farm vs our farm).
- (Sep 30 11:26 BST, BC) CORRECTION, package land days: on its own quick-60 states the package's day-8 / day-10 intent lines show no
  trims and almost no drops (0.28 / 0.00 dropped per game).
  - So "plants the new quadrant a day late" is the network's ask (d8 6.8 / d10 8.6 vs M&M 14.7 / 19.2; next days 12.7 / 23.0),
    not a compiler drop.
  - This matches the Day compiler's recland test from M&M's dawns (package asks 9.7 vs 15.5 on d8, 12.3 vs 19.5 on d10) and the
    old D1: the package's ensemble dilutes land-day asks. Push tests (solo / qday on d8 / d10) are running.

- (Sep 30 11:37 BST / 10:37 UTC, BC) Fallback steering for the package (pk_fv) on its own quick-60 states, days 0-11. Stage 1, day-limited,
  gated. Arms: sep29_bc_mm/models/pk_fv_mm1..mm7; runs reports/own7_* / own11_*; table scripts/pk_traits.py.
  - Zero-cost pushes that fire:
    - .opening "6 1": days 0-5 decode in M&M's style, not DSM's "6 7". Share biases do not reach days 0-5 under the DSM opening.
      d2 melon 0 -> 1.0 (label 1.6).
    - d6 cbias / abias / qday: strawberries 9.9 -> 14.3 (14.1), wheat 5.2 -> 2.5 (2.9), cows 2.68 -> 1.90 (1.78).
    - d8 land_push 8 8 10: day-8 crops 6.7 -> 12.8 (M&M 14.7). This mostly moves plantings a day earlier (astra-016: d9 falls
      12.4 -> 8.0); the extra is mostly wheat.
  - Pushes that do not fire on d10 (8.3-8.6 crops vs M&M 19.2): solo, qday 0.8, landpush + landpushfix. The package's own networks
    plant the 4th quadrant on d11; only a copy-style main network changes that (L1).
  - Open: tomatoes d8-11 (5.4 vs 9.6) and sheep d4-11 +39%. A tomato share push (cbias 2 1.0 8 11, mm7) and an mm3-only base to d11
    (to separate the land push from the opening / herd pushes) are running.

- (Sep 30 11:37 BST / 10:37 UTC, BC) Wording corrections from the Astra reviewers:
  - v1-007: "sbc1 is not a lottery seed" holds only at the recipe level (G2 70/71 for seeds 1-3). Seeds 2 / 3 differ from seed 1 in at
    least one whole-farm count or land decision on ~20% of held-out game-days, so payoff robustness is unresolved. Seed 1 stays the
    preselected arm; s2 / s3 are replication diagnostics only (no seed picking, no averaging).
  - v1-005: I5 means only that explicit opponent inputs add 1.6-2.5% likelihood beyond the shared market book (inventory, price,
    shops), which already reflects the opponent's actions. It does not show M&M ignores the opponent.

- (Sep 30 14:29 BST / 13:29 UTC, BC) Opponent forecaster redesign, first round (user-requested).
  - Audit of the deployed big2_stk_xs:
    - per-product tf model only;
    - --shift 0.3 rotated 30% of sequences' hours against the fixed market cycle, and inconsistently (hours by k, 6-h band
      features by round(k/6) bands);
    - data ended Sep 26; no current M&M subs; non-Kaggle lineage / local / synth rows at x1-x5;
    - rows cover days 1-28 only (day 29 uses the old heuristic).
  - Calibration of big2 on 1,200 held-out real games (Sep 27-30): daily totals are fine (0.96-1.02); the hour profile is off.
    M&M's evening is over-forecast (wool 1.60, milk 1.36, strawberries 1.32) and its afternoon eggs too (1.52). Our own subs'
    dawn is over (milk 1.49, tomatoes 2.2). There's no dawn over-forecast on real top-30 opponents.
  - Live opponent mix (Sep 29-30): ranks 101+ 41%, 31-100 23%, 11-30 24%, 1-10 11%, M&M ~0. Weak teams are NOT over-weighted in
    the training data.
  - Retrained (fc_tf_intra.py unchanged; data fc/d1: all Kaggle ranks + recent games x2, --shift 0):
    - fc1; fc2 = fc1 + --xlag 1 (the cross-product input the C++ runtime already supports).
    - Live-mix Poisson loss of band totals, big2 / fc1 / fc2: dawn -3.71 / -4.27 / -4.32; h6 -1.78 / -2.04 / -2.06;
      h12 -1.88 / -2.09 / -2.10.
    - fc2 bands are 0.89-1.12 almost everywhere (M&M evening wool 1.60 -> 1.11).
  - Exports: sep29_bc_mm/fc/export/fc2.bin (C++ parity 1e-5) -> Imitation for full games. Tools: fc/fc_eval.py, fc_compare.py.

- (Sep 30 14:41 BST / 13:41 UTC, BC) Forecaster, round 1 complete.
  - Clean evaluation: my extra 31-100 / 101+ eval seats shared episodes with training via our own seat (astra).
    - Re-reported on the clean 800 (mm / top10 / r11_30 / ours) plus 71 clean weak-opponent seats from Weaknesses' hold set.
    - fc2 beats big2_stk_xs in every group and at every decision hour. Clean mix dawn -3.69 -> -4.43. Weak clean 31-100
      -3.71 -> -4.37; 101+ -3.91 -> -4.67.
  - Next-day model fc_nx (big2_nx recipe with --next 12, no shift, new data, xlag): tomorrow h0-11 from h18, clean mix
    -1.69 -> -2.12; weak clean -2.21 -> -2.59 / -2.16 -> -2.48.
  - Files: sep29_bc_mm/fc/export/fc2.bin (forecast_tf), fc_nx.bin (forecast_tf_next); C++ parity 1e-5.
  - Full games (Imitation, G3 mixed 24 vs the package): package + fc2 +1.63k (SE 0.93k), own +2.30k; fc1 +0.58k. In judge.sh.

- (Sep 30 15:20 BST / 14:20 UTC, BC) fc2 judge result and training overlap.
  - Judge (Imitation): pk_fc2 PROMOTED on G3 at n 48. At n 56: +1,407 per game (SE 507), own +1,933.
  - fc2's training targets include 99 of the 218 G3 worlds and 109 of the 234 swap worlds (recent Sep 27-30 rows).
  - The margin is the same in untrained G3 worlds: +1,415 (n 35) vs +1,393 (n 21). No leak signal (Weaknesses agrees,
    swap/train_split.py).
  - Rule for the next forecaster: keep G3 worlds (swap/list_g3w.txt) and swap worlds (swap/list_exact*.txt) out of training.
- (Sep 30 15:20 BST, BC) Kaggle-ready bundles for fc2 + hirecheck. Local only: not submitted, no kernel push.
  - A = sep29_bc_mm/packages/m3fc2hc: the live m3 package + fc2 + dc11 `hirecheck=1 latehire=1` (= sep29_dc12/models/pkq_fc2_hcl).
    Archive sha 41be9097.
  - B = packages/m3cmafc2hc: A + 56706309's cma decode. Archive sha b674b267.
  - Bridge: built from work/sep29_fund/src_m14 (the tree the Day compiler tested); bc_m3min has no latehire.
  - Checks:
    - With m3's model files the new bridge reproduces m3's 4 verify games exactly.
    - A and B 4 / 4 DONE and deterministic. The standalone rebuild matches.
    - Worst call, same window: m3 1.56 s, A 1.45 s, B 1.90 s.
    - Kaggle-speed deadline emulation (Day compiler, sep29_dc12/runs/deadline; 24 games vs d3crop, FULL_BUDGET=2.5,
      FULL_DEADLINE=1): 0 dawns past the deadline in every arm; the deadline changed 0 of 24 games.
      | arm | worst step | day-6 / day-10 dawn | overage left mean / min |
      |---|---|---|---|
      | m3 | 4.55 s | 2.07 / 1.79 s | 42.1 / 31.2 s |
      | + fc2 | 5.08 s | 2.31 / 1.93 s | 40.2 / 31.0 s |
      | A (+ fc2 + hirecheck + latehire) | 4.43 s | 2.26 / 2.11 s | 36.9 / 31.7 s |
  - Kernel-check folders prepared, not pushed.
  - Local-LB branch submit/pavel-bc-opus-v17d-dc12m14-fc2-m68 (b38f02e); its agent files are identical to A's.
- (Sep 30 15:20 BST, BC) Converged-field clone, for Weaknesses' judge.sh opponent pool.
  - Model dirs: sep29_bc_mm/models/field_s0 (pooled field style 0) and field_decem (style 3). Network m4q_field_sbc_s1 (sbc recipe
    + data/arrays_field x3), init v17g6ft5.
  - G2 on clean held-out episodes. The first report counted 150 episodes that were in arrays_mm4 / m4q_v2 TRAIN rows (astra-007).
    The init lineage has none.
    - Pooled field, 527 episodes: field_s0 68 / 69 (only fail: d12-17 tomato 0.86) vs the M&M copy 49 / 72.
    - DECEM, 108 episodes: field_decem 71 / 71 vs the copy 53 / 73.
  - The clone sells with our DP seller, not the field's dawn / morning small lots.
  - Build needs .gblind + .gridblind (sep29_bc_mm/trees/net_i/build, or scripts/d11_patch.py on any dc12 tree).
  - Tools: data/field/clean_heldout.py, filter_g2.py; reports/g2_field_clean/.
- (Sep 30 15:45 BST, BC) Is fc2's bed gain from knowing our own sub? G3 and swap both put our sub in the opponent seat, and fc2's
  largest recent group is our subs' games (recent_ours, 919 sequences, weight x2).
  - Offline, dawn head, days 12-27, clean 800 (Poisson loss of band totals; lower is better):
    | opponent group | big2 | fc2 | fc3 (bed worlds out of TRAIN) | fc3n (+ recent_ours out of TRAIN) |
    |---|---|---|---|---|
    | our subs | -3.73 | -5.50 | -5.34 | -4.01 |
    | real-opponent mix | -3.69 | -4.43 | -4.34 | -4.31 |
    Our-sub data barely matters for real opponents. Weak seats agree: 101+ -4.61 / -4.57, 31-100 -4.28 / -4.24 (fc3 / fc3n).
  - fc3 / fc3n picked their checkpoints on val_recent, which holds recent_ours and bed rows (astra 14:30). They are replaced by
    fc3v / fc3nv (fc/d3: selection on the 89 clean val sequences) for Weaknesses' judge vs the field opponent. fc3 / fc3n arms
    were never judged.
  - Weaknesses: pk_fc2 vs the field opponent (field_s0 + v5t seller, not in fc2's data) PROMOTE at 24: n 28, +719 (SE 503),
    own +1,190.
- (Sep 30 15:45 BST, BC) The Day compiler's live audit (milk / wool forecast ~70% of actual vs real opponents, 55-70% at h3-20)
  reads sale.rival_units: the DP's integer units after per-hour lround.
  - fc2's raw means on the same kinds of opponents are calibrated: top 10 / ranks 11-30, milk and wool bands 0.87-1.09.
  - So the shortfall is rounding of flows < 0.5 / h, not the forecaster.
  - rivalfrac=1 (cumulative rounding) was rejected on top of big2, whose evening over-forecast partly cancelled the rounding loss.
    It is worth re-testing on top of fc2 (sent to the Day compiler).
- (Sep 30 16:15 BST, BC) The field clone's tomato flip is the Q4 land ASK, not funding. RETRACTS my earlier funding knife-edge guess.
  - Replays (reports/field2_qual; imf1 duel_mm; DC11_INTENTLOG / DC11_LANDHOUR). In both flip worlds, the side with 3 quadrants never
    asks land on d9-14. landfirst=1 (field2) never fires; field2 = field, so dropped.
  - On the teams' own held-out dawns (G2 clean, 551 games), converged teams ask land on d6 100%, d8 94%, d10 29%. The clone matches
    (d10 0.29). So the field's Q4 buy is a state-dependent yes/no call in ~30% of games. Our play moves the opponent's d10 state
    and so its Q4 call; that is part of any candidate's effect vs this opponent.
- (Sep 30 16:10 BST, BC) Forecaster seed ensemble.
  - fc2 + fc2s1 + fc2s2: same data (fc/d1) and recipe, seeds 0 / 1 / 2. The C++ runtime already averages .forecast_tf, .forecast_tf.2,
    .forecast_tf.3 in the dawn and intra heads, so no code change is needed.
  - Offline, days 12-27, Poisson loss of band totals (fc/eval_recent/clean800_fc2ens.txt, fc/eval_weak/weak_fc2ens.txt):
    | read | fc2 | ensemble |
    |---|---|---|
    | real-opponent mix, dawn | -4.43 | -4.54 |
    | real-opponent mix, h6 / h12 / h18 | -2.43 / -2.53 / -1.41 | -2.49 / -2.57 / -1.46 |
    | M&M / top 10, dawn | -4.83 / -4.60 | -4.93 / -4.70 |
    | weak seats 101+ / 31-100, dawn | -4.67 / -4.37 | -4.77 / -4.46 |
    Single seeds are within ±0.03 of each other; the ensemble gain is about 3x the seed spread.
  - Judge arm: models/pkq_fc2ens (pkq_fc2 + the two extra files).
  - Local bundle: packages/m3fc2ens_hc = bundle A + the extra files + A's bridge stripped of debug info (14.5 -> 3.3 MB, for size).
    Archive 92.1 MB; Local-LB agent files 101.8 MB (cap 104.86 MB). A 4th seed would not fit the Local-LB cap.
- (Sep 30 16:30 BST, BC) Forecaster choice, episode-paired on the clean 800 (fc/fc_paired.py; dawn head, days 12-27, per-episode
  Poisson loss of band totals, B - A, mean (SE); negative = B better):
  | comparison | top 10 | ranks 11-30 | M&M |
  |---|---|---|---|
  | fc2ens - fc2 | -0.099 (0.007) | -0.110 (0.011) | -0.103 (0.007) |
  | fc3v - fc2 | +0.048 (0.010) | +0.105 (0.022) | +0.063 (0.012) |
  | fc3nv - fc2 | +0.058 (0.011) | +0.097 (0.022) | +0.071 (0.011) |
  | fc3vens - fc2ens | +0.039 (0.006) | +0.063 (0.012) | +0.054 (0.007) |
  - Removing the bed worlds (~1/3 of the recent top-team games) costs accuracy on real opponents. Removing our-sub data costs
    nothing more (fc3v ≈ fc3nv). The seed ensemble gain is ~2x the bed-world loss.
  - Best live forecaster: fc2ens. Its defect is judge leakage on swap-type beds, so its bed reads are G3 / field / untrained worlds
    only. Clean arms models/pkq_fc3vens / pkq_fc3nvens give a leak-free lower bound for the family.
- (Sep 30 16:30 BST, BC) Candidate bundle = packages/m3fc2ens_hc (m3 base + fc2ens + hirecheck / latehire, stripped bridge). Local only.
  - Kaggle-speed emulation vs A (reports/deadline_ens, same window, 24 games each): 0 dawns past the deadline, 0 games changed.
    Worst step 4.81 vs 4.46 s; min overage 29.5 vs 31.1 s.
  - 4 / 4 verify DONE, deterministic. Kernel-check folder prepared, not pushed. Details: its CHECKS.md.
  - The cma-base variant was dropped: live m3cma 2704 vs m3 2808 after ~60 games.
- (Sep 30 16:40 BST, BC; judge numbers from Imitation / Weaknesses) The clean 3-seed forecaster fc3vens (bed worlds out of TRAIN and
  checkpoint selection) PROMOTED on G3 218 at 96: n 120, +784 (SE 347), own +1,176, opp +392, score 63.3% vs 52.1%.
  - Vs the field opponent: +800 (SE 505; own +1,002, opp +203), no verdict at the 96 cap. It is the first forecaster arm where the
    field opponent doesn't gain as much as we do. The leak-free swap stage is running.
  - Local bundle packages/m3fc3vens_hc: same layout and checks as m3fc2ens_hc, archive sha 00c47802.
  - The user's choice if swap holds: fc3vens (clean game evidence) vs fc2ens (better held-out loss, leak-prone bed reads).
- (Sep 30 17:25 BST, BC) Divergence triage (user priority 1; sep29_mm_copy/runs/divergence/MAP.md). Model error = M&M's label vs our
  ask on M&M's exact dawn; state shift = our ask on our own dawn vs on M&M's dawn; block swaps on DC11_INDUMP dumps.
  - Copy (cp_fc2; 99 clean G3 worlds; reports/d11_cpfc2):
    - On M&M's dawns the copy asks what M&M does: sheep 1.02x, carrots d11-17 1.09x, strawberries d10-14 0.99x.
    - C3a sheep: own-state ask = M&M-state ask on d1-8. So fewer sheep placed is execution. d9-14 -6.5%, 83% from the opponent-farm
      block.
    - C4 carrots d11-17: own ask +29%. The opponent blocks (farm summary 36%, calendar 28%, market inventory 26%) and own crop groups
      (33%) drive it; servicing inputs (dry / unfed, care, fertilized days) < 5%. The shifted inputs are the opponent's quadrants
      (our 4Q sub vs M&M's mostly-3Q real opponents) and its tomato / melon plants. So C4 is a response to our sub as the G3 opponent.
    - The servicing gap does not drive the network's divergence; it sits in execution (compiler).
  - Package (48 G3 worlds; reports/pk_own, pk_attr). Per game-day, M&M label | package on M&M's state | package on own state:
    | item | label | on M&M's state | on own state | reading |
    |---|---|---|---|---|
    | P1 strawberries d6 | 14.4 | 13.8 | 9.9 | state shift |
    | P2 wheat d8 / d9 | 10.4 / 4.0 | 10.3 / 5.8 | 5.4 / 9.3 | state shift: asks a day late |
    | P2 wheat d10 / d11 | 13.0 / 12.1 | 6.0 / 8.5 | 7.2 / 11.5 | model error on d10 |
    | P2 tomatoes d8 / d9 / d10 | 2.9 / 2.2 / 4.3 | 0.0 / 0.15 / 0.08 | 0.0 / 0.12 / 0.21 | model error (5.8 on own d11) |
    | P3 geese d9-14 | 0.64 | 0.24 | 0.50 | mostly model error |
    | P4 cows d4-6 | 1.03 | 1.20 | 1.40 | asks more; fewer placed = execution |
  - Package components (60 held-out M&M games, OD_FINAL, reports/pk_attr):
    - Decode keys barely move P1-P4: reach +0.17 geese, earlycow +0.2 cows, v219 only the d10 land.
    - The old members dilute the main (main alone: strawberries d6 15.5 vs 13.9, wheat d8 13.2 vs 9.8). But even the main misses
      land-day tomatoes and d10 wheat: the D1 root cause (package networks not M&M-trained).
    - Block swaps for P1 / P2 are running (reports/d11_pkg).
- (Sep 30 17:35 BST, BC) Package P1 / P2 state-shift attribution (reports/d11_pkg; the package main v17g6ft5, raw asks, 48 G3 worlds).
  The driver is the package's own earlier plan, not servicing:
  - P1 strawberries d6 (15.3 on M&M's dawns vs 12.6 own). Crop groups 36%, seeds 26%, own plants 24%, animal types / groups 17-20%.
    By d6 we already have more strawberries standing (d3-5 push) and fewer melons; no strawberry seeds held.
  - P2 wheat d8 (12.6 vs 9.3). Crop groups 106%, own grid 48%, ready calendar 33%; dry / unfed 13%. We already have 2x the wheat
    planted and no wheat in the shed / seeds.
  - Wheat d9 (5.1 vs 10.0). 3x the free tiles at dawn d9: M&M filled its new land on d8, we fill it on d9.
  - Tomatoes d8-10 (main 0.83 vs 0.23): model error dominates (M&M 2.9-4.3).
  - Water / care flags < 15% in every swap.
- (Sep 30 17:35 BST; Day compiler's one-day harness on 600 exact live pkm1 game-days) Margin per game vs big2: fc2ens +738 (SE 311),
  fc3vens +589 (SE 194), oracle +2,114 (SE 459; own +62, opponent -2,052). The learned heads capture ~30% of the one-day value,
  mostly denial (strawberries moved into h3-11).
  - PROVISIONAL (Day compiler 18:10): these one-day margins counted our cash + shed + standing yield but only the opponent's cash.
    So denial that makes the opponent hold stock was counted as its loss. Rerun with symmetric assets in sep29_dc12/runs/sym.
    The first oracle-split arms were void: the Day compiler's xargs wrapper dropped the mode setting (not the build, as first said).
  - Symmetric one-day margins (runs/sym, both farms' cash + shed + standing yield + held product; set A days 3-26, 600 game-days):
    fc2ens +574 (SE 264): own +601, opponent +27. fc3vens +308 (SE 252): own +280, opponent -28.
    So the one-day gain is our own value, not denial; the cash-only read's opponent -0.43..-0.51k was accounting.
    Oracle arms rerun in runs/sym2.
- (Sep 30 17:45 BST, BC; judge numbers from judge.log) Forecaster arms, clean game evidence:
  | arm | G3 218 | vs field opponent | swap 234 |
  |---|---|---|---|
  | pk_fc3vens | +784 (SE 347) PROMOTE | +800 (SE 505), own +1,002, opp +203 | +2,161 (SE 501) PROMOTE; origin-weighted +1,201 (SE 274), origin 1-10 +4,108 (SE 947) |
  | pk_fc2ens | - | +290 (SE 541), own +987, opp +697 | leak-prone |
  fc3vens has the only leak-free, all-bed evidence; bundle packages/m3fc3vens_hc is ready (local).
- Hourly timing skill (fc/fc_hourly.py, fc2ens, clean 800, days 12-27). The model's hourly means beat "same daily total x the group's
  average hourly shape" by 2.5-7 Poisson units per product-day. Its top-3 predicted hours hold 51-58% of strawberry / milk / wool
  units vs 31-43% for the average shape. About 45% of units still fall outside them, which is where the one-day oracle's +1.4k
  headroom lives.
- (Sep 30 18:40 BST, BC) Early-day forecaster accuracy, days 3-9 (fc/fc_hourly.py). Sets: set A (25 live pkm1 games after 07:30,
  opponent seat, confirmation use) and the clean 800. Files: fc/eval_setA|eval_recent/early_units_3-9.txt.
  - Opponents sell little on days 3-9: strawberries ~0, eggs 0.1-0.3 (top 10 on set A 1.2), milk 1.7-2.1 / day in h0-11, wool
    3.2-4.4 / day ~95% in h3-11.
  - fc2ens / fc3vens put 91-100% of milk / wool units in their top-3 predicted hours (big2 64-88%). After day 12 it is 50-58%.
  - h3-11 is over-forecast by 5-20% (0.1-0.5 units / day), fc3vens closest.
  - The opponent forecast is not the early-day weakness. Early prices are set by our own supply and shop demand.
- (Sep 30 18:55 BST, BC) Reserve check (reserved_confirm.txt 131 + pkm1_after0730_all / B_unscored). Forecaster data d1-d3 and copy
  arrays: 0 rows. LEAK: arrays_field (field clone m4q_field_sbc_s1 -> field_s0 / field_decem / Weaknesses' opp_field) trained one Vadim
  perspective of set-A game 115748906 (11:22 UTC). make_corpus.py excluded only M&M's post-07:30 games; fixed to exclude every team.
  No read is affected: the clone is never used on set A.
- (Sep 30 19:10 BST, BC; oracle split from the Day compiler, runs/sym2 symmetric accounting, set A days 3-26, vs fc2ens)
  - Full oracle +1,908 (SE 289): own -810, opponent -2,718, so it is denial.
  - True day totals +1,012 vs true shape +680.
  - One product at a time: milk +543, wool +521, strawberries +289. One band at a time: h12-20 +1,197, h3-11 +447.
  - By days: 3-10 +6, 11-18 +843, 19-26 +1,059.
  - Does the forecaster miss daily-total information? No (fc/fc_resid.py, clean 800, days 12-27, fc2ens):
    - daily totals already correlate 0.88-0.91 with actual and are unbiased;
    - a cross-validated gradient-boosted model of the residual from every row feature explains R2 0.00-0.04 (milk day 0.020,
      afternoon 0.002; wool 0.016 / 0.001; strawberries 0.037).
  - The oracle's value is the opponent's unobservable hold / sell choices. New forecaster modelling beyond the 3-seed ensemble is
    closed. Next lever (Day compiler): selling decisions that use the forecast's real dispersion.
- (Sep 30 19:25 BST, BC) Forecast-error dispersion for the DP's scenario solves.
  - Tables: fc/eval_recent/residual_dispersion_fc2ens.csv; nb_sampler_table_fc2ens.csv (fc/fc_disp.py, fc_nbtable.py). Source: fc2ens,
    clean 800 real opponents, days 12-27, live-mix weights.
  - Band totals are much lumpier than Poisson: phi = Var / mean is 3-8 at means < 1 (NB size k 0.02-0.19), 2.3-5.4 at means 1-4, and
    up to ~6 for milk / wool h21-23.
  - Sent to the Day compiler for a screen: NB band totals spread over the forecast's hourly shape vs plain Poisson paths.
  - Copy arm with the clean forecaster for the copy-vs-package swap read: models/cp_sbc1_land_fc3vens (cp_sbc1_land_fc2 + fc3vens
    files; only forecast files differ).
- (Sep 30 19:50 BST) Improve Agent (kaggriculture-4e) is submitting pavel-bc-opus-v17d-dc12m14-fc2-m68 (= packages/m3fc2hc, single fc2)
  on the user's ask, retiring 56706309 (m3cma). BC told them the artifact is sound, and that m3fc3vens_hc has the better leak-free
  evidence, for the user.
- (Sep 30 19:45 BST, BC) Day-level forecast dispersion: the Gamma day-factor variance v implied by day totals is small where the volume
  is (means 4-10: milk 0.053, wool 0.089, strawberries 0.11; means 10+: 0.01-0.04). Only small means need v 0.3-0.7. The lumpiness is
  within the day (band allocation). The Day compiler's scendisp v = 0.5 / 1.0 arms over-disperse; calibrated v 0.05-0.1 or band-level
  NB is the faithful test.
  - Drop-in sampler for that test: sep29_bc_mm/fc/cpp/nb_bands.hpp (NB band totals spread by the forecast shape); a 200k-draw test
    reproduces the table's Var / mean.
  - scendisp (Gamma day factor, over-wide) lost dose-dependently: v 0.5 -448 (SE 140), v 1.0 -779 (SE 157); own +0.52k, opponent
    +0.97-1.30k (we sell more, deny less). Closed.
  - Within-day band allocation given the actual day total spreads 3.9-5.9x a multinomial around fc2ens's band shares.
    Dirichlet(alpha0 x shares) fits with alpha0 ~ 3.8 strawberries, 2.2 eggs, 3.6 milk, 3.8 wool. That calibrates the Day
    compiler's scenlump arm (Poisson day total split by Dirichlet), which runs with alpha0 = 2.
  - Band samplers (runs/sband): nb_bands.hpp -1,085 (SE 170; own +2.10k, opponent +3.18k); scenlump=2 (Poisson day total,
    Dirichlet(2) split) -225 (SE 98). All four dispersion arms lose and scale with the spread: wider scenarios make our DP sell
    more stock and give denial away. Committing to the point forecast is what denies. Dispersion line CLOSED (Day compiler's
    stop list).
- (Sep 30 21:00 BST; Weaknesses' pre-live judge) The bundle packages/m3fc3vens_hc itself (build m14bf, identity 3 / 3):
  - G3 first 96: PROMOTE at 48, n 58 +1,111 (SE 460), own +1,247.
  - Swap 93 clean: PROMOTE at 24, n 39 +1,947 (SE 688), own +440, top-10 origin +2,865.
  - Vs the judge arm pk_fc3vens (the difference is hirecheck / latehire + m14 tree): G3 +397 (SE 195), swap +67 (SE 200).
  - Submission is the user's call.
- (Sep 30 18:44 BST / 17:44 UTC) Kaggle 56714867 = pavel-bc-opus-v17d-dc12m14-fc2-m68 (packages/m3fc2hc, single fc2 + hirecheck /
  latehire), submitted by the Improve Agent on the user's ask. Kernel check 4/4, worst call 2.63 s, overage >= 45.5 s. Retired 56706309
  (m3cma). Active pair: 56710248 (giovanni-rl-v1643) + 56714867. m3fc3vens_hc stays local (the user's call).
- (Sep 30 20:10 BST, BC) M&M's animal placement rule, days 0-9. Sources: 650 non-reserved M&M games vs our 107 live m3 games before
  07:30; tool tools/place_log.cpp; data reports/placement.
  - Both farms place each animal in the nearest free structure of its kind (moves to the nearest shed-access tile): M&M 98-100%,
    ours 94-100%. The difference is the ORDER.
  - Day 0, same 5 pastures:
    - M&M (650 / 650 games): cow r0 h3; sheep, sheep r1 h5-6; sheep + cow r2 h7-9.
    - Ours (106 / 106): cow r0 h3; sheep r1 h5; COW r1 h6; sheep r2 h10; sheep r2 h18.
  - Days 3-9, M&M: pastures built before coops the same day (h13.7 vs 14.9); cows / sheep placed before geese.
  - Days 3-9, ours: the reverse (coops h14.7 before pastures h16.5, geese first). So our coops hold ring 0 (0.58 vs 0.27 per game)
    and our cows / sheep are placed ~3 h later.
  - Rule for the Day compiler's balanced arm: keep nearest-free; day-0 claim order cow, sheep, sheep, then sheep / cow; later days
    pasture animals and pastures before geese / coops; day-0 placements by ~h9.
- (Sep 30 20:40 BST, BC) Walking cost of animal distance, days 3-29, 650 M&M vs 107 of our live m3 games (tools/trip_log.cpp,
  walk_log.cpp; reports/placement/trip_read.py).
  - Removal detour of each animal stop on the routes as walked rises 0.11-0.13 moves per extra step (animals are chained, not
    dedicated trips). Animal stops per animal-day 1.15-1.3.
  - => 0.13-0.15 walk turns per animal-day per extra step.
  - Steps beyond ring 3 per game: ours 178 vs M&M 49. Extra walking: ours 23-39 vs M&M 6-7.5 worker-turns per game, a gap of
    ~1 turn a day.
  - Calibrates the lifetime site-cost key: 0.13-0.15 x remaining days per step (d5 ~3.1-3.6; d10 ~2.5-2.9 turns), vs today's 2 paid
    once.
  - Walking is a small cost; any milk-market loss from far cows must run through collection / sale timing.
- (Sep 30 21:25 BST, BC) Intent v2 design note: experiments/v10/sep29_bc_mm/DESIGN_intent_v2.md, reviewed against Codex's independent
  note (findings/bc_intent_redesign_codex_sep30.txt).
  - Adopted:
    - binding fields with a per-job miss ledger;
    - delivery as a collect -> return -> deposit package with a direct-return flag, fertilizer as a separate service;
    - deadlines = first sellable hour, with opening bins h1 / h5 / h9;
    - the v2 model conditioned on the decoded aggregate;
    - no style averaging (M&M main, others via a style input);
    - explicit fallback rules;
    - oracle vs learned fields through one executor, and full-game reports by day block.
  - Kept mine: a separate small v2 model instead of heads on the main network (fine-tunes of deployed mains cost 1-2k); placement as a
    rule for M&M (deterministic in 650 / 650); no spending / prebuy plan in v1.
  - Open conflict to test as separate arms: Codex's legal balanced placement -484 (SE 517) vs the Day compiler's nearanimals=6 +1.2k.
- (Sep 30 21:05 BST, BC) Collection -> shed timing by ring (tools/collect_log.cpp), days 3-29, weighted by units:
  - Far animals (ring 4+) are harvested h13-15 and their product reaches the shed ~h22 (mostly overnight) for both farms. Near milk
    is in the shed by h6.6 (M&M) vs h10.6 (ours).
  - 33% of our milk comes from far cows vs M&M 14%. Far placement costs delivery time, not walking.
- (Sep 30 21:50 BST, BC) Intent v2 opening labels (tools/label_v2.cpp; reports/intent_v2). Teams: M&M 650, DECEM 470, Vadim 343,
  DSM 247 (pre-07:30); ours 107 live m3 games.
  - The four converged teams play the same fixed opening script by day.
  - Crew wave 1 by day 1-9: 3, 5, 6, 6, 6, 9, 8, 8 (+1 hire at h1), 9 (+1). M&M = the day's mode 82% of days, +-1 98%. We hire 2-3
    fewer on d2-7.
  - 80% of near units sellable: wool d6 h6 (100%), wool d9 h6 (84-88%), milk d8 h6 (100%). Ours: wool h7-9, milk h4-5.
  - Fertilizer on a later stop: 97-100%.
  - Proposed v1 = this table as rules (opening_script_mm.csv), not a learned model. Imitation's regime probe (night delivery from d10)
    lost -2.3k on the real-opponent bed because our seller sells the night stock at h21-23, so "night" stays out until a dawn seller
    exists.
- (Sep 30 20:55 BST, BC) User deadline (best end-to-end agent by 22:30). Two local bundles, both with the Local-LB branch pushed:
  - packages/m3fc3vens_hc (branch 9402597): recommended.
  - packages/m19fc3vens (branch 2368827): + splitfert=10 nearanimals=6.
    - G3 vs m3fc3vens_hc +961 (SE 502); swap interim +758 (SE 821) with own -765.
    - Imitation's realistic bed says the opening keys are ~0 on margin (-0.04k) and cost ~1.8k own money.
  - Both: 4 / 4, deterministic, deadline OK.
  - The intent-v2 opening script as binding fields lost on the real-opponent bed (openscript=1 -0.62k, =2 -0.20k), so no v2 agent
    beats these tonight. The next lever is a mid-game dawn seller (Day compiler).
- (Sep 30 21:10 BST; Day compiler) The teachers' mid-game dawn-sale share as an executor rule loses on our farm.
  - dawnshare=50 on the hybrid bed: milk + wool -603 (SE 303); all four products -907 (SE 199); own -1.2k / -1.8k.
  - Holding evening units overnight: -189 per game-day.
  - With the opening-script loss (-615 / -202), neither teacher script transfers as a binding rule. Intent v2 fields for them are out;
    the DP timing stays.
- (Sep 30 21:30 BST, BC) USER DIRECTION: intent v2 cancelled. All work goes to beating Local-LB #1 (pavel-bc-opus-v17d-dc12m19-fc2-m68,
  1637.9) on the old interface. BC arms on the #1 base (sep30_pkg_improve/packages/...dc12m19-fc2-m68/model) in sep29_bc_mm/models/lb1/.
  Each differs from the base only as listed:
  - f1_fc2ens: + fc2 seeds 1 / 2.
  - f2_fc3vens: the clean 3-seed forecaster.
  - f3_fclin: fc2 recipe with our subs' games x6, 3 seeds.
  - f4_fclinfresh: f3 + the 35 pkm1 games 07:33-14:30 (Weaknesses' list; B_unscored and post-14:30 kept out). Training.
  - c1_str15 / c2_rec110 / c3_both: the main's condition strength 1.5 / recency 1.10 / both.
  - e1_mw04: mainweight 0.4.
  - c2f2: c2 + f2.
  - Imitation's G3 pre-screen vs #1 (n 18, noisy; base mirror score 0.53): c2_rec110 0.67 (+0.50k, SE 1.02k), f2 0.56 (+0.03k); c1,
    e1, f1, c3 0.39-0.44 (-0.5 to -1.6k). The exact-LB replay (Weaknesses) decides.
- (Sep 30 21:45 BST, BC) Recency conditioning, the Local-LB push's lead arm (c2_rec110: main condition recency 1.025 -> 1.10).
  - Imitation's realistic bed: +1.74k vs #1 (SE 0.63k, n 18), the opponent -2.1k from day 10.
  - Mechanism, dawn herd on 24 G3 worlds (reports/lb_herd): geese +0.5 by d12-14 (6.42 vs 5.92), cows +0.4 on d4, sheep -0.3.
  - Queued arms: c4_rec120 (1.20), c5_rec110all (members too; their .condition files are read), f4_fclinfresh, plus the Improve
    Agent's stacks.
- (Sep 30 22:00 BST; Weaknesses' exact Local-LB replay) Recency conditioning FAILS on the exact LB vs #1.
  - c4_rec120: score 10% (3 of 30), -2,812 (SE 1,028); c2_rec110: -961 (SE 1,123).
  - It helped on the realistic hybrid bed (+1.74k) but loses on the lineage roster: moving the plan toward the converged teams (more
    geese) loses to #1's lineage play.
  - Open: c5_rec110all, f1_fc2ens (full run under way), f3 / f4 lineage forecasters, the Improve Agent's stacks.
- (Sep 30 22:30 BST, BC) Local-LB push status and user rules (end of competition: general changes only; no lineage-weighted training;
  no knob search; a candidate must pass the exact LB judge + be non-negative on the hybrid bed + have an exact identity check).
  - f1_fc2ens PASSED the exact LB: vs #1 53.3% (n 30); vs the top 3, 90 games: 76.7% vs base 64.4%, paired +922 (SE 490). Hybrid bed
    +0.53k (SE 0.42k). Packaged by the Improve Agent (PR). My model dir is identical (packages/m19fc2ens).
  - Recency arms failed on the exact LB: c2 -961, c4 -2,812, c5 -942. f3 / f4 (lineage-weighted forecasters) are dropped by the user rule.
  - f6_fc3nvens (no lineage data at all) is queued. Flag given: fc2 (#1's forecaster) itself trains on our subs' games at the normal
    x2 recent weight.
  - #1 vs the sibling late-strawberry gap (Day compiler's dumps; scripts/d11_pair.py):
    - the main network's d10-13 strawberry share is lower on #1's states (1.70 vs 2.01 per dawn) while its total crop ask is
      slightly higher;
    - driven by the whole richer farm (own grid 36%, animals 29%, plants 27%, tile kinds 22%), not one input;
    - no decode arm proposed under the rules.

## bind() audit for dc12 (Sep 29 ~19:50, BC; dc11 v95 = experiments/v10/sep25_compiler_overhaul/dc11)

How the network's DayIntent becomes stops / entities (compiler.cpp bind() l.70-330, trim_new_entity l.700-742). What the network
means by each field:
- one-shot groups: `options[g][o]` = number of members getting option o (water / fertilize / harvest bit set, or clear); counts sum
  to the group size;
- ongoing groups: `retain / clear / fertilize / harvest` = member counts;
- animal groups: `feed / care / collect` = member counts;
- whole farm: `new_crop / new_animal / reserve / buy_land`;
- `trim_order / drop_loss`: the decoder's ranking of which new units it would give up first.

Each assumption: lines, evidence vs M&M, severity (H / M / L).

1. **Budget trims ignore the network's preferences and cut the most expensive new units first.**
   - Lines: trim_new_entity l.717-742. Before `cropfirst`, crops go most-expensive-seed first, then animals; from `cropfirst` on,
     animals go first in the order sheep -> cows -> geese. `drop_loss` is never read; `trim_order` is used only with `trim_net=1`
     (not in any package line).
   - Evidence: the copy's live-world herd tilt (geese +0.9, cows -0.6 vs M&M, Imitation's mm.sh vs live d3crop) appears only when
     cash is tighter than on M&M's own path. In pinned M&M worlds (M&M's cash path) the copy's geese match M&M's: 7.46 / 7.56 vs
     7.55 at dawns 11 / 13. Weaknesses q336: the day-3 cow is asked in 93% of our live games and bought in 4%.
   - Severity: **H** for herd composition. dc12: trim by the network's own drop_loss (log-probability lost per unit per dollar), not
     by price.
2. **Ongoing crops are watered only when dry (survival) or when the watering doubles tonight's production.**
   - Lines: l.200-205 (retained members; non-retained members never get water).
   - Evidence, days 6-16 on M&M's own dawns and intents (87 games): tomato waterings ours 39-41 vs M&M 56.6 per game. At dawn 10,
     73% of our strawberries are dry vs 9% of M&M's, which shifts the M&M net's inputs (day-10 ask -3.7) and makes the next day's
     waterings mandatory (P_SURVIVAL). Forced daily watering: -10 / -4 gate plantings (Day compiler).
   - Severity: **M**. Yield-neutral per the rules, but it concentrates labour on alternate days and moves the network's inputs off
     the teacher's distribution. dc12: model the watering as a two-day choice (today vs forced tomorrow) with the labour of both days.
3. **Animal service has flat values: feed $30 if not hungry (cost if unfed), care $20.**
   - Lines: l.244-253.
   - Rules: care + feed banks +1 unit for the next production, and banked bonuses need feed on the production day. So one care is
     worth one milk / wool / egg (milk / wool $50-130), and a missed feed on a production day loses the whole bank.
   - Both are P_OUTPUT (required), so the values act through the router's ordering and the route-end drops under time pressure.
   - Evidence: Imitation, facing real M&M our sub feeds / cares less and produces less milk (-8.5) and strawberries (-11) on days
     12-29; from M&M's day-12 state the copy cares for cows less. Whether that comes from these values or from the care intent is
     untested.
   - Severity: **M-H** (thin-product volume is where M&M's edge is). dc12: value service = expected banked product x price, and a
     feed on a production day = the bank at risk.
4. **One-shot water-only stops are valued at the whole plant.**
   - Lines: l.163-166. Value = seed + current yield x price, whatever the water does; a yield-adding water's marginal gain (water_gain
     x price) is not used. Harvest stops do include water_gain (l.155).
   - Evidence: melons, days 6-16 on M&M's dawns: the net's water intent = M&M's (66.5 vs 67.2 per game); dc11 executes 68.3-69.0 vs
     M&M's 69.4, yield-adding equal (corrected 19:05: the earlier -2.6 missed h23 waterings).
   - Severity: **L** today, but conceptually wrong. dc12: value = water_gain x price (+ survival value if dry).
5. **New entities are valued at a flat 3 x seed (crops) or the purchase cost (animals)**, whatever the crop, the day or the market.
   - Lines: l.293, 309. Used when entities don't fit (dropped P_NEW).
   - Evidence: the day-10 drops on our own states are P_NEW plant stops (3.4 per game with the fuller ask, BC dry2). Which crop is
     dropped follows this value; untested vs M&M.
   - Severity: **M** on wave days (6, 8, 10). dc12: value from the network's drop_loss or the expected harvest value.
6. **The new quadrant's tiles become sites only at the land-purchase hour.**
   - Lines: l.62-68, 97-98, with land_afford_hour deferring it.
   - Evidence: on day 10 the land moves to h14-17 (cash wall: land ~$3.9k vs ~$1.35k dawn cash); new-quadrant plantings are dropped.
     M&M sells at h8-11 and buys at h9.
   - Severity: **H** on the Q4 day; it belongs to the funding redesign (same-day sales funding purchases).
7. **Fixed night rules: 1 wheat per animal kept for tomorrow's first feed; night room 95 minus reserved animals.**
   - Lines: l.321-328.
   - Evidence: the day-6 cow trims on M&M's intents come from the next-dawn reserve (Day compiler, v95 reserve=0).
   - Severity: **M** (funding).
8. **Option-to-member assignment**: the option with the most actions goes to the member nearest the shed.
   - Lines: l.129-138.
   - Assumes members are interchangeable. There is no bounds check if the option counts sum to less than the group size
     (the decoder guarantees it).
   - Severity: **L**.
9. **Survival level**: options collapse to harvest + survival water, feeds are capped by a cash-based budget. Fallback only. **L**.
10. **Output stops use today's price** (held x current price, l.259-262), not the sale-time price the seller will get. Collections
    can be deferred (P_EXTRA, defer_collect), fertilizer-only pickups are P_EXTRA, collectmin gates fertilizer. **L-M** (it feeds the
    router's collection timing, which is linked to the seller's late selling; see Day compiler items 1-4).

The biggest levers for dc12 from the intent side: (1) trims by the network's own preferences, (3) service values from the banked
production, (6) same-day financing of land / animals.

## Sep 30 22:17 BST: overfit audit of #1 / f1, network side (user ask via Imitation)

FLAG = chosen on lineage beds or the exact LB only.
- Main v17g6ft5, FLAG. Data is clean enough: our subs are at least 20 of 4,641 fresh rows. The problem is the seed pick: one of 4-6
  same-recipe seeds (~+0.3k mean, +-1k spread), picked on h2h vs econm6 (+1.37k) and the exact LB (260/300). Real-opponent beds were
  level or negative: RR over 8 clones +0.24k, clone bed -199 (SE 535), live27 -300 (SE 375). No isolating live A/B.
- Members w384b / sw100 / mw5k / mw2k with equal weights, mild FLAG. An incumbent since E. Kept because fresh members lost on the
  exact LB (-2.1k) and the clone bed (-0.6k). Weights never tuned.
- .condition "1 0.75 1.025": the training default, not tuned. OK.
- .opening "6 7", FLAG. Chosen on Sep 25 lineage mirrors (82-38) and the exact LB. On Sep 28 the other pins lost on h2h and the
  clone bed.
- .decode:
  - reach: lineage mirrors; neutral on live27. Low harm.
  - v219 / max_land: real top-10 replays +2.1k; the lineage LB was against it.
  - earlycow: real-game lists +0.39k / +0.96k.
  - earlycrop: 760 games and 53 real-world duels.
  - qpush: gate + RR over clones, then Local-LB #1.
- .compiler keys, FLAG. Sep 25-26 C++ mirrors vs our own packages + exact LB. sell_order 2 is a general mechanism, also found
  independently (X13). race_early / race_steps / melon_tie_now are inert. hire_cap_stock read ~0.
- fc2, FLAG. recent_ours = 919 of 1,938 recent train sequences (x2 weight, ~5% of the weighted data). Real-opponent evidence: the
  one-day live harness, fc2ens +574 (SE 264). Clean swap: f6_fc3nvens.
- f1 margins are within ~1-2 SE: exact LB 53.3% (n 30), 90 games +922 (SE 490), hybrid +0.53k (SE 0.42k).
- (22:35 BST) Real-opponent check of the clean forecaster. Bed: the symmetric one-day harness (runs/sym bed, set A, 600 game-days,
  same binary 16ba1fd8; reports/sym_fc3nv/).
  - vs pkg: fc3nvens +585 (SE 241), fc2ens +574 (SE 264). Paired fc3nvens - fc2ens: +11 (SE 226).
  - The exact LB orders the same forecasters by lineage data: fc2ens +922 > fc3vens +42 > fc3nvens -1,153 vs #1.
  - So fc2's exact-LB edge is most likely lineage fit, not skill vs real opponents. Caveat: the one-day bed has no compounding or
    reaction.
  - f1 vs #1 adds only fc2 seeds (no new data).
- (22:58 BST) Honest main-seed arms for Imitation's real-opponent re-selection: sep29_bc_mm/models/honest_seeds/<arm>. Base m3_fc3nv,
  with only model.bin swapped; identity checked by build.sh.
  - Same recipe as the current main v17g6ft5 (seed 121): g6b / g6c / g6d.
  - soupG4: all 4 same-recipe seeds.
  - The + sep28e recipe: e1-e5, plus soupE5.
  - soupG4E5: all 9 seeds.
  - The swaps change 19-45% of game-days, mostly days 6+ (soups the least).
  - Suggested: soups first (no selection), then seeds with ~2.5 SE on both beds.
- (22:57 BST) Seed re-selection runs only the two soups (soupG4, soupG4E5): Imitation's hybrid bed + the Day compiler's one-day bed (build_m19td, runs/keyaudit). The 9 single seeds are not run (no best-of-9 pick).
- (23:50 BST) Pick-2 reads for Imitation (second Kaggle pick = m3_fc3nv vs honest1). Bed: BC's symmetric one-day bed, set A, 25 games,
  build_m19td (build_dc12v31 aborts on honest1's keys); reports/sym_honest1/.
  - The package reproduces 600/600 recorded days.
  - m3_fc3nv - pkg: +585 (SE 241).
  - honest1 - m3_fc3nv: +267 (SE 139), all on days 3-10; own +187, opponent -80 (SE 18).
  - Split on m3_fc3nv: splitfert=10 alone +104 (SE 87; own +28, opponent -76); hirecheck + latehire alone +105 (SE 78; own +96).
  - Forecasters, days 3-9, opponent morning milk (h3-11) pred vs actual 0.86: big2 (m3) 0.91-0.96, fc2ens 1.10-1.12, fc3nvens
    0.91-0.93. So fc3nvens is like m3's forecaster where fc2 departs; wool is about equal across the three. fc3nvens has the best milk
    loss.
- (00:05 BST Oct 1) Work finished per the user (via Imitation). Final Kaggle pair: 56720080 (m3) + 56720831 (honest1, with the fc3nvens forecaster). No BC jobs running.
