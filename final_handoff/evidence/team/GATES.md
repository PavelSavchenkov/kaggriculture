# Gate table (owner: Imitation; others send results, Imitation writes)

Candidate = model dir (network + decode keys + .dc11 compiler keys). X = experiments/v10/sep29_mm_copy.

| Candidate | G1 (216 M&M games) | G1s seller (24-48 worlds, vs rec_s) | G2 | G3 whole / U12 / U18 (6 exact worlds; real M&M +9.49k) | League vs package | Date |
|---|---|---|---|---|---|---|
| X/models/d3crop_m68_m3 (submitted package, Kaggle 56690263) | FAIL 51 / 93 | base seller -1.10k (SE 0.46k); price per unit milk 110.5 / strawb 137.7 / wool 142.1 vs M&M 110.1 / 138.5 / 134.2 | FAIL 33 / 60 (216 games, fast) | +6.20k / +7.00k / (d3crop_m68 +11.35k) | 0 (+1.44k vs 8 agents) | Sep 30 02:15 |
| X/models/copy_pure (BC: M&M copy CPP5c_v3, all decode patches removed, opening style 7) | d2-5: plantings 0.806, fallback 39.9% | - | 56 / 71 (201 held-out) | +5.28k / +8.37k / - (U3 +4.40k, U6 +3.37k, U9 +4.31k) | -1.68k (3 strongest), -1.71k (all 8) | Sep 30 02:29 |
| X/models/copy_pure_m3 (copy_pure + package m3 compiler keys) | = package keys | - | = copy_pure | +0.00k / +6.44k / - (collapse) | -3.31k (3 strongest) | Sep 30 02:29 |
| copy_pure_or (+ optrate) | - | - | 69 / 71 held-out | +3.82k / +8.49k / - | -2.24k (3 strongest), -2.10k (all 8) | Sep 30 |
| copy_or_v4w3 | - | - | 72 / 75 (BC) | +3.52k / +7.81k / - | -2.48k (3 strongest, 36) | Sep 30 |
| copy_or_v5d (BC: recency-conditioned copy + optrate) | - | - | 70 / 71 (201 held-out), 73 / 75 (BC, 117) | - | - | Sep 30 |
| copy_pure_b (+ achieve=1 reserve=0 feedcost=1, build_dc12i) | d2-5 13 / 15: plantings 0.954, fallback 2.3%, cows -0.02 | - | = copy_pure | +2.99k / +6.90k / - | running | Sep 30 |
| copy_or_v5d_b | = copy_pure_b | - | = copy_or_v5d | +2.67k / +6.91k / - | running | Sep 30 |
| copy_pure_af (+ achieve=1 feedcost=1) | d2-5: plantings 0.903, fallback 4.1% | - | = copy_pure | +4.17k (-1.11k, SE 1.58k) | - | Sep 30 |
| copy_pure_r0 (+ reserve=0) | - | - | = copy_pure | +0.29k (-4.99k, SE 1.58k): reserve=0 is the loss | - | Sep 30 |
| M&M's farm + our seller (exact, same-hour fix) | - | -1.05k (SE 1.10k) exact; package seller -0.38k (SE 0.56k) G1s 24 worlds | - | - | - | Sep 30 |
| d3crop_m68_m3_solo (solo 12 17) | - | - | 35 / 60 | +6.26k / +9.57k / - | -0.20k (SE 0.35k); swap top-10 +0.86k (SE 0.44k), all +0.23k | Sep 30 |
| d3crop_m68_m3_lp10solo | - | - | 33 / 60 (d10 wheat 5.7 -> 9.1, tomato 0.1 -> 1.2; d8 unchanged) | +5.16k / - / - | -0.16k (SE 0.36k) | Sep 30 |
| d3crop_m68_m3_lp810solo | - | - | 34 / 60 | running | -0.38k (SE 0.42k) | Sep 30 |
| d3crop_m68_m3_v18 | - | - | - | +5.30k / +5.93k / - | +0.56k (SE 0.37k); swap top-10 +1.85k (SE 0.59k), all +0.42k (SE 0.30k); 760 -0.21k (SE 0.17k) | Sep 30 |
| regime w0.01 seller | - | -1.02k (SE 0.54k); M&M's evening share, day-average price lower (strawb 132.9 vs 138.5) | - | - | - | Sep 30 |

## Baseline detail: package d3crop_m68_m3

G1 fails (216 games): d2-5 plantings 0.964, cows -0.30, fallback days 15.5%, dropped 0.20 / day, harvest 0.95, care 0.94; d6-11
plantings 0.976, dropped 0.13, egg units 4.05x M&M (dawn share -43 pts), milk units 1.30x (dawn -28, evening +16); d12-17 evening
share strawberry +40 / milk +30 / wool +27 pts, egg units 1.59x, wool 1.24x; d18-24 dropped 0.48 / day, weeds +0.27, egg evening
-38 pts, units 1.47x. Passes: land (99.7-100%), herd d6+, field units, services d6+, next-dawn value +51..+363 $/day.
(Watering: total waters 0.80-0.96 of M&M is reported only; yield waterings are the check from the next run.)

G2 fails (20 games): land-day crop asks per game-day (M&M -> ours): d6 melon 1.8 -> 0.1; d8 tomato 3.4 -> 0, geese 1.7 -> 0.6;
d10 wheat 10.9 -> 5.1, carrot 3.0 -> 0.6, tomato 4.2 -> 0.3, geese 1.4 -> 0.1; d11-12 wheat / tomato ~70%; d12-24 wheat 0.82-0.85;
d1-5 cows +0.2 / day. Land ask recall 98%, precision 100%.

G2 options value by kind (scripts/g2_value.py; $ per game-day |diff| (signed net - label), d12-17 / d18-24), copy_pure: ongoing_harvest
328 (+287) / 474 (+222) = the net harvests ~13% more strawberry / tomato units (M&M leaves yield on the plant); feed 103 (+43) / 97 (+40);
care 111 (+47) / 114 (+52); one-shot harvest / fertilize ~100-150 (~0 signed); collect ~0 (valuation fixed: units over the held cap).

Other gate results (Sep 30): fieldflow seller f=0.7 G1s -4.54k (closed); skipwater funding fix: fallback 0% but plantings 0.48 (closed,
stop list); affordanimal cows -0.83 (closed).

G3-wide (Weaknesses, 278 held-out M&M worlds, the package live): M&M's recording +1.43k (SE 0.38k) vs the package, 67% wins; the package
in M&M's seat -1.35k vs the recording (SE 0.62k; 110 worlds so far). Copy arms running.
Seller diff (teacher-forced per hour, 201 held-out games x d10-27): same-hour overlap with M&M's units, DP 39-41%, I1 sampled 39-52%,
I1 threshold (v2t) 56-65%; P(sell) after drains M&M .43-.54, I1t .51-.63, DP .34-.39.

League vs all 8 (Sep 30 03:57, paired vs the package, build_dc12i): copy_or_v5d_af -0.49k (SE 0.43k, n132), copy_pure_af -0.92k (SE 0.50k),
copy_or_v5d_afrn -0.95k (SE 0.47k), copy_pure_afrn -2.03k (SE 0.52k), copy_pure -1.71k (SE 0.51k). G3-wide full (278): copy_pure vs
M&M's recording -1.56k (SE 0.46k); quick 60: or_v5d_afrn - package -0.90k (SE 0.63k). Best copy system so far: copy_or_v5d_af.

Package line (Sep 30 04:24; build_dc12i; league all 8, paired vs the package d3crop_m68_m3, n132):
| Candidate | League | Exact G3 | Guards |
|---|---|---|---|
| d3crop_m68_m3_afrn (reserve=0 -> achieve=1 feedcost=1 reservenet=1) | +1.14k (SE 0.46k), wins 73% vs 64% | -3.07k (SE 1.02k; biased bed) | swap / 760 / G3-wide quick (Weaknesses), pinned 200 (Day compiler): running |
| d3crop_m68_m3_rn (reserve=0 -> reservenet=1) | +0.57k (SE 0.30k) | - | - |
| d3crop_m68_m3_af (+ achieve=1 feedcost=1, reserve=0 kept) | +0.55k (SE 0.40k) | - | - |
| d3crop_m68_m3_r1 (reserve=0 dropped) | +0.19k (SE 0.44k) | - | - |
Copy line: G3-wide full (278) copy_or_v5d_af - package -0.40k (SE 0.27k); copy_or_v5d_af_v5t league read void (tracker bug), rerunning;
copy_or_v5d_af_a1 (animalfirst=1) queued.
Sep 30 04:46: d3crop_m68_m3_afrn FAILS the guards (760 -0.72k SE 0.20k; G3-wide quick -0.66k; swap level, top-10 +1.07k); prep
cancelled. copy_or_v5d_af_v5t (learned seller, fixed): league -3.07k (SE 0.47k) vs the package, quick 60 -1.73k vs copy_or_v5d_af.

Sep 30 08:58 package-line candidate table (paired vs the package d3crop_m68_m3; guards: league / swap 234 / G3-wide quick 60 / 760):
| Candidate | League | Swap (all; top 10) | G3 quick | 760 | Verdict |
|---|---|---|---|---|---|
| + feedvalue=1 (Day compiler; patch m11 for BC's bc_m3min) | +0.75k (SE 0.20k, n480) | +0.43k (SE 0.27k); +1.29k (SE 0.48k) | +0.71k (SE 0.46k) | -0.24k (SE 0.15k) | most balanced; 760 slightly negative |
| afrn (achieve feedcost reservenet) | +1.14k (SE 0.46k) | -0.01k; +1.07k | -0.66k | -0.72k (SE 0.20k) | fails |
| af (achieve feedcost, reserve=0 kept) | +0.55k (SE 0.40k) | +0.27k; +1.04k | -0.09k | -0.44k (SE 0.21k) | fails |
| depcredit 0.1 / 0.2 | +0.52k / +0.44k | - | - | -0.54k / -0.92k | fails |
| v18 main | +0.56k | +0.42k; +1.85k | -1.86k (SE 0.73k) | -0.21k | fails G3 quick |
Live 56690263 (package m3): vs top 10 n 11, 55% wins, +0.41k; rating ~2770.

Sep 30 09:57 copy line (paired vs copy_or_v5d_af; quick 60 = M&M's worlds with the package live; league = block 501, all 8, n96):
| Arm | G2 held-out | G2-own (own-game asks vs M&M) | Quick 60 | League |
|---|---|---|---|---|
| copy_or_v5d_af (base) | 69-70 / 71 | melons d1-3 3 vs 4; d6 strawb 11.9 vs 14.1; d6 geese / cows 2.63 / 1.38 vs 2.15 / 1.78; tomato wave 2 7.4 vs 10.0 | 0 | 0 (-0.06k vs package) |
| sbg (BC: seed + grid-off) | 68 / 71 | melons 4.0; strawb 14.4; geese / cows 2.07 / 1.77; wave 2 8.7 | VOID (baseline ran a wrong net), rerun | -1.41k (SE 0.48k); vs d3crop -0.10k |
| sbg + ss2 (slot fix v1) | = sbg | = sbg | VOID baseline, rerun | -1.36k |
| DP + ss2 (slot fix v1, Day compiler cp_ss2) | = base | = base | VOID (both arms ran a wrong net) | -1.30k (Day compiler; recheck window) |
| v5t (learned seller, no slot fix) | = base | = base | -1.73k (SE 0.42k; valid run 04:40; the -2.79k run is void) | -2.58k |
| sbc (BC: seed + grid type-channel blind) | 69 / 71 | as sbg | running (build_dc12l) | - |
| v5t + ss4 v2, sbc + v5t + ss4 v2 (full copy system) | - | - | running (build_dc12l) | - |
Beds disagree (league = our older hybrid lineage); the swap bed on sbg / ss2 decides (Weaknesses).
Sep 30 10:05 CONTAMINATION: copy_or_v5d/model.bin was overwritten through symlinks 09:06:30-10:03:02 BST (BC); every arm linking to it
(copy_or_v5d_af and aliases, the Day compiler's cp_* / cpk_*) ran a wrong network in games started then. Void entries marked above;
reruns on build_dc12l queued. sep29_mm_copy model files are read-only now.
