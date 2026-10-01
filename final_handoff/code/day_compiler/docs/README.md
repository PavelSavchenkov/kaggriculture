# sep29_dc12: an M&M-like day compiler, designed from scratch

Goal (user, Sep 29): a day compiler whose agent plays as well as M&M or better. It is designed from M&M's measured behaviour and
the engine's rules, not by patching dc11. Every dc11 component is re-read in full for hidden assumptions before any of it is
reused.

Files:
- DESIGN.md: the dc12 design (engine facts, why dc11 sells late, the seller, milestones, judges).
- REWORK.md: dc11's assumptions vs M&M, the philosophy, the stop list, the requests to other sessions.
- AUDIT.md: the full-read audit of dc11, component by component (router: Day compiler; bind: BC; funding: Weaknesses; market
  and executor seller: Imitation).
- PROGRESS.md: this experiment's log.
- KEYS.md: every dc12-era compiler key with its status (live / used by other sessions / in test / closed).
- HANDOFF.md: the state for the user and the other sessions (live package, evidence, closed, open, tools).
- m1_saleslots3.patch, m2_nighttrim.patch, m3_survivalfloor.patch, m4_landfirst.patch: the root-cause fixes as small patches on
  bc_v95 + BC's local additions, applied in order (m1-m3 are live as Kaggle 56690263; m4 is judged clean, not yet packaged).
  The isolation ladder showed the dc11 core reproduces M&M's production, so dc12 ended as root-cause keys in the dc11 tree
  (dev tree work/sep29_fund/bc_dc12, build build_dc12e), not a new compiler; src/ stayed empty.
- tools/: the beds used here. duel_arm.sh / duel_sub.sh are the 6 pinned-shop M&M worlds with dc12 in M&M's seat or in our
  sub's seat (summaries: duel_arm_sum.py / duel_sub_sum.py). rsum.py / hdist.py / bands.py / cows.py / gate_sum.py summarize
  teacher_day runs on M&M's intents. margin.py / fidelity.py / cashpath.py read rung 1 / 2 runs; mm_bed.sh runs the 72-world
  M&M bed; made_phase.py (creation vs collection), collect_sum.py (DC12_COLLECTLOG), land_days.py (quadrant days) read the
  probes added on Sep 30. swap_products.py splits a swap-bed arm change by product and hour; dawn_react.py tests whether live
  opponents' dawn sales react to ours; land_hour.py (rung 1 land hour vs the teacher) and land_asks.py (land asks vs purchases
  from DC11_INTENTLOG) read the land runs. Sep 30: dep_bands.py (deposit / sale bands from a G1 CSV), band_rev.py (thin sales by
  hour band on the G1s / M&M-world bed), md5_score.py (rung 2 multi-day continuations: total value vs M&M at dawn D+k),
  g1_window.py (the summed one-day G1 differences over the same windows), cash_sum.py, sell_price.py, sd_dist.py.
- runs/: run scripts and their outputs (sf_*.sh G1s seller arms incl. sf_so216.sh on all 216 worlds; af760/ the line-B 760 trace
  split; md5/ rung 2). models/: small model dirs for arms (symlinks to the package / copy files plus a changed .dc11).
- teacher_day (dev tree tools_dc11) options added here: TEACHER_LABELS=2 (later labels translated onto our groups; rung 2),
  TEACHER_PRODUCT=<item> (per-hour field / pockets / shed / sold of one product, prefix productcsv).

Links:
- The dc11 reference (snapshots v62-v95, HANDOFF.md, PROGRESS.md): experiments/v10/sep25_compiler_overhaul/.
- The team's findings (one file per session) and the stop list: work/mm_handoff/FINDINGS.md.
- The judges: Imitation's duel beds (experiments/v10/sep28_top_lb_imitation/runs/duel/: mm.sh 48 M&M worlds, pin.sh 6 worlds)
  and the local league (runs/league/league.sh); Weaknesses' 760 + wide bed.

## Gate ladder (user, Sep 29 19:35): isolate the compiler first, then widen

Each change is traced to the model term that causes it, not to the symptom ("sales dropped at h1" is a symptom; the term in the
DP / router / forecast that makes that decision is the cause).
1. One day from M&M's recorded dawn, M&M's own intent, against the recorded opponent (teacher_day, list_mm40, days 12-17; 240
   days). Measures: our margin vs M&M's actual day (value_next - opp_money_next, both vs M&M), per-hour sales, crew / routes
   (rsum.py), race_sum.py. Model checks: TEACHER_ORACLE=1 (true opponent flow) splits forecast error from objective error.
2. 5 / 10 days from M&M's dawn (TEACHER_DAYS=k; TEACHER_LABELS=2 compiles M&M's later labels translated onto our groups): does
   the compiler leave a worse state. Status (Sep 30 05:58): parity from day 12; day 10 loses Q4 plantings to melon cash timing.
3. Reacting local opponents from M&M worlds (duels, league), then contested beds (opponents with the same order-slot rules).
4. Pinned live games (real opponents' recorded play, non-reacting) and Kaggle games.
Rung 1 status (19:40): our compiler is +90 / day margin above M&M's own day on its states; the oracle flow adds +86 / day.
