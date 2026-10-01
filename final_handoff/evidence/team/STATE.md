# M&M imitation: current state (one page; read this first; Imitation keeps it current)

Updated Sep 30 14:30 BST. History: GATES.md, DEVIATIONS.md, findings/*.md, sep29_mm_copy/LEDGER.md (archive, not needed to start).

## Goal and the one number that matters

Beat M&M (#1) on Kaggle. Live rating of every submission since Sep 28 sits at 2770-2849 despite +1-5k local gains; no local bed has
predicted the live result reliably. M&M's measured edge over our package in M&M's own worlds (G3-wide, 278 held-out worlds, M&M's
frozen recording vs the package): +1.43k / game (SE 0.38k), M&M wins 67%. That is the gap to close first.

## Best agents now

- Submitted: package d3crop_m68_m3 (Kaggle 56690263), rating ~2770-2790, vs top 10 ~55% wins (n ~15).
- Package candidate: + feedvalue=1 (Local-LB branch 3ad3ef3, not on Kaggle): league +0.75k, swap +0.43k (top 10 +1.29k), G3 all +0.03k.
- Copy line: copy_or_v5d_af = level with the package (swap +0.14k, G3-wide -0.40k). copy_or_v5d_af_sbc1 (BC: seed-blind +
  grid-type-blind, style 1): quick 60 +1.13k (dev-set optimism); clean 99 (confirmation) +0.08k (SE 0.46k), own +0.97k, opponent
  +0.89k, score 58.6% vs 51.5%: no verdict. The network fix raises our income and the opponent's equally.
- Seller line: bug fix rivalfrac=1 (the DP's margin term rounded each hour's expected opponent units, losing 15-60% of the opponent's
  afternoon flow): G1s +0.80k (SE 0.24k) vs the package seller, = rivalnight 0.5's gain. BUT pk_rn05 (package + rivalnight 0.5)
  REJECT on G3 218 at 96: -0.46k (SE 0.45k), own +0.32k, opponent +0.78k. The G1s gain (M&M's recorded farm) does not carry to full
  games with our farm; rivalfrac held (key in builds, off) until the split explains why.

## Since 12:00 (read this first)

- Top teams CONVERGED on one style Sep 27-30 (M&M included; likely a public notebook; Weaknesses searching): dawn + morning lots,
  evening eggs, d6 ~19 / d8 ~17 plantings, geese ~7. Forecaster must be trained / judged on Sep 27+ games.
- Our live opponents (48 h): M&M 0.2%, 2-10 15%, 11-30 27%, 31-100 21%, 101+ 37%. All local beds' live opponent is our own sub.
- M&M's seller = own state + price rule (opponent adds <= 1 pt of deviance): milk sells 35% of stock at a high-price dawn, wool
  steadily above price 24, eggs at h21-23. Our seller has the dawn stock but holds it to h21-23 (Day compiler: deposits are not the
  cause on days 12-27). Faithful M&M-rule seller (Day compiler, mmfaith) fails reproduction on M&M's farm so far (-15k): being fixed.
  Table-rule floor under the DP beats M&M's own lots on M&M's farm (+0.48k) but not yet in full games on ours.
- Forecaster (BC): deployed big2 had stale data (to Sep 26), hour-rotation augmentation (wrong + inconsistent), per-product history
  only; on real top-30 opponents its error is the evening (M&M wool 1.6x), dawn excess only vs our own subs. fc1 / fc2 retrained,
  scoring pending.
- fc2 (retrained forecaster) status, clean reads only (worlds outside fc2's training): G3 M&M worlds +1.42k (n 35); swap top-30 +0.57k
  (n 49, SE 0.63k); current-package top-30 slice -0.84k (n 32); swap ranks 31-100 +1.67k (n 32); vs field-like opponents +0.72k
  (flip noise) / +0.08k (n 96). The swap PROMOTE was partly a TRAINING LEAK (fc2 trained on those very games: trained worlds +4.3k).
  Rule: learned parts judged on swap beds exclude those worlds from TRAIN + checkpoint selection.
  Sep 30 evening: fc3vens (clean recipe, no swap world in training) G3 +1.05k at 96 (SE 0.37k) PROMOTE, swap no-CMA weighted +1.20k
  (SE 0.27k; every origin band positive; top 10 +4.1k), field +0.80k (SE 0.51k). Forecast confirmation on 25 unseen live games:
  learned heads MAE -6.6% vs big2, equal among themselves. Bundles (BC, local, not pushed): m3fc2ens_hc and m3fc3vens_hc (sha
  00c47802..., 4/4 verify, deadline emulation 0 overruns, overage >= 31 s). Submission only on the user's ask.
  NO CMA (user rule 16:40): no CMA decode / agents in any role; m3cma (56706309) live 2704 < m3 2808; swap lists *_nocma.txt (206).
  Copy + land keys + fc2 (cp_fc2): clean 99 PROMOTE +0.98k vs copy, +0.86k (SE 0.84k) vs M&M's recording; swap next.
- Closed today: statistical copy of M&M's hourly selling (mmfaith v2: right volumes / hours, -5.3k on price per unit); denial keys
  also vs a dawn-selling proxy opponent. Day 10: our compiler plants 94% of M&M's own day-10 intent; the gap is the networks' ask.
- Seller-key sweeps (rival / rivalnight / rivalfrac / hold / morning floor / forecast multipliers / oracle) all failed in full
  games: closed.

## KEY FINDING (Sep 30 ~20:50): vs a FIXED real opponent our agent = M&M
- Opponent-replay bed (M&M's worlds, M&M's real opponents' recorded actions, identity 48 / 48): the PR agent (56714867 + splitfert +
  nearanimals=6) minus M&M's own recorded play: +0.15k (SE 0.76k), own +1.06k, opp +0.90k; hand-over at dawn 5 / 10 also level.
- So the live gap (M&M +5.5k vs top 30, we ~50%) is interaction: how reacting opponents respond to our play vs M&M's.
  Weaknesses: live comparison of top-30 teams' behaviour vs us and vs M&M. Local beds cannot measure it (lineage opponents
  react like us; replays do not react).
- Closed this evening: more compiler time (-0.55k), late-game night carry (-2.3k), long-term site cost, crop reservation.
- Next rule: BC's opening script (the four converged teams' fixed crew / delivery table, days 1-10) as an openscript key (Day compiler).

## CURRENT GOAL (user, Sep 30 ~21:00): beat Local-LB #1 with the old day-intent interface
- Target: pavel-bc-opus-v17d-dc12m19-fc2-m68 (live 56714867 + splitfert=10 nearanimals=6), Local-LB 1637.9 (190-80).
- Intent-v2 design CANCELLED. Levers: network ensembling / conditioning (BC), forecaster retrain (BC), DP objectives /
  early sales (Day compiler), package / decode combos (Improve Agent). Judge = exact Local-LB replay (Weaknesses).
- RL / PPO / CMA agents are overfit (user): not targets, screen opponents or evidence.
- Judge (Weaknesses, work/sep29_validation/lb): LB's own lbgame.py, identity 6 / 6 exact; 90 games per arm = the blocks a new
  challenger gets vs m19-fc2 / m14-fc2 / mmpq-policy-v2, both seats; paired vs m19base; lbsum.py. Send it a model dir (only model
  files differ from m19-fc2) or a full agent folder.
- Pre-screens (Imitation, G3 24 worlds with #1 as the live opponent, runs/fieldopp): h2h_* = BC's lb1 arms; lin_* = arms closed on
  the realistic beds but untested vs our lineage (openscript=1 / 2, dawnshare=50 milk+wool / all four).
- Why lineage-specific arms: every Local-LB opponent is our lineage and sells late (wool h6-8 in the opening, evening dumps later).
  The opening keys (m19) gave +90 rating: m19 beats m3 22-8 (+2.1k) while m14-fc2 beat m3 only 12-18.
- Candidates (Sep 30 ~21:30 BST; all file-only on the m19 bridge unless noted):

  | arm | change | G3 vs #1 (24) | other reads | status |
  |---|---|---|---|---|
  | c2_rec110 | main condition recency 1.025 -> 1.10 | 16-0-8, +0.66k (SE 0.79k) | | judge next; hybrid guard running |
  | f2_fc3vens | 3-seed clean forecaster | 0.54, +0.13k | league +0.27k (0.79 vs 0.70) | judge queued |
  | stk_c2f3s4 | c2 + fc3vens + saleslots=4 | | | judge queued; hybrid guard running |
  | f1_fc2ens | fc2 ensemble forecaster | 0.46, -0.60k | judge vs #1 +0.48k (n 27, flips 10/2) | judge running |
  | f3_fclin | lineage-weighted forecaster | | judge n 9 -0.08k | judge running; LB-specific, needs hybrid guard |
  | pr_ss4 | saleslots=4 | | DC lineage league +0.35k (SE 0.75k) | judge queued |
  | c1 / e1 / c3 | strength cond / mainweight 0.4 / both conds | 0.50 / 0.42 / 0.38 | | judge last |
  | closed | openscript 1/2 (-4.3k / -2.4k vs #1), dawnshare (level / -0.9k), rival 1.5, hourdisc 0.98, leader 0.5, feedvalue, wheat floor | | | |

- FINAL (Sep 30 ~22:15 BST): pavel-bc-opus-v17d-dc12m19-fc2ens-m68 (f1 = #1 + fc2 seeds 1 / 2) merged as Local-LB PR #248.
  Gates: exact judge +922 (1.9 SE, wins 26 / 8), hybrid +610 (SE 406), frozen LB replays +4,085 (SE 476), deadline emulation
  clean. Every other arm failed vs #1 (table in sep29_mm_copy/LEDGER.md). User rules: no lineage-weighted parts (fclin), minimise
  overfit-prone techniques at the end. Open lever for later: network-side late planting (findings/day_compiler.md 20:55 UTC).
- OVERFIT AUDIT + HONEST LINE (Sep 30 ~22:50 BST, user ask): the LB rewards lineage fit.
  - nearanimals=6 and fc2's training data (47% of recent sequences = our own games) are lineage-only.
  - LB costs of dropping them: honest1 1531-1585 vs f1 ~1750.
  - Real-opponent beds are level (hybrid, one-day live continuations).
  - Honest entry pushed: pavel-bc-opus-v17d-dc12m3-fc3nv-m68 = m3 (most-tested, 152 live games) + clean fc3nvens forecaster.
    Vs m3: live continuations +611 (SE 237), hybrid +1,313 (SE 608), exact LB +82 (proj 1553 vs 1523).
  - Old v59 d3crop sub: worse than m3 on live continuations (-926).
  - Next honest lever: re-select the main-net seed off the LB (BC arms on m3_fc3nv; hybrid + one-day live beds).
- FINAL (Oct 1 00:05 BST): user-asked Kaggle pair = 56720080 (base m3 resubmit) + 56720831 (honest1 = m19 build without
  nearanimals + clean fc3nvens + splitfert + hirecheck); both checked, validation episodes reproduced exactly. User: done, all
  sessions stopped.
- KAGGLE (Sep 30 ~22:20 UTC, user-asked): the base m3 was resubmitted as 56720080 (exact archive of 56690263).
  - Checks: local verify, kernel v2 (2.59 s worst call, overage >= 44.1 s), validation episode reproduced exactly.
  - Active pair: 56720080 + 56714867 (fc2 + hirecheck), in the same window, so this is a clean live A/B. Weaknesses tracks it.
  - Second-pick candidates: bundles ready, NOT submitted (user's call). m3_fc3nv (sha 7c9e5fcc) or honest1 (sha b8ac48f8).
    Opponent-replay bed read of both + m3 is running.
  - Main-seed soups: closed (hybrid -964 / -388, live continuations -307 / -213 vs m3_fc3nv).
- Push rule: a Local-LB PR only for an arm the exact judge promotes AND that is not clearly negative on the hybrid bed
  (runs/hybopp vs h_pr). Improve Agent builds the package; Imitation pushes the branch.

## Sep 30 ~21:40: live gap sources + deadline candidate
- Live T-response (Weaknesses): top-30 teams earn +4.6k more vs us than vs M&M from day 11: wheat +2.9k (M&M plants ~40 more
  wheat tiles d10-28, sells 113 more units), wool +2.1k / milk +0.65k (mid-game sale timing: teams sell ~30% at dawn, we 46%
  of milk at h21-23). Levers: wheat crop mix (Improve Agent probe), two-day dawn seller (Day compiler; diagnosis: one-day
  horizon ignores what an evening dump leaves in tomorrow's dawn book).
- New bed: runs/hybopp (build_m14or7 DUEL_OPP_SELL, LIST=runs/fieldopp/list_field.txt): the real team's recorded farm +
  learned M&M seller; sells at real hours, money within 0.3k of real. First local bed where timing / volume denial can show.
- User deadline 22:30 BST: candidate = experiments/v10/sep30_pkg_improve/packages/pavel-bc-opus-v17d-dc12m19-fc2-m68 (fully
  verified; Local-LB branch pushed). Closed tonight: opening crew script, land-day copy asks, more compiler time, night carry.

## LEAD (Sep 30 evening): early milk / wool deliveries in the opening (days 3-9)
- Copy vs M&M on G3: the copy's whole gap is days 3-9 execution (+2.9k for M&M's days 3-9), not plan items, not the seller given
  equal shed stock. Mechanism: our routes deliver wool / milk after other stops (h6-8, with the opponent); M&M sells at h3-5, first.
- Upper-bound test (runs/early, carried milk / wool into the shed at once on days 3-9, our DP decides; 24 G3 worlds): copy +1.94k
  (SE 0.76k, opp -1.50k), live package +3.09k (SE 0.95k, wins 20 vs 9). Days 10-29 early delivery hurts (-1.38k): our DP dumps at
  dawn and the opponent sells later into the drained book; first-mover pays only in the opening (few shops).
- Real routes cannot reproduce it (Day compiler, package, 24 worlds): earlydep (accepted rarely) -0.69k, earlyforce (no-op),
  woolfirst -0.85k (collection unchanged), woolfirst + 2 hires -2.65k (wool later); layout equal (sheep 1.8 vs 1.5 steps).
  M&M's first-wave hires harvest ~1 sheep each and return (hire-days 54 vs 40); ours bundle ~3 sheep + fertilizer per hire.
  Those forced variants are closed. NEW (Codex session, work/sep30_early_products, verified by Imitation): the missing piece is the
  fertilizer collection bound into our output stop (every wool / milk unit waits ~1 h; M&M 1%). 'Courier' patch (days 3-9 harvest-
  only short trips from actual spawn tiles, fertilizer / service later, same crew): wool sold by h5 8.2 -> 14.7; v1 n24 +1.54k
  (SE 1.41k); v2 n44 +1.29k (SE 0.86k), own +1.59k, opp +0.31k. Extending; integrate into the compiler if it holds at 96.
  Evening: splitfert (fertilizer unbundled; router-chosen short trips) 96: +0.31k (SE 0.47k); + sheep-first nearest placement
  (sfna) wool by h5 29.5 (M&M 20) but +0.38k (62): days 3-9 gain = teleport's, lost on days 15-24 to the opponent's milk (cows
  pushed out). Teleport holds on 71 worlds (+2.82k, SE 0.54k). Next: balanced placement (M&M: sheep ring 1, cows ring 0 / 2).
  Live: 56714867 = m3 + fc2 + hirecheck / latehire (user-asked, 17:44 UTC), active with 56710248.
  Placement from BC's M&M census (nearest free structure; order cow, sheep, sheep, then sheep + cow; pastures before coops):
  splitfert + nearanimals=5 fixed 96 +0.96k (SE 0.52k), na6 (+ cows / sheep before geese) +1.17k (SE 0.40k), flips 24 / 10.
  VALIDATION BIAS: all local opponents run our compiler and sell wool at h6-8; real teams at h3-5. On the real-opponent
  bed (recorded opponent replayed with loans, runs/opprec): na5 +0.48k (SE 0.49k), own -0.82k, opp -1.30k -> smaller live
  value expected. na6 CONFIRMED on 122 untouched G3 worlds (+1.24k, SE 0.40k). Local-LB PR pushed: submit/pavel-bc-opus-v17d-
  dc12m19-fc2-m68 (= 56714867 + splitfert=10 nearanimals=6). Open: stacking on the forecaster bundle (G3 na5 on bundle -0.32k,
  real-opponent +0.18k; na6 on bundle running); PR agent vs live agent on the real-opponent bed (96) running.
  CLOSED: late-game night carry (regime from day 10) -2.3k real-opponent, -5.0k lineage (DP sells the night stock that evening).
  NEXT: BC intent v2 (separate small MLP: crew, deliver_by, fert_mode for the opening; placement stays a rule); Day compiler slim executor.
- Copy vs package with the same forecaster (fc3vens, 93 clean top-30 worlds): copy -2.41k (SE 1.08k, n 34) REJECT. Live line = package.
- Deployable now: package + fc3vens: +1.03k (SE 0.52k) on 93 clean swap worlds, +1.45k pooled 147, G3 +1.05k, field +0.80k.
  Exact bundle m3fc3vens_hc (with hirecheck / latehire) being confirmed on G3 96 + swap 93 (Weaknesses). Submission = user's call.

## Macro divergence from M&M (user priority 1, Sep 30 evening; map: experiments/v10/sep29_mm_copy/runs/divergence/MAP.md)
- Copy (M&M-trained network + our compiler) stays close to M&M's plan in M&M's worlds; from M&M's exact dawn 8 / 10 (hand-over runs)
  it plants M&M's crops that day. Its land-day lateness in own games (d10 wheat -2.1, caught up d11) is state drift built before the
  land day: dry strawberries from our every-other-day watering read as trouble by the network (73% vs 9% dry at dawn 10).
  waterdaily (compiler) moves the asks toward M&M's but costs labour (-5.45k); optional waters are never done. Closed on the compiler
  side. Dry-blind inputs re-tested under the current compiler: runs/dryb (early read negative on day-10 wheat; diagnostic running).
- Copy's extra sheep / cow releases d15-21 = value-rational reaction to crashed books (our package opponent crowds wool / milk);
  keepfed -0.33k, closed. Care gap 0..-7% (late network care share). Old water / care gaps partly the h23 counter bug (fixed builds
  build_m13o / m13x / imho). Carrots +0.8 / day even from M&M's dawn = the copy's own ask (BC attributing).
- Package (live m3): plan gaps are its networks' asks (not M&M copies; ensemble land averaging + v219 forced Q4 without crop
  conditioning; day-10 ask 8 vs M&M 19). Compiler executes M&M's own intent (d10 ~94%, 0 extra escapes).
- Vs the top-30 field (swap, no-CMA): top teams go 4Q in 21-29% of worlds (we 100%); 4Q top-10 games are our worst (win 17%).

## Open blockers (ranked by measured $)

1. Opponent income (denial). In M&M's seat the copy earns about M&M's own money (own -0.31k) but the opponent earns +1.44k more
   than against M&M. On M&M's own stock our seller gives margin -0.45k (SE 0.35k: own +1.37k, opponent +1.81k; valid in-place bed).
   Where it comes from (11:25): our seller moves milk / strawberries / wool from h0-11 to h21-23; the opponent then sells at h12-20
   into a fuller book (+1.7k strawberries, +1.6k milk, +1.4k wool there). M&M sells in the morning, before the opponent's afternoon.
   The DP's existing opponent term (rival=1) is worth +2.1k margin (rival 0: -2.1k); rival 3 no better; rivalnight 0.5 / 1 and
   rival 2 +0.72k, rival 3 +0.71k, rivalnight 0.5 +0.74k (promoted) vs the package seller; vs M&M's own lots +0.27..+0.30k: on
   M&M's stock our seller is now level with M&M's. Next (Day compiler): the opponent forecast per hour vs its real sales (hourly logs
   mm_g1s_{rpS,dpS,r3S}); per-product isolation (mm_g1s_dp_p3/5/6/7) queued.
2. Own-state drift of the copy network (D11), valued -1.7k at M&M's prices (melons d1-3 -1.46k, herd mix / wool -0.93k, tomato
   wave 2 -0.66k). sbc fixes the asks on own states (melons 4.0 = M&M, tomatoes d12-19 9.9 vs 10.2, herd ~M&M). In G3 218 now.
3. Land-day funding (day 6, also days 8 / 10 in the package: new quadrant planted a day late). Binding limit: the next-dawn reserve
   (~$500 kept at night; M&M ends day 6 at $73); second: pocket-carried fertilizer / wool reaches the shed only at night. Fix in test
   (Day compiler): reserveuntil=6 survivalfloor=1 shadowskip=2 -> day-6 plantings 15.45 -> 18.80 (M&M 19.9) on 21 short worlds;
   full 60 to day 11 running (~12:00). Combined build planned: build_dc12v14 saleslots=4 slotcash=1 reserveuntil=6 survivalfloor=1
   shadowskip=2.
4. Wheat d8-10 -2.8 / game (half network d10 ask, half land-day compiler drop). Carrots d12-19 +2.4-2.9 (network following
   the newest M&M sub vs 4Q opponents; low priority).

## Proven non-blockers (do not reopen without new evidence)

Network capacity (G2 71/71 held-out M&M states); per-unit sale prices (= M&M); learned hour-by-hour M&M seller I1 (-1.97k vs M&M lots on 4 products: wool, end-game eggs; old "sells
inputs" story retracted); h0 order-slot cut (fixed, 0% cut, but no money: ss2 swap REJECT -0.45k); opponent forecasting
by team label (oracle ~0; but an hourly-sales oracle was worth up to +8.8k on pinned games: forecasting the opponent's hourly sales is
NOT closed); opponent grid inputs (< 0.03 / day); herd size (0.2-0.6k); forcing Q4; watering (dryblind -1k); macro dials; overnight
pockets (engine clears them); land asks (98% / 100%); services / field units d6+; tick / fieldflow / hourdisc seller rules.
Measurement traps: the 6 exact worlds (outcome-selected), the league (our own lineage), clone beds (did not predict the top 10).

## How we test (work/runq/README.md)

Stage 1 (seconds-minutes, `slot.sh -p hi`): G2 / own-state intent splits / G1s seller bed / day-limited continuations.
Stage 2 (survivors only, one per session): G3 218 -> swap 234, arms interleaved, seqwatch stops at the first verdict (24/48/96/192).
Final: league + 760 collapse check, then a live Kaggle A/B only when the user asks. Every process via work/runq/slot.sh (18 total).

## Lines and owners (everything else parked)

- L1 copy trajectory: BC (network, Stage 1 own-state splits) + Day compiler (combined build: ss4 v2 + slotcash + land-day fix,
  ETA 12:15) -> Imitation integrates sbc1 + that build -> Weaknesses judges (G3 218, swap).
- L2 denial: Day compiler + Imitation: DP margin objective on the G1s bed (own and opponent money, 48 worlds, minutes per arm).
- Fallback if L1 fails on G3 218: port M&M's traits into the package via BC's zero-cost steering (table in progress).
