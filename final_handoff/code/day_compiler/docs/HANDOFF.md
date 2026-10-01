# sep29_dc12 handoff (Sep 29 evening; updated as reads arrive)

For: the user and the other sessions (Imitation, Weaknesses, BC). Details and numbers: PROGRESS.md (timeline), DESIGN.md
("Revision after the isolation ladder"), README.md ("Gate ladder"), team stop list in work/mm_handoff/findings/day_compiler.md.

## Status (Sep 30 11:45)

LIVE: Kaggle 56690263 (d3crop_m68 + m1 keys + nighttrim + survivalfloor), 2,802.7 at 11:22. Nothing submitted by me.

L1 (copy trajectory) hand-over done: m13 patch on Imitation's src_dc12i (m13_sep30_landkeys_rivalfrac_on_src_dc12i.patch), frozen build
work/sep29_fund/build_m13f, copy keys `reserveland=1 survivalfloor=1 shadowskip=2`. Cause: on the copy's land days the next-dawn reserve
(~$500 for tomorrow's feed) and late pocket cash cut the evening strawberry seeds (M&M ends day 6 at ~$73). Own states (60 worlds),
crops d6 / d8 / d10: 16.62 / 12.48 / 15.87 -> 17.52 / 12.63 / 15.50 (M&M 18.63 / 14.72 / 19.2). Imitation runs it with sbc1 on judge.sh.

L2 (seller): rivalfrac=1 fixes a rounding bug in the margin term (G1s +0.8k), but full games reject it (league -0.85k, G3 -0.46k for
the equivalent rivalnight 0.5): seller -> cash -> farm feedback (it doubles the copy's day-8 seed cuts). Off; astra-014 trace next.

Earlier candidate (package + feedvalue=1): league +747, swap +430, G3-wide +26, 760 -241; not promoted.
Retracted: slot / cash-cut probe counts before build_dc12v14 (funding-simulation executor runs mixed in).

## Recommendation (written before the submission)

1. Live candidate: the lineage line (d3crop_m68) + m1 keys `dropany=6 cashsell=1 wheatcash=1 reserve=0 saleslots=3`, packaged by
   BC from their tree + `m1_saleslots3.patch`. Positive on every bed with dawn-selling or reacting opponents (pinned +1.5k,
   760 +1.1k, contested +0.9k / +1.6k, 48 M&M worlds +1.3k, recorded-M&M +0.7k); negative only on the wide bed vs our own capped
   evening-selling sub (-1.24k, SE 0.86k), an opponent type that is 1-6% of the live top 30.
   Caveat (Imitation, all-8 league): on the hybrid line (sidecar startcomplete=6 cropfirst=6) m1's funding keys let days 0-5 spend all
   cash, so the land slips (Q2 d9, Q3 d11, no Q4) and a few seeds lose ~16k; that line had land < 4 in 3% of games, the d3crop_m68
   line in 0 of 168. m1 is validated for the d3crop_m68 line only; a hybrid + m1 package needs a land-cash reserve (open item).
2. Hedge: m1b = the same without cashsell (level on the wide bed; gives up ~0.7k on the 760 / contested beds).
3. Recommended add-on (m2 = m1 + nighttrim=1, m2_nighttrim.patch): stops ~10 units / game destroyed at the night deposit.
   Weaknesses' 760 -19 (SE 60) vs m1 (level; +1,078 vs d3crop_m68), contested +268 (SE 129), pinned +9 (SE 67); same latency.

4. REQUIRED safety fix before any package: survivalfloor=1 (an all-unfunded day runs the survival plan instead of idle). Without it
   m1 collapsed to $10 in 1 of 234 real-world games (-186k: a $10 dawn, two idle days, all animals escaped); with it that game ends
   -7.9k. Side effects: pinned 200 games 0 changed; Weaknesses' swap bed (234 exact games) only that game changed. 760 read at
   Weaknesses. Package = m1 + survivalfloor (+ nighttrim).
   Clean patch on top of the m1 and m2 patches: `m3_survivalfloor.patch` (39 lines, dc11/compiler.cpp / compiler.hpp; key
   `survivalfloor=1`). bc_v95 + m1 + m2 + m3 patches (work/sep29_fund/bc_m3min, build_m3min) reproduce build_dc12d's
   m1 + nighttrim + survivalfloor games exactly (6 pinned games, byte-identical rows).

## What was done

The day compiler was isolated from the network and the opponent before any full-game judging (the user's method):
1. One day from M&M's recorded dawn, with M&M's own intent, against the recorded opponent (teacher_day; ~1 minute per 240 days).
2. 5 days from M&M's dawn (later days use our network's intents).
3. Reacting opponents (clone beds, league, contested beds), then pinned real games and the wide bed.

Findings on rung 1 (M&M days 12-17): the dc11 core (bind, router, realize, executor) reproduces M&M's production on its own states
(the same plantings per crop, harvests, field units 87.6 vs 87.5, field value within $5) and ends each day with a better margin
(+90 / day). The measured faults were in the couplings between components. Each was traced to its root cause and fixed as a key.

## Milestone m1

Keys (append to the dc11 options of any line): `dropany=6 cashsell=1 wheatcash=1 reserve=0 saleslots=3`.
Frozen tree `work/sep29_fund/bc_dc12_m1`, build `work/sep29_fund/build_dc12_m1` (full_games_dc11, duel_dc11, replay_games_dc11,
teacher_day). With the keys off the build reproduces the package lines' games exactly.

| Key | Root cause it fixes |
|---|---|
| dropany=6 | The hire search only dissolved the last hire's route; crews ran one hire larger than M&M's on 31% of days. |
| cashsell=1 | The seller ignored the plan's purchases: the cash they need is a constraint in the seller (Lagrangian bonus). |
| wheatcash=1 | The dawn shed wheat was kept as feed reserve; M&M sells it at dawn and buys feed later. |
| reserve=0 | A next-dawn feed reserve in the funding check trimmed funded plans (day-6 cows). |
| saleslots=3 | The engine takes 10 orders a turn, one per hire: the router filled the first hour with hires and the seller's dawn lots were dropped. Dawn sales now take a first-hour slot from a hire only when routing again keeps the crew and drops no required stop. |

Reads (arm = line + m1 vs the same line, paired):

| Bed | Opponents | Result |
|---|---|---|
| Pinned real games (200, live29 + live28) | real recorded play, non-reacting | trimmed +1,495 [+866, +2,081], p < 1e-4, 70% up |
| Weaknesses' 760 (d3crop_m68) | reacting clones | +1,097 (SE 174), all 8 seed sets positive |
| Contested clones (clones also on m1) | reacting dawn sellers | +930 (SE 418) |
| Imitation, contested league | the 3 strongest with dawn slots | d3crop +1.58k (SE 0.71k), v3 copy +0.57k (SE 0.61k) |
| Imitation, league | the 3 strongest (capped lineage) | d3crop_m68 +1.50k (SE 0.53k, 72 games; margin +1.02k vs -0.48k); v3 copy +0.61k (SE 0.43k); d3crop (older) -0.31k (SE 0.57k) |
| Imitation, 48 M&M worlds / recorded M&M | M&M's play | +1.33k (SE 0.59k) / +0.69k (SE 0.43k) |
| Wide bed (53 worlds) | our capped sub | -1,243 (SE 863) |

The wide-bed loss is a game effect (ledgers): our dawn strawberry sales take the morning from a capped evening-selling follower
(our own h0-5 price $126 vs $104), and the follower moves ~20 units into the drained evening (+$1.9k for it). Against dawn sellers
(contested beds, live top teams) m1 gains; against our own capped lineage it can give the evening away. Per-key wide runs
(Weaknesses) split it: m1 -1,243; m1 - wheatcash -594; m1 - cashsell +30; m1 - saleslots=3 -951; dropany alone +189 (SE ~0.8k
each). The channel is cashsell: it funds more of the network's animal asks (geese: eggs +18.7 units, spend +$450) and we produce
7.4 fewer strawberries, the product this opponent competes in, so its strawberry prices rise (+$1.5k).
Candidate m1b = `dropany=6 wheatcash=1 reserve=0 saleslots=3` (m1 without cashsell): level on the wide bed; pinned (200) trimmed
+1,085 [+502, +1,675], p 0.0002 (vs m1 -279, ns); on rung 1 it gives back cashsell's funding days (day 10: -2 plantings, land late again; day-2 cow -0.25).
760 / contested reads for m1b running (Weaknesses).
Which bed represents live (Weaknesses, our live games vs ranks 1-30, 163 opponent games, days 10-27): 94% of the opponents are
dawn sellers (a thin h0 sale on > 15% of days; typical team: h0 sales on 40-50% of days, 22-36% of thin units at h0-2); we are the
capped evening follower (h0 on 5% of days, 46% of thin units in the evening). So the contested reads (+0.9k to +1.6k) represent the
live field; the wide bed's opponent (our capped sub) is 1-6% of it. Recommendation: keep saleslots=3 in a live variant.

Kaggle time (harness, 20 games, overloaded machine): dawn step +16%, overage left mean 48 s (worst 21.9 s).
No submission was made; packaging (BC) and any submission are the user's decision.
Clean patch for any package tree: `m1_saleslots3.patch` (102 lines against v95 + BC's local additions, dc11/compiler.cpp and
compiler.hpp; the other four keys already exist in v95). v95 + patch reproduces build_dc12_m1 exactly (1,280 rung-1 day rows).

## Next candidate on top of m1: nighttrim=1

Full games vs our clones lose ~10-12 units per game at the night deposit (days 21-29, ~$400 / game): on end-game nights the plan
itself carries more than the shed holds (day 27: 143 units planned in pockets vs capacity 100), because output stops (harvest /
collect) are must-do work even when their product cannot be stored. nighttrim removes, after the router's overflow repairs, the
pure output stops whose cargo cannot fit (the product stays on the animal / crop for tomorrow). Clone games (80, paired with m1):
discards 11.9 -> 2.9 and 11.0 -> 1.6 per game, margin +133 (SE 258); rung 1 late days neutral; pinned (200) +9 (SE 67, no harm).
Contested bed (160 paired vs m1): +268 (SE 129). 760 (630 games): +47 (SE 64), own +183. Needs build_dc12d (the m1 build does not know the key).
Clean patch on top of the m1 patch: `m2_nighttrim.patch` (82 lines; router.cpp / router.hpp / compiler.cpp / compiler.hpp);
bc_v95 + m1 + m2 patches (work/sep29_fund/bc_m2min, build_m2min) reproduce build_dc12d's m1 + nighttrim games exactly.

## After the package (Sep 30, 00:00-)

- m1 vs M&M itself: in M&M's 72 recorded worlds against live d3crop, m1 in M&M's seat vs M&M's recorded play replayed in the same
  worlds (Imitation's mm_rec): margin +1,129 (SE 810), own +420 (SE 736). Caveat: the recorded play is open-loop.
- Land cascade (all three lines with m1; Imitation: the copy ends with 3 quadrants in 8 of 12 games on seeds 504 / 505 vs D / E):
  m1 funds the day 2-4 cows, the day 6-7 land ask comes with many plantings, the full ask is unfunded, and the ladder drops LAND
  first (NoLand); Q2 / Q3 slip and the network never asks Q4 after day 10 (-13k to -16.5k). Key landfirst=1 (the land alone
  before NoLand): hybrid seed 505 -16.5k -> -10.5k / -11.8k, land 4. The first version had a bug (the agent's larger-intent
  tries took the landfirst plan as complete; fixed: marked fallback NoNewEntities); fixed reads running (Imitation's league,
  pinned). Patch m4_landfirst.patch (49 lines on m3). Judged: inert on the live d3crop line (pinned 0 / 200, league 96 / 96
  identical); hybrid + m1 +0.36k (SE 0.18k, seed 505 fixed); copy +0.09k. Safe for the next package.
  A second cascade is the network's own: with m1 the day 2-5 animal asks are funded, dawn cash on day 6 is ~$30, and the
  network (money is an input) does not ask for land on day 6 (seed 504 copy vs E: logit -2.5 vs +13.4 without m1). No ladder
  change can fix a land that is not asked; on the d3crop line (startcomplete=1) it is rare.
- Base line on pinned 200 (field): hybp + m1 +6.1k vs the d3crop base, d3crop_m68 + m1 +3.8k, copy + m1 +3.1k (copy vs hybp
  -3.0k, SE 0.6k). hybp's line (hyb_nolp) was weaker than d3crop live (Kaggle 56665619), so the team's candidate stays
  d3crop_m68 + m1 + nighttrim + survivalfloor (BC is packaging it from bc_m3min; no submission).
- Pulsed sales, root cause: our farm creates evenly but the network's collect intent collects cohorts in batches (router drops
  none); the seller sells the batches through. collectall (collect every holder) lost on the M&M bed (confounded); pinned queued.
- Seller hold value (holdsteps=20, holdbest, hold 0.98): raises both farms' income, not the margin. Closed.
- Seller vs the field (Weaknesses' recsell: top team's recorded farm + our m1 seller vs our live sub): the field's day-long / dawn
  selling denies our sub $1.1-4.8k per game more than our seller on the same stock. The opponent there is our lineage (1-6% of
  the live field); seller variants must also not lose on pinned (field flow) and contested clones.

## Closed (do not repeat; details in the stop list)

- Every-day dawn slots (saleslots=1 / 2): production lost to the smaller first hire wave.
- Leader seller, two versions (an opponent that reads our plan; one that re-plans on a forecast of us): -141 / day on rung 1;
  -1.6k / -4.0k vs our clones.
- Funding one-piece fixes: cashroute, affordanimal (again), buyahead, lower turn cost for early hires, dropping the waters of
  unseeded plantings (it let underfunded plans pass).
- downprobe (faster hire search): -25% routing time, -16 / day.
- Forecast input fixes on this line: late-harvest History fix (no effect: the tf forecaster reads its own History), stockcap,
  BC's shed-estimate gate (-63 / day).

## Open, by measured size (updated Sep 30 01:25)

1. Forecast of the opponent's flow (BC's area). On pinned full games, the true recorded flow is worth +2.8k / game (SE 1.3k,
   median +1.8k) for the live line (BC's earlier dc11 probe: +8.8k, seller-only +5.1k, dawn-compile-only +8.1k); on rung 1
   +118 / day, half in the dawn plan and half in the hourly seller. Pinned opponents do not react and real ones are
   price-responsive, and no forecaster change has converted this against reacting opponents yet.
2. Early-game cash path (days 2-4 plantings, the day 2-4 cow). Ceiling is small: M&M's whole days 0-5 transplanted into its own
   worlds are worth +0.8k (Imitation); running the full ask unfunded (nofund=5) gained nothing. Every one-piece fix lost.
3. Network-side items seen from the compiler (findings/day_compiler.md): batched collection (but collecting everything loses),
   early asks draining cash so the land head skips day 6 on the copy / hybrid lines, feed intent.
Closed since the package: hold value variants, collectall, nofund, M&M's operating point (depcredit), speed (kernel check worst
call 2.67 s, overage left >= 46.9 s), rivalnight (denial over the hold horizon: pinned live29 -553, own -1.3k for opponent -0.8k),
landpull (the land 1.5-3 h after M&M's on the same state, but the hour does not limit the day's plantings; rung 1 +2 / day, SE 5).
Live fact for seller work (tools/dawn_react.py): the top-30 field's h0-2 sales do not react to our evening sales (slope ~0 over 183
games), so pinned (fixed opponent flow) is a fair bed against it; reacting followers are our own lineage (1-6% of the field).

## Tools (all in the dc12 dev tree `work/sep29_fund/bc_dc12`, build `work/sep29_fund/build_dc12e` incl. duel_mm)

- teacher_day env: TEACHER_ORACLE=1 (true opponent flow), DC12_ORACLE_MODE=1/2/3 (volume / shape / true), DC12_ORACLE_PRODUCTS,
  DC12_ORACLE_HOURS=a-b, TEACHER_REASON=1 (per-day compile reports incl. land hour and profile), TEACHER_DAYS=k.
- DC12_SELLLOG (seller decisions and night room), DC11_FUNDDEBUG (funding simulation; names the failing unit), DC12_ROUTEPROF
  (router passes), REPLAY_LEDGER / REPLAY_ORACLE (pinned games).
- Summaries in sep29_dc12/tools: margin.py, fidelity.py, cashpath.py, race_sum.py, ledger.sh / ledger_cmp.py, rung2.sh.
- Sep 30 probes: duel_mm DUEL_SELL "field" column (product waiting on own tiles; tools/made_phase.py), DC12_COLLECTLOG (per species
  collect intent / bound / dropped; tools/collect_sum.py), DC12_HOLDLOG (holdbest's chosen step), DC12_ORACLE_WHERE=1/2 (true flow
  in the plan / executor only, with DC12_ORACLE_MODE=3 DC12_ORACLE_HOURS=0-23; the mix probe needed a brace fix), DC12_LANDLOG
  (the network's land head; only in work/sep29_fund/bc_dc12h), tools/land_days.py (quadrants and NoLand by day).
- Sep 30 01:30-: tools/swap_products.py (swap bed arm vs ref by product and hour band, both seats), tools/dawn_react.py (live: the
  opponent's dawn share vs our previous evening), tools/land_hour.py (rung 1 land hour ours vs the teacher's), tools/land_asks.py
  (DC11_INTENTLOG now prints the land ask, land hour and quadrants; run one thread per file to keep games in order).
