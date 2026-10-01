# Weaknesses analysis findings

Written only by Weaknesses analysis. Other sessions read it. Format: `- (time) finding. Number. Source.`
Working folder: `work/sep29_validation` (old: `work/sep25_bc_weakness`). Scripts named below are in `work/sep29_validation/analysis`
or `live/` unless a path is given.

## Findings

- (Sep 30 14:27) The converged top-team style does not come from the 8 tested public notebooks (work/sep29_validation/nb/; untested
  or private notebooks and a common reaction are not ruled out, astra-020). The 12 top-voted notebooks run
  Sep 25-30 were pulled and 8 agents extracted, then matched action by action against 80 replays of the converged teams: every agent
  matches only 9-15% of farmer actions (a same-lineage fork would be ~96%). The public agents share one opening (buy 8 wheat, sell 3);
  the converged teams open with 5 hires + cow + sheep like M&M. A field-like bed opponent needs a BC clone on Sep 27+ games instead.
  - (14:40) Stopgap opponent v5t (M&M copy + learned seller, judge.sh JUDGE_OPP=v5t): its selling hours and lots match the converged
    field much better than the package (night share 0.12-0.13 vs field 0.13-0.17 vs package 0.31-0.37; hour similarity ~0.92 vs
    ~0.78), but not eggs (evening 0.48 vs 0.85) or dawn wool stock (4.0 vs 6.3): a timing-matched sensitivity opponent, not
    field-representative (astra-029; swap/opp_qualify.py).
- (Sep 30 14:24) Swap bed extended to our live games vs origin ranks 31-100 (103 exact of 107) and 101+ (144 of 150), with package
  and copy baselines. Copy minus package by origin band: 1-10 +1,127 (SE 752), 11-30 -284 (455), 31-100 +1,246 (538), 101+ -428
  (435); exposure-weighted (live mix 0.11 / 0.24 / 0.23 / 0.41) +169 (SE 259). judge.sh now runs the bands after a swap and prints
  the weighted line (judge_wsum.py). Bands are origin ranks: in the swap bed the arm always faces our live sub (astra-020).
- (Sep 30 12:40) The top-30 field converged on one style on Sep 27-29, and M&M joined it on Sep 27 (work/sep29_validation/fc/;
  2,875 perspectives of ranks 1-30 + M&M, days 10-27; profiles.csv).
  - Hour-profile similarity to M&M's Sep 29-30 thin-product profile: M&M itself 0.79 (Sep 18-26) -> 0.97 (Sep 27-28); DSM / Vadim /
    DECEM ~0.9 -> 0.96; akmr / yuto / Gordeev / Mother-Goose 0.91-0.97 from Sep 27; Boey, Majkel, Anton Tikhonov, Just A game,
    TheEggman moved on Sep 29-30. Holdouts: mtmr_s1, THIRD FARM CLUB, 吃白饭的大肥鱼, keiz (night / big-lot milk and wool).
  - The farm moved at the same time: d6 plantings ~11-13 -> 19, d8 2-9 -> 15-17, geese at dawn 10 1-4 -> 6-7; eggs sold in the evening
    (h18-23 share 0.85-0.9); thin products at dawn / morning in small lots.
  - Our live opponents: ranks 2-10 ~14%, 11-30 ~17-27%, 31-100 ~21-33%, 101+ ~36-41%, M&M ~0. Forecaster data from before Sep 27
    describes a different field; a held-out Sep 28-30 set in fc32 format is in fc/hold/ (1,608 target seats, none in training).
- (Sep 30 12:06) Every full-game arm lifts the opponent; it is not a mirror artifact, and the gap is wool (swap/mirror_test.py,
  wool_straw.py; vs M&M's recording replayed against the same live package, g3w_rec).
  - Against M&M's recording the opponent earns about what it earns against the package mirror (+203, SE 379); against every
    candidate it earns +0.65 to +2.7k more. Same-hour overlap with the opponent falls for all arms incl. M&M, so overlap is not it.
  - The opponent's wool revenue, our games minus M&M's: copy +1,837, sbc1 +2,340, package +343, pk_fv +639, rn05 +689, mm6 +1,363.
    Timing (all arms): we sell wool late (h21-23 0.23-0.34 vs 0.10-0.19) in big lots (3.5-4.2 vs 2.2-2.8) with ~40% less dawn stock;
    the opponent then sells less wool at dawn (17-19 vs 31 units) and more midday at higher prices. Supply: copy-line arms make 6-8
    fewer wool units than M&M (fewer sheep-days, same wool per sheep-day); paired regression (astra-023, wool_paired.py) puts that at
    ~+0.7k (copy) / +0.9k (sbc1) of the opponent's wool gain, not 1.5k; the rest (~1.1k) is neither supply nor dawn share.
  - Strawberry is not the gap (opponent revenue -447 to +404).
- (Sep 30 11:55) Why pk_rn05 (rivalnight 0.5) flipped from G1s +741 to G3 -460 (swap/rn05_split.py, band_shift.py):
  - G3 (own farm, 107 worlds): our spend, hires, production, dawn stock and units sold are unchanged; ~70 units move from h0-20 to
    h21-23 and the reactive package opponent follows (h0-2 -35 units, h21-23 +41). Both sell the same units at night prices; the
    opponent gains more (+988 vs +564: milk +496, wool +372, strawberry +142, egg +150; d18-29).
  - G1s (M&M's recorded farm, 48 worlds): only ~5 units can move to the night (deposit hours are fixed by the recording), and the
    opponent leaves the night (-8 units) and loses -1,041.
  - G1s caps the candidate's timing shift and inverts the opponent's response: it is not a valid screen for seller changes that move
    timing. Screen them on our own farm (G3 with fixed cohorts or continuations from our own states).
- (Sep 30 11:50) G3 run orders (astra-018): swap/list_g3w218_mix.txt and list_g3w218_clean_mix.txt = the same worlds in a seeded order
  stratified by M&M sub (the old list put 48 worlds of one sub first); judge.sh uses them for new runs.
- (Sep 30 11:33) Stage-2 reads through judge.sh (fixed cohorts):
  - sbc1 (BC's M&M-pinned seed + grid-type-blind copy) on the clean 99, the confirmation set for G3-trained networks: vs the copy
    +81 (SE 460; own +973, opponent +892), score 58.6% vs 51.5% (+7.1, SE 6.0); no verdict. The quick-60 +1.13k did not hold.
  - pk_rn05 (package + rivalnight=0.5; G1s +741 on M&M's farm) on G3 218: REJECT at n=96; 107 pairs -460 (SE 451), own +321,
    opponent +781. The opponent-income gain seen on the seller bed reverses in whole games.
- (Sep 30 10:50) Swap bed, style-1 copy line (build_dc12k, manifests, gated). copy_or_v5d_af vs the package: +138 (SE 392), score
  61.1% vs 64.5% (SE 4.6), level; the usual band shape (ranks 1-10 +1,127, 11-30 -284). cp_ss2 (dawn-slot fix) vs the copy: stopped by
  seqwatch at 66 games, -324 (SE 350), score 53.0% vs 56.1%. sbg dropped (not a Stage-1 survivor).
- (Sep 30 10:15) Evaluation fixes (astra-003 / 004 / 007).
  - Ties: G3 ties are all package-vs-package mirror games (69 / 278), so earlier "wins X% vs 36%" reads overstated every arm. With
    half ties, G3-wide vs the package (48.4%): pk_fv 47.8%, copy_or_v5d_af 46.4%. On the swap bed vs the package (64.5%): pk_fv 61.5,
    v18 63.2, af 61.5, afrn 60.3, solo 59.8. All gain 6-16 pts vs ranks 1-10 and lose 8-10 pts vs ranks 11-30 (SE ~5). A same-build
    check of the package on dc12i vs the dc12h baseline: 58 / 58 games identical (stopped there), so not a build artifact. The 11-30 drop sits in worlds our sub originally
    won (package 72%, candidates 55-62%): the package shares our subs' lineage, so any deviation loses fit there. Read the swap bed by its
    all-234 score, not the band split. g3w_pair.py / swap_pair.py now print W/T/L, half-tie score, gained / lost.
  - swap/run.sh keeps a manifest per tag (sha of binary, model files, env): mismatched resumes are refused, mid-run changes warned.
    The 09:06-10:04 copy_or_v5d overwrite voided 128 swap baseline games (rerun) and my net_i identity check.
  - Quick 60 = development data. pk_fv: quick 60 +706, 218 complement -161 (SE 288). Clean lists and per-model membership are in
    mm/g3_membership.md; reserved confirmation episodes (created after 07:30 UTC) in mm/reserved_confirm.txt, refreshed per pull.
- (Sep 30 08:29) Day compiler's pk_fv (package + feedvalue=1; league +992, SE 268) on the 760: -241 (SE 152; own +49, opponent
  +290), all sub-beds slightly negative; not a clear fail. pk_dcso (depcredit + scenopen) stopped at 120 games, -1,179 (SE 385; own
  -927). pk_so (scenopen) 760 -168 (SE 118; own -554, opponent -386), closed. pk_fv swap bed: +430 (SE 272), ranks 1-10 +1,294 (SE 484; the real top team earns 1.3k less), 11-30 +61; wins 144 vs
  151. G3 quick +706 (SE 460; wins 57% vs 33%). Row: league +747, swap +430, G3 quick +706, 760 -241: three of four positive, the
  most balanced package-line candidate so far (v18 / af / afrn each had one strongly negative guard). (09:57) On all 278 G3-wide
  worlds it is level: +26 (SE 247); 56679033 -99, 56680759 +659. Only the league and the swap top-10 band stay clearly positive.
- (Sep 30 07:55) Early-deposit credit (Day compiler's pk_dc1 / pk_dc2, depcredit 0.1 / 0.2) fails the 760: dc1 -539 at 420 games
  (SE 218; own -71, opponent +469), dc2 -916 at 500 games (SE 203); G3 quick level (+52 / +351). The credit brings a few units into
  the shed earlier (pockets h3-11 -0.2..-0.3) but our DP still sells them in the evening, so there is no morning-sales channel.
  Closed by Day compiler (league +0.36k vs 760 -0.5..-0.9k, same bed conflict as afrn / af). pk_so (scenopen) also closed.
- (Sep 30 06:10) Production timing, live package (56690263, 20 games vs ranks <= 30) vs M&M (387 replays vs ranks <= 30;
  mm/prod_curve.py). Tomatoes d12-17: 0.1 vs 10.5 (M&M's d8-9 plantings give an early d16-17 harvest). Wool d12-17 34.2 vs 41.8
  (half the total ~1.5 days later; sheep at dawn 12 5.2 vs 5.7). Eggs 186 vs 238 (geese at dawn 9 2.0 vs 4.0). The package makes
  more milk (234 vs 195) and later melons (d18-29 18.3 vs 4.3). Strawberries match.
- (Sep 30 06:08) Thin-book day mix, d12-27 (units-weighted day price minus the plain mean, $/unit; mm/day_mix.py): M&M strawberry
  +5.6 / milk +8.0 / wool +18.4, its opponents -1.8 / +1.7 / +7.5; our live package (vs ranks <= 30) -3.9 / +6.6 / +5.2, its
  opponents +3.5 / +7.9 / +9.8. Everyone sells more on high-price days; M&M most for wool and strawberries. With Day compiler's G1s
  (our DP on M&M's stock is -102 vs M&M's selling), this gap is our stock timeline (harvest / deposit days), not the seller logic.
  - (06:10) Detrended (mm/harvest_timing.py): M&M's harvests follow the within-game price about as much as ours (wool 0.42 vs
    0.38 / 0.22, strawberry 0.04 vs 0.19 / 0.04, milk 0.24 all). The edge is production earlier in the game, when prices are higher
    (plantings, sheep placement), not the choice of harvest days.
- (Sep 30 06:08) d3crop_m68_m3_v18 on the G3 quick 60: -1,857 vs the package (SE 734; own -412, opponent +1,444). Its guard row:
  swap +420 (ranks 1-10 +1,850), 760 -207, league +0.56k, G3 quick -1,857 (the only strongly negative one).
- (Sep 30 06:04) Live 56690263 (package m3), 20 games vs ranks 1-30: ranks 1-10 6 / 11 wins, +412 (own 109.0k, opponent 108.6k);
  ranks 11-30 7 / 9, +5,746; rating 2770 (04:49 UTC). First non-negative top-10 margin of our subs (hyb_melon68 -1.8k, d3crop -2.6k,
  earlier windows); small n. Scorecard vs M&M's own games: M&M's late price edge per unit over its opponents is strawberry +15.7,
  milk +12.4, wool +23.5 (mostly day mix); ours -8.6 / -1.5 / +6.7 (live/scorecard.py).
- (Sep 30 05:44) d3crop_m68_m3_af guards vs the package: swap all +268 (SE 392; ranks 1-10 +1,044, SE 865), G3 quick -89 (SE 584),
  760 -439 (SE 205), league +0.55k (Imitation). Same shape as afrn: positive vs the real top 10 and the league, negative vs the
  clone beds; only the 760 is significant.
- (Sep 30 05:43) The G3-wide gap sits mostly in one M&M submission, 56679033 (swap/g3w_bysub.py).
  - 56679033 (79 worlds): M&M's recording beats the package by +3,558; arms vs the recording: package -3,614 (SE 688), copy_or_v5d_af
    -4,168, copy_pure -3,735.
  - 56680759 (115 worlds): recording +719; arms -0.8 to -1.2k. Older subs (84 worlds): arms about level with the recording.
  - The two newest subs have near-identical profiles. 56679033 plants more strawberries / tomatoes on d12-17 (8.6 / 7.9 vs 6.4 / 6.9)
    and fewer carrots late; live vs ranks <= 30 it makes +6.6k (100% wins) vs +5.0k.
  - BC's mm4 corpus (v4 / v4w3 / v5d copies) trained on 148 of the 278 G3 worlds; no in-sample advantage (swap/list_g3w_mm4in.txt /
    mm4out.txt).
- (Sep 30 05:41) M&M pull: 624 games (+30), same 5 subs, no new M&M-vs-ours games. Never in any training corpus: 83 (47 test/val +
  36 train-bucket); mm/mm_heldout_seat.txt 160, mm_testval_seat.txt 124 (mm/heldout_check.py, corpus_overlap.py).
- (Sep 30 05:19) G3 with a field-like opponent (quick 60, opponent seat = copy_or_v5d_af_v5t, which sells at M&M's hours;
  swap/v5opp.sh). copy - rec -1,591 (SE 709), package - rec -721 (SE 763), copy - package -870 (SE 479). Against the package
  opponent on the same 60: -2,835 / -1,918 / -917. The arm ranking holds, and both arms are ~1.2k closer to M&M against
  field-like selling. Against a dawn-selling opponent our DP moves strawberries to dawn (h0-2 share 0.35 vs 0.14).
- (Sep 30 05:10) copy_or_v5d_af_a1 (animalfirst=1), quick 60: vs copy_or_v5d_af +579 (SE 610; own +1,564, opponent +985; wins 38% vs
  45%), vs the package -338 (SE 636). It adds geese (dawn 8 3.03 vs M&M's 2.27), so the copy's goose bias grows and the cow / sheep
  gap stays. Noise-level; not a fix for the herd mix.
- (Sep 30 05:06) M&M's edge over ranks 2-30 is tomato volume (mm/mm_edge.py; 357 replays, 92% wins, +5,571 per game).
  - Revenue is almost equal (+549). Tomatoes +4,722 (142 vs 79 units), strawberry +1,349, wool +1,030, melon +681, milk +586.
  - Wheat / fertilizer revenue is -8.0k, but the opponents buy 13.9k of wheat and 2.3k of fertilizer to resell (churn); M&M buys
    5.2k and 0.
  - M&M spends more: hires +2.0k, land +2.7k (Q4), tomato seeds +0.4k.
  - Our live package also leads on tomatoes (158 vs 44 per game vs ranks <= 30, n 12). The copy's tomato shortfall (-21% plantings
    d10-29, all in wave 2) cuts into M&M's main edge.
- (Sep 30 05:05) G3 opponent caveat (g3w_rec, 278 worlds, d10-27): M&M's real opponents sell at M&M-like hours (h0-2 share milk
  0.31, strawberry 0.28, wool 0.29). Our package, the G3 opponent, sells in the evening (h18-23 milk 0.38, strawberry 0.41) and races
  at dawn when the arm does. A field-like G3 opponent (copy_or_v5d_af_v5t) was offered to Imitation (list swap/list_g3w60_v5opp.txt).
- (Sep 30 05:03) The dawn gap is the seller's choice, not the stock (G3-wide 60 worlds, d10-27; swap/opp_dawn.py).
  - The copy's shed at h0 is about M&M's (milk 5.5 vs 5.3, strawberry 9.0 vs 9.2, wool 3.8 vs 4.2), but it sells about half of it at
    h0-2 (milk 1.14 vs 2.62 per day).
  - Against v5t (M&M's hours), the live package doubles its dawn strawberry sales (4.23 vs 2.05 per day against the recording, same
    own stock, same opponent dawn volume); its h0-2 thin revenue goes 12.3k -> 18.6k. Our package is a reactive dawn racer that
    the frozen recording cannot answer, so an M&M-style dawn seller must win the h0 race, not only match M&M's hour shares.
- (Sep 30 05:01) G3-wide attribution for copy_or_v5d_af (278 worlds, vs M&M's recording; swap/opp_gain.py): margin -1,755 = own
  -313 + opponent +1,442, so ~80% of the gap is the package opponent earning more against the copy.
  - The package's revenue vs the copy minus vs M&M: +2,007 per game. Wool +1,837 (price +7-10 per unit at equal units), melon d12-17
    +770, strawberry / milk d12-17 +622.
  - Thin-product revenue by band (h0-2 / h3-11 / h12-20 / h21-23): M&M 20.1 / 23.3 / 22.4 / 14.7k, the copy 10.8 / 20.1 / 22.0 /
    28.7k. The package's h12-20 revenue is 23.1k against the copy vs 17.6k against M&M.
  - M&M's heavy h0-11 selling keeps the books low when the package sells in the afternoon; the copy holds stock to the evening.
  - Correction (05:10, revised 10:38 per astra-010): G1s (our DP on M&M's farm, -102) and G3 (-1,755) are different interventions,
    worlds and state paths, so "seller ~0.1k, farm ~1.6k" is NOT an identified split; BC's funded recorded-lot control moves cash by
    -2.2k and the old seller identity fails. The seller / farm share awaits the matched four-arm factorial (Imitation). "80% opponent"
    only says who gains.
- (Sep 30 05:00) Live Q3 slip (56690263, 71 games): 2nd land on d9 in 18% vs M&M's 11% (594 replays; M&M buys it at h6 on d8).
  Slip games carry less cash into d8 (dawn-8 234 vs 498). $0 dawns on d7-8 after the d6 land buy are the intended spend-down (M&M's
  dawn-7 cash is 89). live/q3_slip.py, broke_days.py.
- (Sep 30 04:56) The copy's early melons follow a fixed script. copy_or_v5d_af plants exactly 2 melons on d1 and 1 on d2 in all 278
  G3-wide worlds; M&M plants 4 over d1-3 in 83% of worlds (5 in 26, 3 in 17). The copy has more cash at dawn 2 (202 vs 68).
  - Melons are the copy's largest production gap at M&M's daily prices: -1,462 of -1,705 per game (swap/prod_value.py).
  - Next: wool -929 (from dawn 8: geese +0.3-0.6, sheep -0.2-0.6, cows -0.2-0.4), tomato -657, milk -345.
  - Offsets: eggs +1,141, carrots +654 (overvalued on thin books).
- (Sep 30 04:53) Why afrn loses the 760: dropping reserve=0 shrinks the day 3-4 strawberry plantings. 40 traced F2R games vs the
  package twin (work/sep29_validation/analysis/afrn/split.py, split2.py).
  - Strawberries planted d3 / d4: 2.7 / 2.0 vs 4.0 / 2.9; dawn-3 cash 224 vs 102.
  - First-wave harvest d12-17 -4.6 units (d13 3.8 vs 6.9), sold late at ~114 instead of ~190; our strawberry revenue d12-17 -765.
  - The opponent's thin products gain at the same units: milk +520, strawberry +503, wool +462 (it moves sales from evening to
    midday).
  Reserve=0's early spend wins the day-3-4 strawberry race (same channel as copy_pure_r0 and D6).
- (Sep 30 04:49) D11: M&M replants in waves; the copy has the same waves but plants carrots where M&M plants tomatoes on days 12-19.
  G3-wide 278 worlds, copy_or_v5d_af vs M&M's recording in the same worlds (swap/d11.py, wave2.py, wave2_cash.py).
  - Tomato plantings per game: wave 1 d10-12 (M&M 4.2 / 1.5 / 1.1), wave 2 d15-19 (M&M 7.8, copy 5.0; on d19 40% vs 18% of games),
    total d10-29 15.5 vs 12.2. Tomatoes produced d24-28: 53 vs 37.
  - Carrots: same total (58 each), but the copy plants +3.1 on d12-18 and M&M ramps later (d24-26) in bigger lots.
  - Strawberry d14: 0.62 vs 0.08. Wheat total -5.5 (d10, d12).
  - Not freed plots: planted tiles at dawn 12 / 16 / 20 are equal (75-76). On 158 games with equal melon harvests d15-18 the gap is
    the same (tomato 5.0 vs 7.6, carrot 14.3 vs 11.7).
  - Not cash: dawn-15 cash 23.3k vs 23.1k. Wave-2 tomatoes do not depend on the cash tercile for either farm.
  - Not the opponent: both farms follow shared world signals (M&M vs its real opponent's tomatoes r 0.49, the copy vs our package
    r 0.45). The copy's deficit is a level shift (-3.5 at equal opponent tomatoes) that does not grow with the package's tomatoes
    (swap/wave2_opp.py).
  So it is a crop choice (network ask or compiler substitution; Imitation's intent run splits it).
  - Why it matters (rules: tomatoes produce at ages 8-11 only): wave 2 is M&M's only tomato source on d23-29, where the copy makes 37
    vs 53 tomatoes (d24-28). In 56679033's worlds the copy's d12-17 tomato plantings are 5.1 vs 8.7 (56680759's: 4.8 vs 6.9).
- (Sep 30 04:48) The d3crop_m68_m3_afrn 760 guard fails: -718 (SE 202) vs the package's clone twin. Own income +94, opponent +812.
  F2R -1,407, F3R -544, top-clone -231. Imitation cancelled the candidate. The d3crop_m68_m3_af 760 (reserve=0 kept, 05:26): -439
  (SE 205; own +507, opponent +946; F2R -1,055, F3R -384, top-clone +111), also fails.
- (Sep 30 04:43) The M&M seller pays only with M&M's farm timeline. G3-wide quick, 60 worlds (swap/traj.py adds a selling / deposit
  table).
  - copy_or_v5d_af on our seller sells 2-2.5x M&M's evening share of strawberries / milk / wool (strawberry 0.50 vs 0.19 at h18-23)
    and deposits late.
  - BC's learned M&M seller (v5t) reproduces M&M's hour shares (milk 0.26 / 0.26 / 0.31 / 0.18 vs 0.31 / 0.25 / 0.25 / 0.19), but
    loses -1,725 (SE 419) vs our seller on the same farm: own -2,775, opponent -1,050.
  - recsell: M&M's recorded farm with our seller was 2.4-2.8k worse than M&M's own selling. (10:38: UNVERIFIED per astra-001: the
    DUEL_REC replacement path appends our sales after hires / purchases, changing order positions; the seller identity fails.)
  So the M&M-style seller needs M&M's stock timeline: production mix and early deposits. The farm trajectory (melons, tomatoes,
  deposits) is the prerequisite.
- (Sep 30 04:40) Guards for d3crop_m68_m3_afrn vs the package: swap bed -7 (SE 344; ranks 1-10 +1,072, 11-30 -467); G3 quick -663
  (SE 692). Own income is up on every bed but the opponent gains about as much.
- (Sep 30 04:00) G3-wide full, 278 held-out worlds, the package live, vs M&M's frozen recording: package -1,351, copy_pure -1,561,
  copy_pure_af -1,877, copy_or_v5d_af -1,755. The copies are 0.4-0.6k under the package in margin but win more (44-46% vs 36%).
  They all give back 2.8-4.4k to the recording on days 12-17 (tomato replanting stops, late melons short).

- (Sep 30 03:22) G3-wide quick screen: 60 held-out M&M worlds with the package live (work/sep29_validation/swap/quick.sh,
  q60_read.py, traj.py). Paired vs M&M's frozen recording (rec) and arm vs arm, all on the same worlds.
  - vs rec: copy_pure -2,747 (SE 825; its own income +858 but our sub +3,606), package -1,918, copy_pure_afrn -3,821, copy_or_v5d_afrn
    -2,822.
  - Arm vs arm: afrn - copy_pure -1,074 (SE 653; own -2,291); or_v5d_afrn - copy_pure -75; or_v5d_afrn - afrn +999 (SE 630).
  - Trajectory vs M&M's own recording in the same worlds:
    - melons: plantings on days 0-5 are 0.79 of M&M's (the day 1-2 melons are missing), production on days 12-17 0.50 (~1.5-2k
      per game);
    - tomatoes: plantings on days 6-17 are 0.70-0.77 of M&M's, production on days 12-17 0.52;
    - eggs +13-28% (more geese at dawns 8-10), carrots +15-25%;
    - wheat sold on days 6-11: 1.3 vs M&M's 16.

- (Sep 30 02:46) I5: does M&M react to the opponent when selling? Checked on 594 M&M games, days 10-27, thin products
  (work/sep29_validation/mm/i5/). Hardly.
  - M&M's hour shares are the same against dawn, evening and through-the-day opponents (strawberry h0-2 / h3-11 / h12-17 / h18-23 =
    0.27 / 0.20 / 0.33 / 0.19 vs 0.26 / 0.22 / 0.32 / 0.20).
  - Within a game, M&M's h0-2 units rise only +0.03-0.055 per unit of the opponent's dawn stock; its own stock counts +0.15-0.29.
  - Teams' prices facing M&M vs facing our subs differ by -3.4 (milk, strawberry) and +9.3 (wool) per unit, and noisily.
  So M&M applies a fixed hour profile to its own stock, with a weak dawn response to the opponent's stock.

- (Sep 30 02:38) M&M data for the imitation plan (work/sep29_validation/mm/; pull every 3 h via pull_loop.sh):
  - 594 M&M games with traces from 5 M&M submissions: 56624842 141, 56640167 127, 56658903 94, and the newer 56679033 116 and
    56680759 116. Lists: mm_all(_seat), mm_vs_ours(_seat).
  - Held out for G2: mm_testval_seat.txt (BC's test / val hash buckets, never trained; 117) and mm_heldout_seat.txt (plus
    not-yet-trained games; 278).
  - M&M rarely meets us: 8 M&M-vs-ours games in 4 days, so an exact G3 set stays near 6-7 worlds. Proposed G3-wide: all M&M worlds,
    our arm vs M&M's frozen recording, both against the same live sub.
  - Ledger copy with LEDGER_YIELD (tools/ledger_yield): per dawn, tomato / strawberry plants and stored yield, and harvests per hour.
- (Sep 30 02:42) M&M reference profiles (gate targets; mm/profile_*.csv, profile.txt, mm_profile.py), 594 games. They are stable across
  all 5 M&M submissions.
  - Land: Q2 day 6.0 at h3.8, Q3 day 8.1 at h5.8, Q4 day 9.6 at h8.6.
  - Plantings: d6 strawberries 15, d8 wheat 10 + tomatoes 2.3, d10 wheat 13 + tomatoes 4.4 + carrots 1.4.
  - Thin sales by hour band, days 10-27: strawberry h0 18% / h12-17 34% / h21-23 13%; milk h1-2 23% / h21-23 16%; wool h3-11 38%;
    eggs 52% at h21-23.
  - Feeding: M&M feeds less often than its opponents: cows 69-72% of days vs 75%, geese 79-81% vs 88-91%.
  - Yield left on plants at dawn: 0.64 per strawberry plant and 0.75 per tomato plant, vs 0.57 / 0.66 for its opponents. M&M
    harvests later in the day (strawberry: 0% at h0-2, 37% at h18-23).

- (Sep 30 02:25) M&M plan item 6, D3 root cause (early funding). The live package on M&M's own intents (teacher_day label mode,
  build_dc12h, days 2-5, 216 games; work/sep29_validation/mm/d3/):
  - fallback rates are 19 / 35 / 8 / 0% on days 2 / 3 / 4 / 5;
  - the funding variants that buy at h0-9 fail on "cow short": the cash is not there yet;
  - the deferred variants (buying at h10-23, when cash would cover it) fail on "unit action fails" in 83-88% of fallback days,
    because delay_purchase moves the buy hour but not the cow's pickup / placement stop.
  Fix: re-route the animal's stops after the deferred purchase. M&M places its day 2-4 cows at h14-17 anyway, and production counts
  from the placement day.

- (Sep 29 16:20) hyb_nolp was not better than our lineage against the live top 10. Ranks 1-10 5 / 17 vs rl-v179 5 / 20 over the
  same hours; rating ~2820 vs ~2833; P(win) vs a 2800 opponent 0.57 vs 0.50; its same-window margin lead (+2.5k) came from ranks
  11-30 and blowouts. Bed gains vs top-team clones (+0.4 to +1.5k over d3crop, pinned +1.5k) did not transfer. live/q329, q337.
- (Sep 29 14:45) Head-to-head over our whole dc11 lineage (271 live games vs teams ranked <= 15): 5 / 51 wins vs the top four
  (M&M 0 / 7 -10.0k, DECEM 1 / 13 -5.7k, DSM 2 / 17 -4.6k, Vadim 2 / 14 -4.9k per game). Thin-book price -9.1 to -9.9k per game,
  half won back by our volume; eggs -1.7 to -3.1k by volume. q331 / q332.
- (Sep 29 17:45; scale corrected 18:43: these 51 games are mostly older agents) Herd timing vs the top four (51 games): cows at dawns 4 / 6 / 8 theirs 4.0 / 5.5 / 6.9, ours 3.0 / 4.0 / 6.4;
  equal by dawn 10, then we hold more cows / sheep. Geese 7.6-8.0 vs 5.0-6.3 from dawn 10 (network asks fewer: 5.7 on days 1-9).
  q334_herd_by_day.py.
- (Sep 29 17:45) dc11 drops the network's early cows. d3crop's 90 live games, replayed exactly from our dawns (teacher_day
  TEACHER_NET=1 DC11_INTENTLOG=1): intent 7.83 cows on days 1-9, bought 6.19; the day-3 cow is asked in 93% of games and bought
  in 4% (dawn cash $36-92, "stress order ... item 10 short at hour N" on every variant). q336_own_intent_drop.py, herd/own_d3.*.
- (Sep 29 17:45) With M&M's own intents from M&M's dawns (list_mm40), our compiler ends days 2-4 with 0.72 / 0.70 / 0.48 fewer
  cows than M&M did (-2.40 per game by day 9, sheep -0.80). stress=12 / 23: -2.35 (not the cause). cashsell + wheatcash: -1.68
  (day 2 fixed, day 3 still dropped with "cow short" + "unit action fails"). q335_herd_teacher.py, herd/label_*.csv.
- (Sep 29 18:00) The early cash engine: on days 2-6 the top four sell $493 / 572 / 650 / 683 / 756 of fertilizer per day vs our
  $461 / 516 / 523 / 610 / 523, with 5-6 hires vs our 4 (days 2-5), and enter dawns 3-5 with $28-71 vs our $140-218 idle.
  M&M buys the day-3 cow at h8 from same-day fertilizer sales. herd/fert.py.
- (Sep 29 18:10) The trim order decides which intent is lost when funding fails; neither choice funds both. On M&M's dawns,
  d3crop's line (cropfirst default 1: animals trimmed first) loses 2.28 cows and 4.55 plantings per game on days 1-9; hyb_nolp's
  line (cropfirst=6: crops first before day 6) loses 0.57 cows but 8.28 plantings. herd/net_*.csv.
- (Sep 29) Validated candidates (760 clone games; wide = 53 real-world duels): hyb_melon68 +191 (SE 86) / wide +0.46k; d3crop_m68
  +431 (SE 137, all 8 sets) / wide +1.62k (own-production gains); dropany=6 on d3crop_m68 +34 (SE 135) / wide +0.19k (level).
  validate/, wide/.
- (Sep 29 18:32) giovanni-d3crop-cma-decode (live 56682206): vs d3crop +627 (SE 163, all 8 sets, own +603 / opp -24) on the 760
  clone games, but only +335 (SE 479) on the wide bed; vs d3crop_m68 +196 (SE 151, denial: own -406 / opp -602) on clones and
  -1,285 (SE 565) on the wide bed. The wide bed matched live before, so d3crop_m68 stays the stronger Local-LB candidate.
- (Sep 29 18:24) Cash path, days 2-4, recorded play (teacher_day TEACHER_HOURLY=1; M&M 40 games vs our d3crop 90 live games;
  analysis/cash_path.py, herd/hourly_*.txt). M&M: dawn cash $38-61, 6 hires, cumulative revenue by the end of h2 / h5 / h8
  $122-291 / $220-394 / $415-606 (60-80% of the day), one ~$400-415 purchase (a cow) per day at h7.6-9.5 paid by $311-523
  raised since dawn. Ours: dawn $71-234, 4 hires, by h2 / h5 / h8 $2-170 / $12-215 / $43-263 (7-36% of the day); the day-2
  purchase at h12, day 3 a large purchase in only 18 of 90 games (h15.8). Day totals are similar (days 2 / 4: $514 / $812 vs
  $518 / $738; day 3 $774 vs $581): the difference is WHEN the money comes, not how much. Spec for dc12 funding: raise ~$400 by
  h8 on cow days (fertilizer + stock sold from h0 with the crew sized for it) and buy then.
- (Sep 29 18:26) hyb_nolp's live games (114; cropfirst=6) buy a cow on every day 2-4 but at h12.8 / 12.2 / 10.8 (M&M h7.8 /
  7.6 / 9.5), from $540-580 raised by then; revenue by h8 $99-272 (M&M $415-606), 4-5 hires. So trimming crops first keeps the cow
  by buying it after midday; the morning cash is still missing. analysis/cash_path.py herd/hourly_hyb.txt.
- (Sep 29 18:27) What pays for M&M's morning purchase on days 2-4: fertilizer and dawn wheat, sold before noon. Units per game-day
  (h0-2 / h3-11 / h12-23): M&M fertilizer 0.7 / 5.2 / 0.2, wheat 4.6 / 0.1 / 0; ours (d3crop) fertilizer 0.2 / 2.4 / 2.8, wheat
  1.4 / 0.6 / 1.2; hyb_nolp fertilizer 0.2 / 2.7 / 2.8, wheat 1.1 / 0.8 / 1.5. The day's fertilizer total is similar (6.1 vs
  5.4-5.7); we sell half of it after noon. No milk / egg / strawberry / wool sales on these days. analysis/morning_sales.py.
- (Sep 29 18:40) The day 2-4 cow fails on funding, and the missing morning cash is fertilizer our routes carry. Our dc11 compiling
  M&M's own intent from M&M's dawns (list_mm40; teacher_day label + DC11_EXECDEBUG):
  - M&M buys the cow at h6-9.5 with $263-473 in hand and places it at h14-17.
  - We place it on 20-40% of those days (cashsell / wheatcash 10-75%), buying at h11-12 with $566-664 in hand, ~1 h before the
    cow pickup. "Cow short" appears on every funding variant up to h14.
  - Same dawn, same intent, revenue by h8: M&M $415-606, ours $90-205. Our workers collect 3.6-5.4 fertilizer by h8 (M&M 5-7) but
    deposit only 0.9-1.9 (the shed is empty at h8; each deposit sells within the hour; deposits catch up by h14), ~$250-330 carried.
    Dawn wheat on days 3-4: M&M 5.8-7.0, ours 0.8-0.9.
  - Placement hour does not matter: production counts from the placement day. analysis/cowhour/.
  - Same in our live play (traces; collected / sold fertilizer by h8, days 2 / 3 / 4). M&M 5.0 / 6.0 / 7.1 collected, 4.0 / 4.2 / 3.5
    sold. d3crop 4.3 / 5.1 / 4.0 collected, 1.3 / 0.3 / 1.2 sold. hyb_nolp 4.3 / 4.9 / 5.2 collected, 1.0 / 1.3 / 2.0 sold. By h11 M&M has
    sold everything it collected; we still carry 1.3-2.9. cowhour/live_carry.py.
- (Sep 29 18:43) CORRECTION of scale: for the current agents the herd gap is small. The "-1.5 cows on days 4-8" (17:45) pooled the
  whole dc11 lineage, mostly E / D / v17 games. d3crop + hyb_nolp vs ranks <= 30 in the same games (q334, 84 games):
  - cows -0.2 at dawns 4-6 (d3crop alone -0.6 / -0.8; hyb_nolp level), level by dawn 8, ahead from dawn 12 (+0.4-0.7);
  - geese -0.4 / -0.9 / -0.4 at dawns 8 / 10 / 12, then -0.3; sheep level or ahead.
  - A cow placed one day earlier makes +1.1 milk and +0.8 fertilizer over the game (ledger slopes; ~$190).
  So the remaining herd gap is worth ~$0.2-0.6k per game, much less than the thin-book price gap vs the top four (-9 to -10k per
  game). The day-3 cow drop is real, but d3crop buys it on day 4 instead. The morning-cash facts above stand; their value through the
  herd is small. cowhour/cow_value.py, q334_herd_by_day.py pkd3 pkhyb.
- (Sep 29 18:48) The thin-book price gap for the current agents: d3crop + hyb_nolp + rl-v179 vs ranks <= 10, 54 live games, exact fills
  from the ledger sales rows.
  - Animals bought are equal (cows 8.3-8.4 vs 8.0-8.4).
  - Price gap per game on strawberry / wool / milk / melon: d3crop -8.0k, won back by volume +7.0k. hyb_nolp -5.2k with volume
    -0.1k, net -5.4k: this is why hyb_nolp did worse vs the top 10. rl-v179 -9.6k / +7.3k. Wool is -2.8 to -2.9k for all three.
  - Both farms already sell mostly in the first turn after a shop drain (hour % 4 == 1).
  - The gap sits at dawn: h0-2 $/unit ours vs theirs milk 84 vs 113, strawberry 98 vs 142, wool 64 vs 110 (~-3.3k per game).
    Evening prices are at parity or better for us (milk 92 vs 82 at h18-23).
  - Days 10-27, the first seller of the day wins. They sell 29.8 strawberries per game at h0 ($148, before the town drain); we sell
    2.3. We sell at h1: 24.9 at $95, against their h1 $138. On 44-48% of thin product-days they sell at dawn and we don't; we sell
    first on 2-3%. They sell only 33-41 units per game of each at h18-23; we sell 52-116.
  - Within-turn slot order (theirs 0.1-0.4 vs ours 0.5-0.8) is worth only ~$0.25k per game.
  - So the lever is the overnight carry into a h0 sale (the pocket-carry / dawn-sale gap), i.e. the seller's model (dc12), not
    herd size. analysis/thin/ (drain_phase.py, same_turn.py, slot_order.py, dawn_race.py).
- (Sep 29 18:50) Margin vs ranks <= 10 by day block, current agents (same 54 games; margin_blocks.py). Ours - theirs per game:
  - total -1.8k (d3crop -2.6k, hyb_nolp -2.1k, rl -1.5k);
  - days 0-9 +0.4k;
  - days 10-13 -5.0k: 4th-quadrant land -2.7k, animals -0.7k, seeds -0.6k;
  - days 14-19 -3.9k: melon -1.8k, thin -1.7k, wages -1.1k;
  - days 20-24 +2.9k (tomato +4.2k) and days 25-29 +3.8k.
  - Melon race (melon_days.py): day-6 melon plantings theirs 2.91 vs ours 1.76 (we plant 1.8 on days 10-12 instead). On days 16-17
    they sell 19.1 melons at $152, we sell 10.7 at $121 and 11 later at $98-117. No shop drains melons, so the first seller keeps
    the price. hyb_melon68 (live) targets this gap.
  - By the opponent's plan (SPLIT=1): vs 4th-quadrant teams (18 games) -3.0k per game (thin -1.7k, wages -1.1k; their tomatoes
    sell first on days 14-19, -1.1k); vs 3-quadrant teams (36 games) -1.2k (our 4th quadrant nets about +2.8k: land -4.0k,
    tomatoes +7.9k, carrots +2.3k, minus wages; thin -2.2k, melons -1.1k).
  - Per team (q331, small samples; ours - theirs per game):
    - M&M 0 / 2, -9.3k: milk -3.8k, eggs -2.0k, melons -1.7k, wool -1.3k, tomatoes -1.1k.
    - DECEM 0 / 4, -7.5k: wheat -4.4k, wool -2.5k, eggs -1.5k. The wheat item is noise: over 12 lineage games DECEM's wheat is
      +0.2k (q331 over pkd3 / pkhyb / pkrl / econm6 / v17 / E / D, 146 top-10 games: margin -1.9k, wins 43).
    - DSM 0 / 6, -6.1k: land + wages -5.1k (our 4Q vs its 3Q), wool -1.5k, eggs -1.1k.
    - Azat 5 / 5, +3.1k.
  - 4th-quadrant tomato timing (network side, for BC):
    - In these games DECEM and M&M plant 8-9 tomatoes on days 8-10; we plant 0.1-0.4 and put 5-6 in on day 11.
    - We buy land on the same days and hours (day 8 h6; day 10 h10 vs their h8.5), but leave it empty that day: empty owned tiles
      at dawn 9 are 14.2 vs 5.0, at dawn 11 27.2 vs 13.0.
    - d3crop's intent asks 0 tomatoes on days 5-8 (7.3 wheat on day 8), and the compiler plants everything asked.
    - Their tomatoes sell from day 16: 16.7 units by day 19 vs our 1.7. thin/q4_race.py.
- (Sep 29 18:53) The overnight carry is the same; the h0 decision is what differs. Same 54 games, days 10-27, per product-day:
  dawn shed stock theirs / ours strawberry 10.6 / 9.3, milk 5.8 / 5.6, wool 6.1 / 3.9. They sell at h0 on 23% / 16% / 5% of days
  (lots 7.1 / 4.5 / 2.7 units); we do on 2% / 2% / 1%. So we hold the same stock past h0 and sell it at h1, after their lot.
  thin/dawn_stock.py.
- (Sep 29 19:05) Imitation's order-slot cap holds in our live traces. The engine takes at most 10 orders per turn, and our h0 hire
  wave fills them, so the seller's dawn sales are dropped. d3crop + hyb_nolp, 204 live games, days 10-27, per h0 turn:
  - ours: 9.94 orders, 9.58 hires, 0.17 sells (0.05 thin), at the cap on 95-96% of dawns;
  - opponents: 9.7 orders, 8.1-8.3 hires, 1.2-1.3 sells (0.46 thin), at the cap on 82-83%. They keep 1-2 slots for sales.
  This is a mechanical cause of the dawn gap in every dc11-lineage agent, including the live ones. saleslots (Imitation's key)
  is ported to old/bcr (tree_patches/port_saleslots.py, build_rs); the 760-game run on d3crop and d3crop_m68 is queued
  (validate/queue_sl.sh). thin/h0_slots.py.
- (Sep 29 19:15) The top teams sell at h0 selectively, on crowded days only. Same 54 games, days 10-27, per opponent product-day
  (thin/h0_when.py):
  - strawberry P(h0) is 0 when their dawn stock is <= 2 and 0.32-0.34 at 6+ (lots 5.9-8.2);
  - it rises with OUR dawn stock (0.06 at <= 2, 0.38 at 10+);
  - it is 0.45 when the book rose >= 5 since yesterday's h0 (flooded) and 0.02 when shops drained it by > 10;
  - the price level has no effect;
  - their h0 thin sells sit in slot 0 (93%; h0_order_pos.py).
  saleslots=1 lets our seller sell at h0 every day (9.4 units / day on M&M's intents), and it lost on Day compiler's pinned live bed
  (200 games, -1.2k, own -1.7k). The slot fix is necessary but not sufficient. The h0 decision needs the crowd condition, and the
  slot reservation should cover only the planned h0 sales (we hold 2.7-2.9 stocked products per dawn; a slot for each moves ~2.4
  hires to h1).
- (Sep 29 19:15) Caveat for local beds: the clone opponents are dc11 agents with the same h0 cap (one F2R game: h0 capped on 100% of
  days, 0.06 sells). The 760 bed, the league and the wide bed (opponent = our live sub) therefore test the fix against an
  uncontested dawn; only the pinned live bed has real h0 lots. A contested subset (clones with saleslots=1) is running
  (validate/contested_ss.sh).
- (Sep 29 19:18) The seller items on the stop list were judged with the h0 cap in force: dawnfirst, racing, lot caps, M&M's hourly
  table, dawnwait. On 12-hire days our dawn sales could not execute, and the bed opponents were capped too, so those losses are
  not evidence against dawn selling once the slots exist. Re-judge on the contested bed or live.
- (Sep 29 19:18) The current agents' thin-book gap is within contested days, not day choice (thin/wool_days.py, days 10-29 vs the
  top 10). On days both sell, per unit ours vs theirs: wool $105 vs $129 (8.1 days per game, ~-2.3k), milk 85 vs 96, strawberry 122
  vs 133, with bigger lots on our side.
- (Sep 29 19:35) Our lots on contested days are bigger at every hour, not only at dawn: wool 11.8 vs 9.4 units per day,
  strawberry h1 lots 5.6 vs 4.0, wool h1 4.2 vs 2.3. This fits Day compiler's finding that rival=1 (a denial credit on the
  opponent's forecast units) inflates our lots. rival=1 has been in every live agent since ib2t (Sep 27) and was adopted on beds
  where both sides were capped at h0 and the opponent was our own seller. The contested ss2 run includes saleslots=2 + rival=0.
- (Sep 29 19:41) Why saleslots loses, from a traced contested slice (d3crop_m68 +/- saleslots=1 vs the DSM / M&M clones with
  saleslots=1, 20 games identical to the bed; exact fills; analysis/contested/). The loss is lost production, not dawn prices.
  - The dawn shift pays: our non-wheat sales move h1 -> h0 (+150.5 units at h0, -147.8 at h1), net +$1.3k per game.
  - Production falls when the first hire wave shrinks: harvested wheat -13.4, carrot -13.8, egg -5.0 per game. Units sold -32.8,
    evening sales -26 units (-$3.8k), total revenue -$1,061.
  - The opponent gains +$612, mostly milk and strawberries in the evening.
  - Contested bed so far (80 games): -1,276 (SE 424).
  A slot fix needs to keep the crew's work: the displaced hires' stops re-routed, not dropped.
- (Sep 29 21:10) dc12 milestone m1 (Day compiler, build_dc12_m1): d3crop_m68 + dropany=6 cashsell=1 wheatcash=1 reserve=0
  saleslots=3.
  - Contested bed (clones also on m1's keys, 160 games): +930 (SE 418), own +464 / opponent -466.
  - saleslots=3 alone on the contested bed: +366 (SE 254, n 130); the every-day version was -1,276.
  - Standard 760, final: +1,097 (SE 174), own +338 / opponent -759; F2R +1,178, F3R +1,334, top-clone +739; all eight sets +.
  - Wide bed (53 real worlds, our sub as the opponent; the bed that matched live before): -1,243 (SE 863). Own +603, but the sub
    gains +1,846; wins 36 vs 44. A per-key split on the wide bed is running (wide/m1_split.sh).
  m1 is positive on the pinned, standard-clone and contested beds but not on the wide bed. The opponent's gain points at a
  component that helps the other side.
  - Wide-bed split (wide/wide_split.py): m1 sells more at h0-5 (+$2,234 per world). The sub, a capped evening seller that
    forecasts us, moves its strawberries into a thinner evening book (+$1,884 on strawberries; its h18-23 sales +$4,601). This is a
    game effect against a capped follower, not a bug.
- (Sep 29 21:40) Per-key split of m1 on the wide bed (Day compiler's read of my runs, vs the d3crop_m68 rerun, 53 worlds):
  - m1: -1,243 (SE 863);
  - m1 minus wheatcash: -594 (794);
  - m1 minus cashsell: +30 (851), own +852 / opponent +822;
  - m1 minus saleslots=3: -951 (837).
  The channel is cashsell (early funding sales feed the capped evening follower), not the dawn slots. Each arm is ~1.5 SE from
  its neighbour, so this is suggestive. dropany=6 alone +189 (620), the same as the earlier v94 read.
  - But m1 minus cashsell loses most of m1's contested gain. Same clones and seeds, 160 games: vs d3crop_m68 +231 (SE 430), vs m1
    -698 (SE 385). Standard 760, final: vs d3crop_m68 +437 (SE 174), vs m1 -660 (SE 170).
  - So cashsell is worth ~+0.7k against dawn sellers (94% of the live top 30) and costs ~-1.27k against a capped evening follower
    (~1-6%). Weighted by the live mix, m1 with cashsell is better. Day compiler's cashseeds=0 may keep the funding without the cost.
- (Sep 29 23:05) m2 = m1 + nighttrim=1 (Day compiler's recommended package). Paired vs m1: 760 -19 (SE 60), level; contested +268
  (SE 129), own +436. vs d3crop_m68 +1,078 on the 760. Live waste audit (thin/waste.py; 206 games vs ranks 1-30, ours vs theirs per
  game):
  - discarded items 5.1 vs 11.1;
  - missed fertilizer 20.0 vs 6.9, but only on days 20-29 at $10-13 per unit (~$150);
  - decay-to-weed plants 16.1 vs 8.0 (strawberries at end of life);
  - held-cap, care-bonus, escape and unfed losses at parity or better.
  So there is no large production-then-waste loss in live play beyond what nighttrim covers.
- (Sep 29 23:05) Seat-swap bed (Imitation's proposal; work/sep29_validation/swap/): our subs' 249 live games vs ranks 1-30 (77 vs
  ranks 1-10). The arm takes the top team's seat and our real sub plays live, with exact reproduction checked per game. Arms:
  d3crop_m68, d3crop_m68_m1, hyb_v3m68_ms_m1, CPP5c_v3_m1. Per game: arm minus the real top team (own), sub minus its real result
  (denial).
  - 234 of 249 games reproduce exactly.
  - d3crop_m68 in the top-10 teams' seats does as well against our subs as the real top-10 teams did: +83 (SE 818). Against ranks
    11-30 it does +4,936 better.
  - m1 vs d3crop_m68, paired: ranks 1-10 -1,153 (SE 655) without one collapse (own +181, our sub +1,334); ranks 11-30 +280 (SE 453).
  - A collapse (Imitation's find): in 114911253 vs akmr, m1 ends day 8 at $10 after buying Q3 (reserve=0 + cashsell). Days 9-10 are
    unfunded, the executor does nothing, and all animals escape (-186k). It happened in 1 of 234 swap games, 0 of 1,840 clone games
    and 0 of 318 wide games. m1 / m2 need a survival floor on unfunded days and a next-dawn cash floor.
  - (Sep 30 00:10) Fixed by Day compiler's survivalfloor=1: only the collapse game changes on the swap bed (arm $10 -> $98,338);
    the other 233 swap games and 160 clone games are identical to m1. m1 + survivalfloor vs d3crop_m68 on the swap bed: ranks 1-10
    -1,301 (SE 662; lineage denial), ranks 11-30 +280.
  - (Sep 30 00:05) All four arms, margin vs the real top team against the same live sub (234 exact games):

    | arm | ranks 1-10 (SE) | ranks 11-30 | wins (real 45%) |
    |---|---|---|---|
    | d3crop_m68 | +83 (818) | +4,936 | 62% |
    | d3crop_m68_m1 | -3,711 (2,740), with the collapse | +5,216 | 65% |
    | hyb_v3m68_ms_m1 | +1,439 (715) | +5,801 | 72% |
    | CPP5c_v3_m1 | +455 (806) | +4,089 | 59% |

    Per team, DECEM is the one every arm falls short of against our subs: -2.6k to -8.8k (7 games). The bed's opponent is our own
    lineage (the capped follower), so it measures how well each arm beats us, not how it would do against the live field.
- (Sep 30 00:15) Farm vs seller (Imitation's design) on the swap bed. Each of the 234 exact games vs ranks 1-30 was replayed with the
  top team's recorded farm and our m1 seller on strawberry / egg / milk / wool, against our live sub.
  - Margin vs the real game: ranks 1-10 -2,434 (SE 348), ranks 11-30 -2,784 (SE 300). Our seller's own money is +0.45-0.49k, our sub
    earns +2.9-3.3k more, and the top team's wins drop from 73% to 47% (ranks 1-10).
  - Price lead over our sub per unit, days 12-24, real vs our seller: strawberry +7.1 / +6.0, milk -0.6 / -0.5, wool +11.3 / +7.1.
  - Our sub's milk sells at 106.8 against the real top team and 112.2 against our seller; wool 125.9 vs 131.8. Meanwhile the top teams
    sell their own milk at 106.2, where our seller gets 111.7.
  The field's selling trades its own price for the opponent's price, ~2.6k per game of denial against our lineage on the same stock.
  Our seller holds for its own price. So the seller matters, through denial rather than own income (Imitation's 48 M&M worlds read
  only -0.68k).
  - Per top-10 team, recsell minus real, margin per game: yuto -1.1k (17 games), akmr -1.4k (13), Mother-Goose -3.7k (11), DECEM
    -4.0k (7), Majkel -4.8k (7), DSM -1.4k (5), Vadim -2.7k (5). Every top-10 team's selling beats ours on its own farm.
  - Mostly the gap is denial: for DECEM our seller earns +1.6k for itself but our sub +5.6k. Majkel and Mother-Goose also get better
    prices for the same stock (our seller's own -3.9k / -2.2k).
  - DECEM's seller is ~4k of its 2.6-8.8k edge over our designs.
- (Sep 29 21:30) The live opponent mix, from our live games vs ranks 1-30 (163 opponent games, days 10-27; thin/opp_mix.py):
  - 94% are dawn sellers: a thin h0 sale on more than 15% of days, typically 40-50%;
  - they sell 22-36% of their thin units at h0-2 and 12-27% at h18-23;
  - only 1% never sell at dawn;
  - we sell at h0 on 5% of days, with 17% of thin units at dawn and 46% in the evening.
  So the capped evening follower on the wide bed and in the league stands for ~1-6% of the live top-30 field. The contested reads
  (m1 +0.93k on the clone bed, Imitation's contested league +1.58k) are the representative ones for live.
- (Sep 29 20:09) BC's mainshare 8 10 on the top-clone sets (the 3Q zoo clones, 3 seed sets, 240 games each, vs hyb_melon68):
  hyb_m68_ms -858 (SE 249; own +172, opponent +1,030), hyb_v3m68_ms -886 (SE 309; own -878, opponent +8). Both negative at ~3 SE,
  unlike Imitation's league (+1.26k for hyb_v3m68_ms). hyb_v3m68 (v3 alone) is next. Running as well: saleslots=3 on the contested
  bed, and BC's forecaster c30_cum03pw (hyb_m68_pw) on the full 760.

## Audit: funding in dc11 v95 (experiments/v10/sep25_compiler_overhaul/dc11/compiler.cpp), for dc12

Evidence below is from the day 1-9 herd replays above (cows asked vs bought in our live games; M&M's own intents on M&M's
dawns) and the live cash comparison. Severity = how much of the early-cow / early-cash gap it explains.

1. **Sales are not scheduled to pay for purchases** (funded() l.547-626; compile_level ladder l.905-1000). The funding check
   replays the plan with the live executor, whose seller picks its sales from its own hold values (evening-leaning). Purchases sit
   at fixed hours and must be covered by whatever cash that seller has raised by then; nothing asks the seller to sell earlier for
   a purchase. The only coupling is a crude financing bonus: deposits before the deferral hour get +0.5 x the price (l.863-864).
   Evidence: the day-3 cow is short at every tried hour (0-14) in 93% of our games while the day ends with cash spare (Day
   compiler: ~$250); M&M sells fertilizer from h0 and buys at h8. Severity: HIGH (the core of the early-cow gap).
2. **Fertilizer is not cash** in the dawn-cash estimates: hire_cap (l.802-808) and defer_purchases (l.829-831) count money +
   0.9 x sellable shed stock but skip WHEAT (except the wheatcash sale) and FERTILIZER. M&M's early cash is fertilizer. Evidence:
   fertilizer revenue days 2-6 $56-233 per day below the top four; hires 4 vs 5-6. Severity: HIGH on days 2-6.
3. **Hires capped by dawn cash** (hire_cap, used unless cashsell; l.906). The crew that would raise the day's cash is limited by
   the cash it would raise. Evidence: hires 4 vs 5-6 on days 2-5; cashsell (no cap) fixes the day-2 cow (-0.72 -> -0.25).
   Severity: HIGH without cashsell; removed with cashsell.
4. **Animals are always deferred first** (defer_purchases l.839-861): purchases are ranked wheat, fertilizer, seeds, land,
   animals; once one cannot be covered by dawn cash, it and every later-ranked purchase wait for the deferral hour. So any
   shortfall defers the cow, and the variants from hour 3 / 6 / 10 / 14 still fail (assumption 1). Severity: HIGH.
5. **The trims drop animals first from day 1** (trim_new_entity l.717-742; crop_first default 1; trimnet off): when no funding
   variant works, the most expensive new entities go, sheep, then cows, then geese, before any crop. There is no value model
   comparing a cow with the crops it competes with. Evidence: cropfirst flips the loss between cows (d3crop line -2.28 cows,
   -4.55 plantings) and crops (hyb line -0.57 cows, -8.28 plantings); M&M keeps both. Severity: HIGH (it decides what is lost).
6. **Every planned order must fill completely at its hour** (funded() l.603-623), except seeds under cashsell (l.618). Animals
   and hires are all-or-nothing, and a short order fails the whole plan, not just that purchase. Severity: MEDIUM.
7. **A deferred purchase moves only its buy hour** (delay_purchase l.812-822: +3 h per repair, 2 repairs per variant), not the
   route stops that place the animal; a unit that arrives before the animal fails, which fails the plan ("unit action fails at
   hour h", l.597-601). Evidence: the day-3 failures with cashsell are "cow short" and "unit action fails" together. Severity:
   MEDIUM.
8. **The router drops animal placements it cannot fit** (l.789-791: a dropped stop cancels the new animal). Evidence: about half
   of the day-3 cow losses in our games are "dropped 1" at fallback 0, not funding failures. Interacts with the router's
   per-turn cost (Day compiler's router audit). Severity: MEDIUM.
9. **The stress forecast** (market.cpp stress_forecast: all visible opponent stock sold at hour 2, stress_hour default 2) funds
   against a pessimistic morning. Evidence: stress=12 or 23 leaves the cow gap unchanged (-2.35 vs -2.40). Severity: LOW for
   animals (untested for seeds / land).
10. **The next-dawn feed reserve** (funded() l.627-646; dawn_reserve default on): end assets (money + 0.8 x stock) must cover
    (animals - shed wheat) x wheat price x 1.2 + $30 when money fell during the day. Each new animal raises it by ~1.2 wheat. BC's
    ablation measured only plantings (-2.0 when removed); Day compiler's reserve=0 fixes day-6 animal trims, not days 2-4.
    Severity: LOW-MEDIUM (untested on animals for days 2-4).
11. **An expected-only plan is a fallback at the same level only** (l.993-998): if no variant is stress-funded, the plan funded
    under the expected forecast is used; otherwise the ladder goes to NoLand and the trims (animals first). Severity: part of 5.

What dc12's funding needs, from this evidence: plan the day's sales together with its purchases (a cash-flow schedule: which
units sell at which hour to cover which purchase), count shed fertilizer and wheat as cash, size the crew for the cash it
raises, place animals when they are bought, and choose what to drop by value (cow vs crops) rather than by type. Test: the
q335 / q336 replays (cows_next and plantings together), then the 760 + wide bed.
- Sep 30 15:09 M&M 56701470 profile (mm/sub_profile.py, ledger 90 games vs 56679033 147 / 56680759 170; M&M seat, d10-27): no style change. Selling-hour shares and lots within 0.01-0.02 (wool lot 2.33, milk 3.60, strawberry 3.68; eggs h18-23 0.60); d6 wheat 3.4 vs 2.4-2.5, strawberry 13.8 vs 15.0; d8 / d10 same; dawn-10 sheep 5.8 vs 5.3. vs top 30: 35 / 35 wins, +5.9k.
- Sep 30 15:09 judge_opp_setup.sh reordered (package G3 + package swap in parallel first, then copy, then rec); field setup relaunched in that order; pk_fc2 judged vs v5t (started 15:08, build_imf1) and vs field (auto-start when the package baseline has its first 3 rows). astra-NEW007 applies to the field opponent too: field_s0 trained on arrays_mm4 (148 G3 worlds, M&M's side). It plays the other seat and the effect is paired (baseline and candidate face the same opponent), so the bias should be small, but report it with each field read.
- Sep 30 15:09 Live pkm3cma (56706309) vs pkm1 (56690263), same window 11:46-13:58 UTC (live/same_window.py): pkm3cma 44 games (top 10: 1, 11-30: 8 at 8/8 +8.4k, 31-100: 9, 101+: 26); pkm1 13 games. Too few to compare; recheck at ~30 games each.
- Sep 30 15:13 pk_fc2 checks (G3 56, swap interim 80; swap/pair_decomp.py, mirror_split.py, train_split.py):
  - Mechanism on G3: price, not volume. Milk +1,095 (103.5 -> 108.4 per unit), wool +895 (116.0 -> 121.9). Both package seats move milk / wool toward h21-23 (arm 0.27 -> 0.30 / 0.23 -> 0.27, opponent 0.26 -> 0.33 / 0.24 -> 0.30); the opponent also gains (+526).
  - Swap interim: margin +289 (SE 547), own -564, opponent -854; milk price reverses (106.1 -> 103.1); the arm moves strawberry / milk away from h21-23.
  - Not a mirror artifact: tie worlds +1,481, non-tie +1,384.
  - Not selection of screen worlds: after the first 24, +1,240 (SE 565).
  - Training overlap: fc2's recent rows include 99 / 218 G3 and 109 / 234 swap worlds (shops replayed). Margin is the same in trained vs untrained worlds (G3 +1,393 vs +1,415; swap +926 vs +165, SE ~1,000). In trained G3 worlds both seats gain ~2-3k and the margin is unchanged. No leak signal in the margin.
- Sep 30 15:17 astra-150 for pk_fc2: G3 worlds outside fc2 train/val and outside the screen 24: n 18, +1,423 (SE 766); untrained n 35 +1,415 (SE 672). Reserved G3 list ready (swap/list_g3res_mix.txt, 119 worlds, 0 in fc2 targets), held for the surviving finalist. pkm1_t30 swap slice (47 live top-30 worlds of the current package) running to separate world vs opponent effects.
- Sep 30 15:19 Baseline context vs the v5t opponent (G3 218-mix first 96, 84 done): package vs M&M's recording +371 (SE 756), own -295, opp -667; W/T/L 68/0/16 vs 66/0/18 (score 81.0% vs 78.6%). Same worlds vs the package opponent: -755 (SE 792), score 50.5% vs 66.7%. Both beat v5t ~80% (weak opponent); M&M's recording is open-loop, so it loses more when the opponent changes (exact-bed bias). Context only, not a gap estimate.
- Sep 30 15:21 astra 14:15 triage (Weaknesses): (a) v5t and field opponents share the v5t seller: one timing-sensitivity family, not two independent field checks; every read will say so. (b) Reserve 119: only M&M's own behaviour was profiled (sub_profile.py); no candidate payoff has been read on it, so it stays a payoff confirmation set. (c) 031 (teacher_day value marks) routed to Imitation / BC.
- Sep 30 15:22 BC caveat on fc2 (adopted): fc2 forecasts our own subs best (dawn Poisson -3.73 -> -5.50 vs M&M -3.87 -> -4.83); G3 / swap put our sub in the opponent seat, so weight v5t / field reads over them (v5t seller also M&M-trained, so not fully free of the effect). Counterpoint: swap (our older subs) own -564 despite best forecasts. BC trains fc3 (G3 / swap worlds out) and fc3n (also no recent_ours); I judge fc3n vs field / v5t once packaged.
- Sep 30 15:36 pk_fc2 vs field opponent (G3 218-mix, build imf1): PROMOTE at n=24 (+1,145, SE 512); n 28 +719 (SE 503), own +1,190, opp +471, score 85.7% vs 78.6%. Arm: wool +762 (price 115.3 -> 120.1), eggs +434 (+10 units; the field opponent sells 10.5 fewer, -484). The field opponent does not follow the arm's hours (unlike the package opponent on G3). Swap stage vs field running. Normal pk_fc2 swap PROMOTE at 96 (+1,058, SE 487; own -376, opp -1,434). fc3n / fc3 vs field armed (judge_logs/wait_fc3.sh; fc3n vs fc3 isolates recent_ours, astra 14:30).
- Sep 30 15:47 pk_fc2 vs field on swap: REJECT at n=24 (n 28: -1,056, SE 780; own -771, opp +285; score 82.1% vs 75.0%). Seller effect as on G3 (arm wool +467, eggs +251). The margin is dominated by field_s0's tomato layer, a yes/no plan switch (+-5-10k per world): opponent tomato units change by 40+ in 10 / 28 swap worlds (8 up) and 12 / 28 G3 worlds (7 up, 5 down). No-flip worlds: swap -674 (SE 1,032, n 18), G3 +680 (SE 528, n 16). Own money over four pk_fc2 reads: G3 package +1.9k, G3 field +1.2k, swap package -0.38k, swap field -0.77k, so it depends on the world (M&M vs our top-30), not the opponent. Field reads = sensitivity with decomposition + flip count, not n=24 gate verdicts (swap/item_worlds.py).
- Sep 30 15:50 pk_fc2 vs v5t (G3 218-mix, n 96): +84 (SE 325), own +240, opp +157, score 83.3% vs 80.2%; max reached, no verdict. Arm milk +352, wool +87, strawberry -179; v5t gains similar. pk_fc2 summary: G3 package +1,407 / field +719 (n 28) / v5t +84 (n 96); swap package +1,058 (own -376) / field -1,056 (n 28). The G3 gain shrinks as the opponent gets more field-like; no robust own gain on field-timed opponents; live A/B is the deciding read.
- Sep 30 15:50 astra 14:45 triage (Weaknesses): pkm1_t30 slice (44 exact) overlaps fc2 TRAIN in 12 episodes and validation in 1; its read will be split by fc2 membership (swap/train_split.py). 021 (livefc audit identity) is Day compiler's.
- Sep 30 15:52 [SUPERSEDED 16:10: the flip is the clone's d10 land ASK, not funding / a cash knife-edge] field_s0 tomato flip = its day 10-13 land buy (swap/flip_cause.py): flip-up worlds opponent spend d10-13 +4.2-5.8k, field plants d14 +18-23; flip-down -5.6-6.1k / -24; dawn-10 cash differs only by a few hundred dollars (knife-edge threshold). BC's G2: the clone's tomato asks are smoother than the teams' on the same states, so the switch is downstream (land funding), not the network. Our package seldom flips (1 / 56 G3, 3 / 98 swap worlds; 0 / 28 vs field): a clone property. Fix option: cash-floor / land-first key on the clone (new opponent name, new baselines).
- Sep 30 16:00 fc2 MEMORISATION LEAK on swap-type beds (our real sub in the opponent seat, shops replayed): pkm1_t30 slice (44 exact, current package live) pk_fc2 - package +572 (SE 871) overall, but trained worlds (12, recent_ours targets) +4,328 (SE 2,144; own +4.1k, ~ the perfect-forecast value) vs untrained (32) -837 (SE 774); diff +5,165 (SE 2,280). Swap 98: trained +1,542 vs untrained +573 (own -661); band 31-100 no difference (+1,857 / +1,669); G3 no difference. Clean pk_fc2 reads: G3 +1,415 (n 35), swap top-30 +573 (n 49, own -0.66k), pkm1 slice -837 (n 32), band 31-100 +1,669 (n 32). Rule: learned components on swap-type beds need those worlds out of TRAIN and checkpoint selection, else read untrained worlds only (swap/train_split.py).
- Sep 30 16:04 BC: fc3v / fc3nv exclusions cover every exact list (list_g3w*, list_exact* incl. list_exact_pkm1_t30 44/44); all bed runs use exact lists, so fc3v / fc3nv reads are clean of the memorisation leak. fc3nv has no recent_ours rows at all.
- Sep 30 16:07 field2 qualification (swap/field_qualify.py): baseline rows byte-identical to field (39 / 39), landfirst only acts in candidate-perturbed worlds. Clone vs donor field on the same worlds: land after d10 0.32 / 0.50 (swap / G3 worlds) vs top-30 0.26, M&M's opponents 0.45 (M&M 1.00, package 1.00); tomato layer 0.84 / 0.65 vs 0.40-0.42; eggs h21-23 0.69 vs 0.49. Same 28 swap worlds: package-opponent pk_fc2 +188 (SE 787) vs field -1,056 (SE 780). pkm1 leak within one window (created <= 07:30 UTC): trained +4,328 (n 12) vs untrained -1,002 (n 16), top-10 share 0.50 / 0.44: consistent with memorisation. pk_fc2ens (3-seed fc2) vs field queued.
- Sep 30 16:10 field2 dropped (BC intent logs, reports/field2_qual): the clone's tomato flip is its day-10 Q4 land ASK (a state-dependent yes / no), not funding; landfirst never fires (flip worlds ask 0 land on d9-14). Converged teams ask land on d10 in 29% of their own held-out dawns; the clone matches (0.29). So flips are part of the candidate's treatment effect on a field-like opponent; read field on all fixed worlds. field2 baselines + pk_fc2 vs field2 stopped (PLANNED marked). Q4-denial lever ranked low (Q4 worth ~1.5k to top teams).
- Sep 30 16:20 astra 15:15: (006) leak wording softened: consistent with memorisation, not proven, and equal G3 margins don't prove G3 is unaffected. (007) saved fc/d3/targets.csv: all G3 / swap / band lists are clean (excl_bed / val_drop only), but list_exact_pkm1_t30 has 12 train + 1 val_recent, so fc3v / fc3nv are read on G3 / swap-mix only. Check membership against saved targets, not builder globs.
- Sep 30 16:22 Forecaster variants vs field (G3 218-mix first 96, vs package): fc3nv +203 (SE 389), own +1,285 (SE 339), opp +1,082 (SE 332), 26 / 96 flip worlds (19 down: the clone skips Q4, spends 1.5k less, ends richer); fc3v -207 (SE 427, n 84), own +687; fc3nv - fc3v +513 (SE 409, n 84); fc2 +719 (n 28). All raise own money +0.7 to +1.3k and the field opponent gains about as much; margin ~0. pk_fc3vens (Imitation: leak-free lower bound for fc2ens) launched on the normal judge + vs field.
- Sep 30 16:23 Final vs field (G3 first 96): fc3v +14 (SE 430), own +680, opp +667, score 78.1% vs 83.3%; fc3nv +203 (SE 389), own +1,285, opp +1,082, score 87.5% vs 83.3%; fc3nv - fc3v +190 (SE 395). No verdicts; dropping our subs from training costs nothing.
- Sep 30 16:29 pk_fc2ens vs field (G3 first 96): +290 (SE 541), own +987 (SE 372), opp +697, score 85.4% vs 83.3%; no verdict. Forecaster variants vs field: fc2 +719 (n 28), fc2ens +290, fc3v +14, fc3nv +203; own +0.7-1.3k each (reliably positive), opponent gains nearly as much; margin ~0, variants not separable.
- Sep 30 16:30 Live pkm3cma (56706309, ~65 games): top 10 0/5 (-5.0k: akmr, yuto x3, Boey), 11-30 11/12 (+5.8k), 31-100 11/16; rating 2704 (15:10) vs pkm1 2808; pkm1 had only 16 games in the same window, so no same-window A/B yet. Recheck at ~10 top-10 games.
- Sep 30 16:36 astra 15:30 triage (Weaknesses): (006) pk_fc3vens is a clean-recipe control, not a lower bound for fc2ens (seller / land responses do not preserve a loss ordering); my JUDGE_EXPECT wording 'lower bound' is withdrawn. (030) all forecaster judges here ran the pkq_fc2 stack on build_imf1 (src_dc12p, no hirecheck / latehire; sidecar SHA e098ddc8), not the final bundle (hirecheck + latehire, SHA 2f587872); reads are labelled with that stack. (other007) fc2ens - fc2 vs field: fixed 24 -400 (SE 690), 28 +376 (SE 767): inconclusive.
- Sep 30 16:37 [stack: pkq_fc2 base, build imf1, no hirecheck / latehire] pk_fc3vens (clean-recipe control): normal G3 218 PROMOTE at 96, n 120 +784 (SE 347), own +1,176, opp +392, score 63.3% vs 52.1% (all worlds out of d3 TRAIN / selection); swap running. vs field (first 96): +800 (SE 505), own +1,002, opp +203; vs fc3v +786 (SE 450), vs fc2ens +510 (SE 540) on the same worlds. cp_fc2 swap (REJECT at 24): fc2-untrained 18 -2,671 (SE 1,417) vs trained 10 +652; diff +3,322 (SE 2,012), leak direction again; origin-weighted +77 (SE 438).
- Sep 30 16:37 Confirmation set for fc2-family forecasters (Imitation's ask): 45 pkm1 live games created 07:33-14:30 UTC, 0 in any saved fc d1 / d2 / d3 non-test split (live/confirm/build_confirm.py). Part A (25) given to the Day compiler for forecast_trace MAE (confirmation use, logged); part B (20) kept untouched for a final read. Later pkm1 games are further untouched reserve.
- Sep 30 16:50 USER PRIORITY 1, divergence on the swap bed (swap/divergence_swap.py; CMA-free 206 worlds; arm = ours in the top team's seat vs its recording): shared with M&M: C1 copy watering alternation (odd -2.9 / even +3.9 per day), C2 care (copy -8%, package -5%, larger vs top 10), P2 late land-day wheat (d8 -3 to -7, d9-11 +5 to +7), P5 no late geese release, P1 / P4 (package only: d6 strawberries -1.8, early cows -0.6). M&M-specific: sheep early release (copy = teams), geese wave (package = teams; copy +1.25). Largest: Q4, we buy land after d10 in 99-100% of worlds vs top-30 recordings 21-29% (top 10 13-19%; yuto / akmr / Mother-Goose ~0, DECEM 0.57, Victor 0.42); plants d12 71-75 vs 57-60; carrots d11-17 +60%. Most top-10 opponents play 3Q.
- Sep 30 16:51 USER RULE (via Improve Agent): no CMA agent in any role. Swap bed: the CMA sub was the live opponent in 28 / 234 top-30 worlds, 9 / 103 r31_100, 40 / 144 r101p; judge.sh now uses list_exact_mix_nocma (206) and the _nocma band lists; judge_wsum drops pkcma worlds; past swap reads re-read on the 206 by filtering rows. Bed note: the swap bed did not protect against lineage-tuned decode knobs (m3cma +0.98k on swap, lost live 2704 vs 2808; only G3 218 rejected it). pk_fc3vens swap re-read (101 CMA-free, all out of d3 TRAIN): +1,977 (SE 497), own +329, opp -1,648, score 85.1% vs 67.3% (denial); bands on _nocma lists running.
- Sep 30 16:51 CMA-free swap re-reads (206 list): pk_fc3vens (101) +1,977 (SE 497), own +329, opp -1,648; pk_fc2 (87) +925 (SE 490), own -594, opp -1,519 (untrained 42 +787, own -869); cp_fc2 (24) -1,365 (SE 1,215), own +810, opp +2,175 (untrained 15 -2,776). Swap gains are denial.
- Sep 30 16:53 CORRECTIONS (astra 15:45): (007) the pkm1 confirmation sets are training-excluded, not unused: A 19 / 25 and B 10 / 20 were already scored in swap-slice / band runs (pkm1_t30, r31_100, r101p). Untouched reserve = B_unscored (10, live/confirm/pkm1_after0730_B_unscored.txt) + pkm1 games after 14:30 UTC, kept out of all bed lists (live/confirm/README.md). (032) duel_mm's .ops counter drops h23 actions (and still misses FERTILIZE / PLANT at h23): my divergence items from .ops (C1 watering alternation, C2 care, P1 / P2 / C4 / C5 planting days) are UNVERIFIED until the counter is fixed; Q4 / plants (dawn .herd) and herd placements / releases (dawn counts) stand.
- Sep 30 16:56 Confirmation A used once (Day compiler): forecast MAE learned heads -0.030 (SE 0.006) vs big2, fc2 = fc2ens = fc3vens; pinned fc2ens margin median +1.9k, 17 / 25 positive, one pinned collapse (115782305). B / B_unscored untouched (live/confirm/README.md).
- Sep 30 16:57 fcconfpin outlier 115782305 = the pinned (open-loop) recorded opponent collapsing (rival 131.1k -> 73.8k with fc2ens; our own 135.4k -> 185.8k), a pinned-replay artifact, not an fc2ens collapse.
- Sep 30 16:58 [stack: pkq_fc2 base, imf1, no hirecheck / latehire] pk_fc3vens CMA-free origin-weighted swap: +1,201 (SE 274), score +11.3 pts (SE 3.7); bands 1-10 +4,108 (own +1,348, opp -2,761), 11-30 +1,034, 31-100 +1,068 (own +1,018), 101+ +593. Leak-free. Divergence .ops re-read with build_imho (h23 counts; identity 3/3 copy / package) running on 48 CMA-free swap worlds.
- Sep 30 17:05 Divergence .ops re-read with build_imho (h23 counts; 48 CMA-free swap worlds, identity 3/3): copy odd-day watering -0.20 / day (old counter -0.82: mostly artifact), even-day +4.64 / day (+11%, real), care -0.70 / day (-5%, SE 0.18; old -1.16); package no watering / care gap (care -0.23, SE 0.20). Planting counts unchanged by the fix, so late land-day wheat (d8 -4 to -7, d9-11 +7 to +10), tomatoes, carrots (+8 to +9 vs ~9), d6 strawberries (package -1.7) are verified. The old counter also missed the teams' h23 watering.
- Sep 30 17:07 astra 16:00 triage: (032) imho .ops reads are labelled 'count correctness pending accepted-step check' (game identity is not count correctness; imho differs from m13x); planting counts identical across counters. (006) fc3vens CMA-free weighted +1,201 is local evidence on our-sub opponents, not a top-30 league or live transfer.
- Sep 30 17:08 Live Q4 context (swap/q4_live.py; recorded CMA-free swap worlds, real outcome our sub - top team, by the top team's land-after-d10 choice): top 10 3Q n 51 -347 (SE 849, win 31%) vs 4Q n 12 -4,435 (SE 1,359, win 17%); 11-30 no difference (+3.8k / +4.3k); 101+ 4Q teams lose big (+40.8k). Within top-10 teams with both: DECEM -5.8k / -7.5k, DSM -4.4k / -6.3k, Majkel +0.6k / -4.3k, Mother-Goose +2.0k / +0.1k. Association, team-confounded: 4Q = strong-state play of strong teams; does not say our always-4Q is wrong.
- Sep 30 17:18 Opening on the swap bed (swap/opening_swap.py; CMA-free; ours - recorded top-30 team, d0-9): hires copy 52 / package 49 vs 66 (fewer every day), spend 16.0-16.1k vs 17.1-17.3k (h3-11 -1.1 to -1.7k), wheat sold 16 vs 24-31 (d0 -5 to -6 units), dawn cash ours +100 to +400 on d4-9 (idle capital); plantings equal. The top-30 field shares M&M's opening (more workers, morning purchases, d0 wheat sale).
- Sep 30 17:19 Opening by band (copy vs recordings, d0-9): hires ours 52-53 vs 61-66 in EVERY band (101+ too); wheat sold 16 vs top 10 53 / 11-30 22 / 31-100 72 / 101+ 72; spend 16k vs 16.9-18.8k. The opening traits (more hires, early wheat selling, extra spend) are universal, not markers of strength (weak teams share them); extra spend / wheat = churn (cf. Sep 26 top-30 spend gap). Crew size needs a within-agent test.
- Sep 30 17:21 astra 16:15: the Q4 proxy (plants d14 - d10 >= 10) agrees with recorded land purchases on 151 checkable top-30 worlds (40 Q4, 0 mismatches), so the Q4 numbers stand; the Q4-outcome link stays observational.
- Sep 30 17:49 swap:cp_fc2x (copy + fc2 vs the live PACKAGE, 93 CMA-free fc2-untrained swap worlds, m13f): +325 (SE 514), own -276, opp -601, score 73.1% vs 68.8%, no verdict at 93; origin top 10 +1,459 (SE 929, denial: opp -2,100), 11-30 -188 (SE 611). Copy sells more eggs / wheat / tomatoes, less melon / strawberry / wool; spend +1.55k. On 47 shared worlds copy+fc2 - package+fc3vens -2,011 (SE 712). The copy is about level with the package in our top-30 worlds; the package+forecaster is ahead.
- Sep 30 17:58 M&M pull 17:48: 781 games (+12), no new sub; held out 240 (159 test / val), reserve 131 (56701470: 98); M&M-vs-ours exactness 5 / 5; 56701470 style unchanged (42 / 42 vs top 30, +5.5k). Next pull ~20:48 (waiter armed).
- Sep 30 17:59 BC field-clone corpus had 1 post-07:30 perspective (set-A 115748906); no field-opponent read used that world; builder fixed (make_corpus.py excludes all post-07:30 episodes).
- Sep 30 18:31 swap:cpv_vs_pkv (same clean forecaster fc3vens; 93 CMA-free fc2-clean worlds): copy - package REJECT at n=24; n 34 -2,410 (SE 1,079), own -2,343, opp +67 (top 10 -132, 11-30 -4,004). Copy's plan mix costs own money (melon -1.86k, strawberry -1.22k, wool -0.80k vs wheat +1.34k, eggs +1.11k, tomatoes +0.80k; spend +1.82k); the opponent takes wool (+1.99k). package+fc3vens vs package: 93 clean +1,025 (SE 519); pooled 147 CMA-free rows +1,445 (SE 428), own +300, opp -1,146, score 83.0% vs 68.7%. The package + clean forecaster is the strongest local line; the copy does not beat it in our top-30 worlds.
- Sep 30 18:39 PRE-LIVE CHECK of the exact bundle packages/m3fc3vens_hc (package + fc3vens + hirecheck / latehire; build_m14bf, package identity 3 / 3; no CMA): G3 first-96 PROMOTE at 48 (n 58 +1,111 SE 460, own +1,247, score 67.2% vs 48.3%; vs M&M's recording -262 SE 1,072); swap 93 clean PROMOTE at 24 (n 39 +1,947 SE 688, own +440, opp -1,507; origin top 10 +2,865); vs the clean-recipe control pk_fc3vens: G3 +397 (SE 195), swap +67 (SE 200). Submission is the user's decision.
- Sep 30 19:04 Exact bundles, same build (m14bf) and worlds: m3fc3vens_hc - m3fc2hc (= Kaggle 56714867): G3 first 96 -76 (SE 462), swap 93 clean (fc2-untrained) +178 (SE 423), no difference. Both beat the package: G3 +1,210 (fc2hc) / +1,134 (fc3vens_hc), swap +1,180 / +1,357. 56714867 is as strong locally as the clean-forecaster bundle.
- Sep 30 19:27 Bed bias (swap/sale_hours.py, days 3-9): wool mean sale hour real top-30 teams 4.5 (h3-5 0.65), M&M 4.5, M&M's real opponents 4.9; our lineage 7.0-7.4 (h6-8 0.56-0.63) as the swap opponent (our live sub 7.3), the G3 opponent (package 7.0) and our arms. Both beds face a late-wool lineage opponent, which likely overstates early-wool gains; [CORRECTED 19:27: the field / v5t opponents are late too (wool days 3-9 mean hour 7.5-7.9, h3-5 0.25-0.27): their farm runs our compiler]. Milk: teams split h0-2 / h6-8 (~0.47 each), package h3-5 0.70.
- Sep 30 19:27 Correction (Imitation): field / v5t clone opponents sell wool late on days 3-9 (mean hour 7.5 / 7.9, h3-5 0.27 / 0.25; my team-like claim came from d10-27 bands). The only local bed with real opponent selling hours is Imitation's opponent-replay bed (runs/opprec, build_m14or DUEL_OPP_REC: M&M's real opponent's recorded actions with loans; identity 3 / 3); na5 vs package there +0.48k (SE 0.49k), own -0.82k, opp -1.30k.
- Sep 30 20:13 T-RESPONSE (live, Sep 28 - 30 18:00 UTC; the same 20 top-30 teams: 470 games vs M&M, 227 vs our subs; team-balanced; live/t_response.py, t_response_px.py): T ends +4,588 (SE 1,680) richer vs us than vs M&M; our money -1,592 (SE 1,761) vs M&M's. Divergence from day 11 (d15-21 +3.4k). T's plan barely differs; T's extra income = wheat +2.9k (M&M sells 476 wheat d10-28 vs our 363; T sells +68 units at +$1.4 vs us: volume denial by M&M) and wool +2.1k / milk +0.65k (T's price +$10 / +$6 vs us: M&M sells at T's morning hours, we sell late: timing denial). Fixed-opponent beds cannot show this (opponent cannot react; lineage opponents sell late).
- Sep 30 20:14 Wheat channel source (live/x_wheat.py, same games): M&M plants more wheat from day 10 (d10-14 +13.2, d15-28 +17.7) and sells +33 / +80 units; animals and feeding equal; plus the d0-5 wheat sale (20.2 vs 10.7). Our cleared plots go to carrots / tomatoes instead.
- Sep 30 20:21 splitfert=10 + nearanimals=6 move only the opening (days 3-9 wool h3-5 0.29 -> 0.71-0.77, mean hour 7.1 -> 5.2; M&M 0.65 / 4.5); days 10-28 unchanged (wool mean hour 12.4 both vs M&M 9.5, milk 13.5 vs 9.9), where the T-response gap sits. Pre-live check of pavel-bc-opus-v17d-dc12m19-fc2-m68 (live + the keys; build_m14sj = src_m14 + m19, source identical) vs the live bundle launched (lo).
- Sep 30 20:33 PRE-LIVE pavel-bc-opus-v17d-dc12m19-fc2-m68 (live 56714867 + splitfert=10 nearanimals=6; build_m14sj, identity 6 / 6) vs the live bundle: G3 first 96 +885 (SE 506), own -610, opp -1,495, score 64.6% vs 70.8%; swap 93 clean +390 (SE 493), own -988, opp -1,378 (top 10 -179, 11-30 +647); no verdicts. Days 3-9 wool shifts to h3-5 (0.27 -> 0.71) on the swap bed too; days 10-28 unchanged. Positive margin only by denial of late-wool lineage opponents (bed bias); Imitation's real-opponent read +0.47k (SE 0.30k).
- Sep 30 20:34 DEADLINE (user, ~22:30 BST, via BC): m19fc3vens (= m3fc3vens_hc + splitfert=10 nearanimals=6; byte-identical to the Day compiler's models/bun_na6) vs m3fc3vens_hc: G3 clean-99 first 96 (Day compiler rows, m14sj vs m14si) +961 (SE 502), own +512, opp -449, score 74.0% vs 70.8%; swap 93 clean running (hi; queued behind dc:evecut). Context: m3fc3vens_hc = m3fc2hc (live 56714867) locally; dc12m19 (live + keys) vs live G3 +885 / swap +390 with own -610 / -988.
- Sep 30 20:40 swap:m19fc3vens paused at 20:40 (Imitation: slots to the hybrid-opponent bed) at n 45: vs m3fc3vens_hc +497 (SE 558), own -810, opp -1,307, score 80.0% vs 75.6% (top 10 +1,188, 11-30 +151); vs live 56714867 +1,178 (SE 649, n 46). Resume at 21:30 (judge_logs/m19fc3vens_resume.sh).
- Sep 30 20:44 swap:m19fc3vens stopped at n 45 (+497, SE 558): Imitation's hybrid-opponent bed (real team's recorded farm + learned M&M seller, real selling hours) gives the keys (PR vs live) -0.04k (SE 0.48k), own -1.83k, wins 37 vs 39 (n 48). The splitfert / nearanimals gains on G3 / swap are consistent with the late-wool opponent bias.
- Sep 30 21:07 M&M pull 20:48: 858 games, NEW sub 56711476 (73): hours / lots as 56701470; d10 tomato 5.2 vs 3.9, carrot 0.5 vs 1.2; dawn-10 cows 8.4 vs 7.7; vs top 30 24 / 24 (+6.8k). Reserve 208, held out 317.
- Sep 30 21:29 LB judge (exact Local-LB d6157df, identity 6 / 6): stage-1 reads vs #1 (n 30, m19base mirror 50.0%): f1_fc2ens PASS 53.3%, paired +398 (SE 516); f3_fclin PASS 56.7%, paired -202 (SE 554). Two-stage queue per Imitation (lb/screen3.sh, gate lb/gatecheck.py).
- Sep 30 21:39 EXACT LB (d6157df, challenger blocks): f1_fc2ens (#1 + fc2 3-seed ensemble) 90 games 69-0-21 (76.7%) vs m19base (#1 itself) 51-14-25 (64.4%); vs #1 16-0-14 (base 8-14-8 mirror), m14-fc2 27-0-3 (24-0-6), mmpq 26-0-4 (19-0-11); paired +922 (SE 390), wins +26 / -8. f3_fclin (60) paired -446. Full run for f1 (6 non-RL blocks) + projection running.
- Sep 30 21:43 LB stats fix: the two seats of a seed are correlated (often identical on seat-symmetric seeds), so SEs are now seed-clustered (lbsum.py, gatecheck.py). f1_fc2ens +922 (SE 490, 1.9 SE; per-game SE 390), wins +26 / -8. c2_rec110 FAIL stage 1 vs #1: 40.0% vs 50.0%, -961 (SE 1,123, 15 seeds); stopped (Imitation; the Day compiler's C++ replay agrees: not an LB gain).
- Sep 30 21:46 EXACT LB stage 1 vs #1 (n 30, m19base 50.0%): c4_rec120 FAIL 10.0% (-2,812, SE 1,028); c2_rec110 FAIL 40.0%; stk_c2f3s4 PASS on margin (46.7%, -25). Recency conditioning loses to #1 on the lineage roster (the realistic-bed gain does not carry). Queue now forecaster arms first (f4_fclinfresh, f3_fclin), then c2f1s4, dc_ss4, f2 (vs #1); c2f2 dropped.
- Sep 30 21:52 f1_fc2ens projection (partial, 137 exact games: vs #1 / m14-fc2 / mmpq / m3fv 30 each, m3 17; 4 weakest + RL missing): LB Bradley-Terry puts it #1 at 1745.1 (107-30) vs m19-fc2 1624.7. c5_rec110all FAIL stage 1 (36.7%, -942). f4_fclinfresh running straight to 90 (user deadline ~00:20).
- Sep 30 21:54 USER RULE (via Imitation): avoid lineage overfit; nothing tuned to our lineage / the LB roster. Stopped f4_fclinfresh (30 games), f3_fclin (60), f5_mix (not started). f1 LB PR merged (#248). LB queue: dc_ss4, f2_fc3vens (two-stage); next f1 + ss4 and the Day compiler's late-crop fix.
- Sep 30 22:00 LB: saleslots=4 closed (dc_ss4 vs #1 n 29 31.0% vs 51.7%, -660 SE 442; f1+ss4 vs f1 -1,084 SE 1,016 n 22; Imitation's hybrid bed f1s4 level / -217 vs f1). f1 hybrid final +610 (SE 406), own +1,127: the merged f1 transfers. f6_fc3nvens (cleanest forecaster: no recent_ours, no bed worlds) running alone, then f2_fc3vens.
- Sep 30 22:01 [CORRECTED: the 4 m14 replays hit the complementary seats, which the C++ harness also matched; only mmpq 971000 s1 is a confirmed harness-side miss] LB identity: Python judge reproduces mmpq 971000 s1 (a C++ miss) and m14-fc2 968002 s0 / 968003 s1 / 968004 s0 / 968014 s1 (C++ matched these too). Package identity checks use the Python judge (lb/lbgame.py).
- Sep 30 22:04 LB identity (final): the Python judge reproduces all 5 official games the Day compiler's C++ replay missed, in the right seats (m14-fc2 as challenger 968002 s1, 968003 s0, 968004 s1, 968014 s0; mmpq 971000 s1), plus the 4 complementary seats: 9 / 9 exact. The misses are C++-harness-side; LB runs are reproducible.
- Sep 30 22:04 LB: f6_fc3nvens (cleanest forecaster) FAIL stage 1 vs #1: 40.0% vs 50.0%, -1,153 (SE 713); vs f1 -1,551 (SE 908). f2_fc3vens next.
- Sep 30 22:10 LB SCREEN SUMMARY (exact d6157df, vs m19base): f1_fc2ens +922 (SE 490), 69-0-21, projection #1 (merged #248; hybrid +610); f2_fc3vens +42 (SE 418), -880 vs f1; stk_c2f3s4 PASS at 30 (-25), stopped at 40; f3 -446 (60, stopped, lineage rule); FAILs vs #1: c2 -961, c4 -2,812, c5 -942, f6 -1,153, dc_ss4 -660; f1+ss4 -1,084 vs f1 (22). Nothing beyond f1 meets the user's push criteria.
- Sep 30 22:14 OVERFIT AUDIT (live evidence for LB #1's parts; live/band_table.py): no same-window live A/B exists for 56714867 vs m3 (m3 retired at 17:44 UTC; 56714867 has 5 top-30 games, rating 2737 still rising) or for d3crop / m68 (partners were the RL / CMA subs, excluded), timing=0.5 (never A/B'd: 56613566 / 56614000 both timing 0.5; before / after vs ib2t only), or the m19 keys (never live). m3 top 10 12 / 22 (best of the era, cross-window). Rating plateau 2800-2850 for all dc11-era subs. #1's parts rest on local beds, not live A/Bs.
- Sep 30 22:41 AUDIT honest1 (#1 - nearanimals + clean fc3nvens) on the exact LB: 150 games 88-0-62; vs m19base (#1 clone) -1,740 (SE 673, 90), vs f1 -1,927 (SE 636, 143); like-for-like projections: 90 games f1 1748 / #1 clone 1650 / honest1 1531 (#4); 143 games f1 1753 / honest1 1585 (#2). The honest version costs ~170-220 LB points vs f1 (~120 vs #1); Imitation's hybrid bed has honest1 +183 vs f1, so the LB cost is mostly lineage fit (lb/proj_common.sh).
- Sep 30 22:51 AUDIT m3_fc3nv (m3 + clean fc3nvens) vs an m3 control on the exact LB (150 identical games, m3 bridge): +82 (SE 646), wins +37 / -27; vs #1 14-0-16 vs 7-0-23; projection 1553 (#3) vs m3 1523 (#4). Honest options vs f1 1748-1753 / #1 clone 1650: m3_fc3nv 1553, honest1 1531-1585, m3 1523.
- Sep 30 22:55 OFFICIAL LB (origin/main 8918e2b): f1 = pavel-bc-opus-v17d-dc12m19-fc2ens-m68 #1 1651.2 (194-76, 270 games); old #1 1617.8. The exact LB judge's pre-push replays match f1's official games on shared seeds 83 / 83 to the dollar. Honest entry (m3 + fc3nv) pushed 22:50, not merged yet (lb/out/lbwatch.txt, polled every 10 min).
- Sep 30 23:15 OFFICIAL LB (d82c092): honest entry pavel-bc-opus-v17d-dc12m3-fc3nv-m68 #3 1523.7 (145-125): vs f1 7-23, m19-fc2 12-18, m14-fc2 11-19, others 18-22 wins of 30. f1 #1 1647.3, m19-fc2 1596.0. Rank matches the exact-judge projection (#3).
- Sep 30 23:30 PICK 2 (USER; base m3 = pick 1): exact LB honest1 - m3_fc3nv on 150 identical games +1,314 (SE 715), wins +38 / -29; projections honest1 1592.5 (#2) vs m3_fc3nv 1552.9 (#3). Swap bed (206 CMA-free real top-30 worlds) running for m3 / m3_fc3nv / honest1.
- Sep 30 23:55 PICK 2 FINAL (swap bed, 206 CMA-free real top-30 worlds, ~126 done): honest1 - m3 +1,904 (SE 429; top 10 +4,227, 37 / 40 vs 22 / 40); m3_fc3nv - m3 +1,250 (SE 411; top 10 +2,980); honest1 - m3_fc3nv +613 (SE 392; top 10 +1,076, 11-30 +400). With the exact LB (+1,314, SE 715), both beds favour honest1 at ~1.6-1.8 SE. Sent to Imitation 23:56.
