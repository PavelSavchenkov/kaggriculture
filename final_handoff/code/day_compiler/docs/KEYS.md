# dc12-era compiler keys (dev tree work/sep29_fund/bc_dc12): status, Sep 30 11:12

All keys default off. Keys from the dc11 era (v62-v95) are documented in experiments/v10/sep25_compiler_overhaul/. Evidence
lines are in PROGRESS.md and work/mm_handoff/findings/day_compiler.md (stop list).

## Live (Kaggle 56690263, d3crop_m68 package)

dropany=6, cashsell=1, wheatcash=1, reserve=0, saleslots=3 (m1); nighttrim=1 (m2); survivalfloor=1 (m3; required with reserve=0).

## Used by other sessions' candidates (keep)

- achieve=1, feedcost=1: the copy line (copy_or_v5d_af). They lose on the package (760 -439), and the package's funding keys lose
  on the copy (league -1.3k / -1.5k): each key set co-evolved with its network.
- sellmodel=1-5, sellmodelfirst / sellmodellast: BC's learned seller (v5t tests; level with the DP on M&M's stock, -1.7k in full
  games on our timeline).
- landfirst=1 (m4): judged clean on its bed, not packaged.

## In test (Sep 30)

- landcash=T + router cash_pass: day-10 Q4 funded from same-day melon sales (league: package +333, SE 271; copy +90).
- feedvalue=1: feed stops valued at the banked product (BC audit item 3). Package: league +747 (SE 204, 480 games), the 760 -241
  (SE 152), swap bed running; copy -517 (block 501). Portable: m11_feedvalue_for_bc_m3min.patch (bc_m3fv = dev build 12 / 12).
- saleslots=4 (task d): the first hire wave leaves the h0 sells their slots, counted from the seller that sells (model thin lots max the
  DP's, DP other products, dawn pockets included; v3). Package: level (+5, 192 games). Copy + v5t: league rerun running.
- slotcash=1 (astra-002): sells fitted to the free slots before their cash funds purchases; league queued.
- reserveuntil=D (Sep 30 11:10, build_dc12v13+): the next-dawn reserve applies on days < D only. On the copy's land day the reserve
  (tomorrow's feed x 1.2 + $30, ~$500) blocks the plans that buy all seeds; M&M ends day 6 at ~$73. From M&M's dawn 6 (60 worlds)
  reserve off: crops 15.75 -> 16.42 (M&M 18.63). Own states, 21 short worlds: reserveuntil=6 17.65, + shadowskip=2 18.80 (copy
  15.45, M&M 19.9). Use with survivalfloor=1. Full-60 read to day 11: runs/own60ru.
- shadowskip=1 / 2 (Sep 30, build_dc12v10 / v11+, needs achieve): cashshadow variants with the horizon at the funding simulation's
  first seed cut; 2 = after the deferral search, the achieved variant re-routed with that horizon. Alone ~0 (its variants fail the
  reserve); with the reserve off +0.1 crops from M&M's state, larger on own states (see reserveuntil).
- rivalfrac=1 (Sep 30, build_l2g for the G1s tree, build_dc12v15 dev): the margin term's opponent units per hour by cumulative
  rounding (lround per hour dropped flows < 0.5 / h: 15-60% of the afternoon flow). G1s 48: +801 (SE 241) vs the package seller,
  level with rivalnight 0.5 (+60), not additive with it. Recommended for every line.
- reserveland=1 (build_dc12v15): no next-dawn reserve on a day whose plan buys land. Own states ~= reserveuntil=6 (day 10 loses).
- hirecheck=1 (Sep 30, BUG FIX, m14 patch / build_m14f, dev v22+): the funding check fails a plan whose hour's hire wave fills only
  partly (it read the first hire order's fill); repair = the crew the cash covered. With latehire=1: M&M-intent day 6 +1.0-1.3 crops,
  day 10 -0.3-0.6 crops (+$240 value). Rare on own trajectories (0 in ~25 full games per line). Full-game check running.
- waterdaily=1 (Sep 30 evening, src_m13w / build_m13x only): retained ongoing crops watered every day as a P_EXTRA stop (value 1) when
  not dry and no doubled production tonight, like M&M. Yield-neutral by the rules; it changes the network's next-dawn dry inputs (C1).
  Copy line Stage 1 running (runs/wd).
- keepfed=1 (existing key): feed an unfed-yesterday animal when 0.8 x remaining productions x current price beats every-other-day wheat.
  Overrides the network's release (C3b); copy screen running (runs/kf).
- duel_mm DUEL_OPS counting fix (not a key): build_m13o (h23 water / feed / care from actions), build_m13x (exact h23 unit-phase replay).
- mmfaith=1 (G1s tree build_l2j only): M&M's fitted selling rule; closed (G1s -5.3k vs M&M's own lots with matched volumes).
- cashrival=1, shadowskip=3: closed (no effect / +0.2 crops).
- Probes (live executor only from build_dc12v14): DC12_SEEDLOG, DC12_SEEDTRIM (cut seeds with the free / reserved stock value),
  DC12_CASHPATH, DC12_RIVALFC (dawn opponent forecast per hour), DC12_SLOTLOG / SLOTDET / CASHCUT / SELLLOG / ROOMLOG. Before v14 the
  slot / cash-cut probes also printed the funding simulations' executor runs (counts retracted).

## Closed (safe to remove after the tests above; one-line reasons)

- Seller: rivalnight, fieldflow, ticksell / tickdawn / tickegg, scenopen (league +38, SE 231), response, stockcap, lumpy, race,
  leader, hourdisc, holdown / holdsched / holdsteps / holdbest (holding raises both farms' income, not the margin), lotcap.
- Funding: reservek, reservenet (760 -718 with achieve), nofire, latehire, cashshadow, cashroute (its horizon was h0 on land days;
  superseded by landcash), affordanimal, buyahead, animalfirst (copy league -399, SE 354), seedlag, skipwater, cashseeds,
  lazyfeed, landpull, nofund (probe).
- Entity values: entityvalue (alone -206, with feedvalue -327 vs feedvalue).
- Deposit timing: depcredit / dephour / depfert (league +0.36k, 760 -0.8k; + scenopen 760 -1.2k).
- Routing / other: downprobe, collectall, trimnet, regime* (regime M), bandsell / respsell / resp* / bandlot / bandroom and
  dawnsell / dawnstart (test-opponent sellers, not for our agent).
