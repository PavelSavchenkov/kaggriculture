# Deviations from M&M, ranked by dollar value (owner: Imitation; others send updates)

Status: open / triage / fix screening / passed gate / integrated / closed (with reason). $ = per game on the 6 exact worlds (G3) unless noted.
TARGET REFRAMED (Sep 30 03:07): the 6 exact worlds are M&M's big wins (outcome-selected) and M&M's recording is open-loop (tiny
perturbations cost it 0.6-1.3k, the sub regresses up). On 278 unselected held-out worlds (G3-wide), M&M's recording beats the live
package by +1.43k (SE 0.38k, 67% wins); the package in M&M's seat is -1.35k vs the recording. So M&M's true edge over our package is ~1.4k
(+ the open-loop penalty), and that is what the copy must reach on G3-wide. Exact-world numbers below are for paired arm comparisons.
G3 now: package +6.20k vs M&M's real +9.49k (gap 3.3k); from a day-12 hand-over +7.00k (solo 12 17: +9.57k); from day 18 +10.59k.
Integrated copy (Sep 30 02:45): copy_pure (own keys) whole +5.28k, hand-over U3 +4.40k, U6 +3.37k, U9 +4.31k, U12 +8.37k (U12 > U9 in all 6
worlds). copy_pure with package m3 keys: whole +0.00k (collapse). Attribution on the 6 worlds: G2 asks match M&M; G1 d2-5 with the copy's
keys plantings 0.77, fallback 54% (216 games: 0.806 / 39.9%; package keys 0.964 / 15.5% but cows -0.30) -> D3 is the copy's main loss
(melons 59 vs 72 per game with M&M's first 12 days, -$2.0k revenue).
G3 gap = denial (Sep 30 02:50, scripts/denial_split.py, 5 exact worlds with ledgers): copy_pure earns what M&M earns (own +0.25k) but
our sub earns +4.46k more than vs real M&M; sub revenue +6.4k per world, mostly wool +2.8k and milk +2.4k on days 12-29. Same units
(milk 177 vs 176, wool 96 vs 95), higher prices: sub milk 106.5 vs 94.6, wool 105.3 vs 80.7; the copy's own milk 113.2 vs 103.2, wool
117.6 vs 103.9. M&M's selling pattern costs itself ~10-14 / unit and the sub 12-25 / unit on milk / wool -> ~1.6k of margin = D4. League
agrees: copy_pure vs 3 strongest -1.68k (SE 0.65k) vs the package, own +1.6k, opponents +3.3k.

| # | Deviation | Evidence | Est. $ | Owner | Root cause (so far) | Status |
|---|---|---|---|---|---|---|
| D1 | Land-day crop asks too low (d6 melon, d8 tomato / geese, d10 wheat / carrot / tomato / geese) | G2 per day (g2_days.py); exact worlds plantings d8 40 vs 89, d10 45 vs 95 | part of 3.3k (days 0-11) | BC | ROOT CAUSE (BC, Sep 30 02:29): the package's networks are not M&M copies (v17g6ft5 all-top-teams main + Sep 8-20 E members); v219 / max_land / landpush / averaging are patches on top. Generalised fix: copy_pure (M&M copy, no decode patches) passes all crop / animal / land G2 checks | fix passed G2 (in-sample); held-out G2, G3 + guards running |
| D2 | Days 12-17 crop asks low (wheat 0.82-0.85, tomato) | G2; solo 12 17 lifts U12 from +7.00k to +9.57k (= M&M) | ~2.6k from day 12 | BC | same root cause as D1 | covered by copy_pure |
| D3 | Early funding (d2-5): M&M's intent cannot be executed: package keys plantings 0.964 / cows -0.30 / fallback 15.5%; copy keys plantings 0.806 / cows -0.05 / fallback 39.9% | G1 (216) + G3 attribution on the 6 worlds | TOP: ~2-3k (copy loses 3.1k on days 0-11) | Day compiler | ROOT CAUSE (Weaknesses + Day compiler): with cashsell, unseeded plantings are skipped but their WATER fails the funding sim, so M&M's h6-10 cow variants are rejected; deferred variants keep the placement stop unrouted; the ladder takes the first funded variant and its cash timeline ignores the seller's own sales | redesign: sales planned to cover purchases, variants compared by achievement (Day compiler line B) |
| D4 | Thin-product selling: we sell through each day (evening share +27..+40 pts d12-17), M&M holds back in the evening, book drains overnight, M&M sells high at dawn; eggs / wool carried by M&M (units 1.2-4x ours per day) | G1; G1s price per unit ~equal (sell_price.py) | TOP with D3: ~1.6k on milk / wool in the integrated copy (G3 denial split); ~0.7-1.1k on the same stock (G1s) | Day compiler + BC (I1) | M&M = tick seller: sells in the hour after each drain, lots ~1/3-1/2 of the drain growing with stock, smaller when the book is above its day mean, plus a dawn lot of overnight (on-plant) strawberries; our DP sells a day's stock through. fieldflow f=0.7 closed (G1s -4.54k) | screening: DP wait cost hourdisc (Day compiler line A) |
| D10 | Deposit timing: M&M brings animal products / strawberries to the shed in the evening (eggs 79%, milk 47%, wool 48%, strawberries 70% at h21-23; milk 23% / eggs 15% at dawn), holds overnight, sells at dawn and after drains; our routes deposit mid-day | G1 dep_* checks (runs/gates/g1_cp_d1224.txt) | part of the ~3k farm-side G3 gap | Day compiler | ROOT CAUSE (03:33): follows the DP seller's deposit values (an h2 drop is worth ~one extra return trip); closed as a consequence of D4. TRIAGE (03:12): M&M hires ~9 at h0 and makes an early collect-and-deposit wave (drop at h2 = 25% of the day's deposits); ours has no h2 wave (first trips carry feed / seeds and collect on the way) and 14% of units arrive only via the day-end pocket auto-deposit. Router already values early deposits most, so it is route structure: (a) can the search build a short first trip? (b) last shed visit too early | root-cause check (a) / (b) |
| D11 | Network drift on its own states: asks match M&M on M&M's dawns (G2) but not on the copy's own dawns (d2 melons 0 vs 1.6, d3-5 strawberries +40%, d10 wheat / carrot / tomato low; G3-wide trajectory: tomatoes d12-29 0.37-0.59, melons late 0.35, geese +15-20%) | runs/duel/intent60*, hand-over at dawn 1 / 2 | largest own-income item (~1.5-2k melons + tomatoes) | BC + Day compiler | the copy's own day 0 flips the d2 melon head (not cash); candidates: day-0 execution (wheat sales, plants 18.0 vs 18.9, idle), dawn stock vs cash shape (DP converts stock to cash) | triage: BC input diff, Day compiler day-0 execution |
| D5 | Dropped stops d18-24 0.48 / day, weeds +0.27 / day | G1 | small | Day compiler | open | open |
| D6 | Q3 slip to day 9 (16% of pinned games) | Day compiler pinned live29 | 0.2-0.3k / game | BC (decode) + Day compiler | day-7 spend-down under reserve=0 leaves $258 at d8 dawn vs $568; land_push 8 8 10: +86 (SE 137), slip games +382 | screening live28 |
| D7 | Watering 0.80-0.96 of M&M (total) | G1 | ~0 unless yield waterings differ | Day compiler | rules: only yield-age / fertilized pre-production waters earn; we water when dry >= 1, M&M daily | re-score with water_yield |
| D8 | Ongoing harvests: net asks ~13% more strawberry / tomato harvests than M&M d12-24 (M&M carries yield on the plant) | G2 value by kind (copy_pure +287 / +222 $ per game-day signed) | unknown | BC | ROOT CAUSE: the per-group mode decode sharpens rates (not the heads); step 68 .decode optrate 1 matches rates (1.007 / 1.012); the remaining +$38-42 signed = our net harvests decaying / capped yield M&M leaves (in our favour) | fix passed G2 (held-out); in copy_or_v5d |
| D9 | Feed / care more than M&M d12-24 | G2 (+40-52 $ per game-day signed) | small | BC | same decode rule as D8 | fix passed G2 (held-out): feed / care 0.99-1.01 |

Data (Weaknesses, standing): pull fresh M&M games from Kaggle every few hours; add M&M-vs-our-agent games to the exact set after the
exactness check; keep a held-out M&M list for G2 (games not in any network's training data).

## Ideas / research lines (Imitation keeps this list; owner in brackets)

- I1 (user, Sep 30 02:40) Learned M&M seller: if M&M's selling rules are hard to reverse-engineer, train a model on M&M's hourly sales
  (all 594+ M&M games, Weaknesses' ledgers) to predict its per-hour, per-product lots from the state (stock in shed / pockets / on the
  plant, market inventory and price, hour, day, opponent stock and recent opponent sales, drains). The executor then sells what the
  model predicts (or uses it as the DP's prior / constraint) instead of the lone-seller DP. Gate: G1 sale shares / units + G1s
  day-average price and margin vs rec_s. [BC trains the model; Day compiler adds the executor hook; Imitation gates and integrates]
- I2 (user, Sep 30 02:45) Fit the DP to M&M: keep the DP seller but expose learnable parameters of its objective (wait cost per hour,
  lot-size / price-impact penalty, carry value across nights, weight on the opponent's revenue, dawn / evening preferences) and
  optimise them deterministically (grid / CMA) to maximise agreement with M&M's per-hour sales on M&M's recorded days (seller-only
  replay: fast, no full compile), then check G1s margin. [Day compiler]
- I3 (user) New DP objectives: count how our lots affect the opponent's revenue, including over several days (the follower's
  reaction, next mornings' books). Margin, not own price. [Day compiler]
- I4 (user) Rule loop: hand-designed selling rules, and a per-hour decision diff vs M&M on M&M's states: where we sell and M&M holds
  (and the reverse), the state there (stock, book, hour / drain, opponent stock and recent sales), and where M&M gains from the
  difference (G1s); feed those gains into rules / objectives / model inputs. [Imitation builds the diff harness; Weaknesses helps]
- I5 (user) Does M&M look at the opponent, and how deeply? Test whether opponent features (its stock, recent sales, product mix, seller
  type) predict M&M's lots beyond M&M's own state (held-out likelihood with / without them in BC's I1 model; natural experiments:
  M&M vs evening vs dawn sellers in the 594 games). [Weaknesses + BC]
Day compiler line B (02:45): achieve=1 reserve=0 on G1 d2-5 (60 games): fallback 34.6% -> 1.7%, plantings 0.804 -> 0.827 (day-2
label artifact; day 3 open), all service checks pass. hourdisc 0.99 G1s -1.15k (denies the sub -0.8k, own -0.85k): not a pass.

Sep 30 02:57 seller size revised (same-hour deposit bug fixed in duel_mm's REC path): G1s package seller -0.38k (SE 0.56k) vs M&M's own
selling (24 worlds, Day compiler); exact worlds M&M's farm + our seller -1.05k (SE 1.10k; was -2.28k with the bug). So D4 is ~0.4-1.0k;
the rest of copy_pure's 4.2k G3 gap is farm side (D3 funding, D10 deposit timing, production day mix). Tick rules (ticksell) closed
(-4.9 / -5.3k). I1 learned seller matches M&M's per-hour decisions on held-out states (seller_diff: shares within 0.01-0.03, P(sell) by
drain phase within 0.02; units ~10-19% low). Line B integrated (src_dc12i): copy_pure_b G1 d2-5 plantings 0.954, fallback 2.3%.
Sep 30 03:12 I1 learned seller on G1s (vs M&M's own selling on its stock, 24 worlds): threshold through day 29 -1.53k, days <= 27 -1.31k,
model only on days 12-24 -2.13k, 6-24 -1.51k; DP -0.38k. No I1 variant beats the DP yet although it copies M&M's per-hour decisions far
better (seller_diff) -> not integrated; triage: what M&M's own selling does that the per-hour copy misses (end-game, under-selling ~0.85x,
stock hand-over). Line B final candidate: achieve=1 feedcost=1 reservenet=1 (principled next-dawn reserve; G1 d2-5 0.953 / 2.5%;
src_dc12i + m6 patch); arms copy_pure_afrn / copy_or_v5d_afrn on exact G3, league, G3-wide quick (60 worlds).
Sep 30 04:24 Package + line B (achieve=1 feedcost=1 reservenet=1 replacing reserve=0): league all 8 +1.14k (SE 0.46k), additive parts
(reservenet +0.57k, achieve+feedcost +0.55k); exact -3.07k (SE 1.02k, biased bed) -> swap / 760 / G3-wide quick / pinned 200 running.
I1 v5t full-game read (-3.84k) VOID: tracker bug in the hook (fixed in m8), rerunning. D11 plan (BC): input dump on own vs M&M states ->
make the network invariant to execution-artifact inputs (drop / coarsen / augment), keeping G2. Day 9: animalfirst=1 halves own-game
drops (0.85 -> 0.38); league pending.
Sep 30 04:54 D11 valued (Weaknesses, G3-wide 278, copy_or_v5d_af vs M&M's recording, production at M&M's daily sale prices): -1.7k per
game = melon -1.46k (d1-2 + land-day melons) > wool -0.93k (herd mix: +0.3-0.6 geese, -0.2-0.4 cows, -0.2-0.6 sheep from dawn 8) > tomato
-0.66k (wave 2 d15-19) > milk -0.35k; offsets eggs +1.14k / carrots +0.65k (overvalued on thin books). Intent logs: asked == planted on
d10-24, so these are the network's asks on its own states (BC). Top item: D11 by value melons > sheep mix > tomatoes.
Sep 30 10:35 PLAN DECISION: one valid full-copy Stage-2 test with the execution fixes (ss4 v3, slotcash, land-day plantings, sbc1
M&M-pinned). If it loses to the package on G3 218 + swap 234, switch the main line to porting M&M's measured traits into the package
(d1-3 melons, d6 herd mix, tomato wave 2, earlier wool), each via Stage 1.
Sep 30 10:50 astra-010: the "seller ~0.1k / farm ~1.6k" split (G3-wide loss minus G1s loss) is NOT an identified attribution:
seller and farm interventions change cash, capacity and the opponent's response, so the parts don't add. Use matched arms / per-world
factorial interactions instead. astra-008: the engine clears worker pockets at night (sim.hpp end_of_day), so dawn pockets are zero and
the slot fix v3 premise is wrong; the Day compiler is re-diagnosing the remaining h0 cut. astra-005 refuted by BC (opponent-grid
channels move tomato / carrot asks < 0.03 / day).
