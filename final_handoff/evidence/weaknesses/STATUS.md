# Status (Oct 1 00:05 BST) — FINAL: work finished (user), all jobs stopped

Role: Stage-2 judge (judge.sh), M&M pulls (mm/, every 3 h), held-out / reserved lists, guard beds (G3, swap, field opponent), live
monitoring, astra triage. Findings: work/mm_handoff/findings/weaknesses.md; log: LOG.md; hand-over: work/mm_handoff/roles/weaknesses.md.

## Evening conclusions (all swap reads on CMA-free worlds; user rule: no CMA agent in any role)

- Forecaster: package + fc3vens (BC's clean fc3v 3-seed ensemble; bed worlds out of training and selection) is positive on every bed:
  G3 PROMOTE (+1,053 fixed 96), swap pooled 147 CMA-free +1,445 (SE 428; origin-weighted +1,201, SE 274), field opponent +800 (SE 505).
  Stack judged: pkq_fc2 base on build_imf1 without hirecheck / latehire (the final bundle has them). Clean-recipe control.
- fc2-family swap reads leak (fc2 trained on bed worlds; our real sub replays its game): pkm1 slice trained +4.3k vs untrained -0.8k.
  Judge learned components on worlds outside their saved TRAIN / selection (swap/train_split.py).
- Copy vs package in our top-30 worlds: copy + fc2 vs package +325 (SE 514, 93 clean worlds); copy + fc3vens vs package + fc3vens
  REJECT (-2,410 at n 34): the copy's M&M plan mix (less melon / strawberry / wool, more wheat / eggs / tomatoes) costs own money.
- Divergence vs the top-30 field (swap/divergence_swap.py, opening_swap.py): Q4 ours 99-100% vs field 21-29% (top 10 mostly 3Q;
  4Q top-10 games are our worst losses, -4.4k, observational); land-day wheat a day late; copy over-waters even days (+11%), care -5%;
  opening: we hire fewer (52 vs 61-66) in every band, incl. weak teams (not a strength marker).
- Field opponent (arms/opp_field = BC field_s0 + v5t seller): its tomato flips = a day-10 Q4 land ask (teams ask 29%, clone 0.29); read
  all fixed worlds (swap/field_read.py). field2 (landfirst) dropped.

- PRE-LIVE CHECK of the exact bundle packages/m3fc3vens_hc (package + fc3vens + hirecheck / latehire, build_m14bf): G3 first-96 PROMOTE
  at 48 (+1,111 SE 460), swap 93 clean PROMOTE at 24 (+1,947 SE 688); equal to the clean-recipe control. Submission = the user's call.

## Live

- pkm1 = Kaggle 56690263 (m3 package), rating 2808; pkm3cma = 56706309, rating 2715 (0 / 6 vs top 10); CMA line dropped by the team.
- Confirmation reserve: live/confirm/README.md (B_unscored 10 + pkm1 games after 14:30 UTC untouched); M&M reserve mm/reserved_confirm.txt (131).

## Running / waiting

- Waiters: M&M pull ~20:48 (after_pull + profile), pkm3cma 10 top-10 games. astra loop paused by the user (last pass 17:15 UTC, sealed 17:20).

## Final (Oct 1 ~00:00)
- Final Kaggle pair (user-asked): 56720080 = base m3 resubmit, 56720831 = honest1 (#1 minus nearanimals, clean fc3nvens forecaster).
- Pick-2 evidence: swap bed honest1 - m3_fc3nv +613 (SE 392, ~126 worlds), exact LB +1,314 (SE 715, 150 games).
- Local-LB: f1 (m19-fc2ens) #1 1647-1651 (merged #248); honest m3-fc3nv #3 1523.7. Exact LB judge: work/sep29_validation/lb (83 / 83 match
  of f1's official games).
- All Weaknesses jobs stopped (swap, LB judge, live loop, M&M pull loop, waiters) on the user's request.
