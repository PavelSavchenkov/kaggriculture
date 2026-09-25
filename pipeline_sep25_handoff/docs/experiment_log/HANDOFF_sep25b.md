# Handoff: sep24_BC_opus, Sep 25 afternoon (supersedes HANDOFF_sep25.md where they differ)

Read PROGRESS.md (audits from 11:55), IDEAS_LEDGER.md (sections from Sep 25 12:30) and
work/sep25_bc_weakness/REPORT.md (another session's failure analysis) first.

## Standing rules

- User goal (24 h from Sep 25 12:15): strongest day compiler + BC agent vs the Local-LB and its
  strongest agents, best unseen-seed performance; audits every 30 min; research online.
- RAM: one training process at a time; <= ~20 game workers (another session also runs games).
- Evaluate on unseen seeds only: seeds 700-731 selected the Vadim opening (winner's curse).
  Broad panel: scripts/panel_lb10.sh (all 10 Local-LB agents, data/localLB_main); select on
  seeds 1300-1311, confirm on 1312-1323 (the baseline covers 1300-1323: reports/lb10/base).
- The Local-LB's own games are reproducible: opponent i (alphabetical active list) plays seeds
  1000*i .. +19 both seats; scripts/lb_seeds.sh replays them (exact W-L match, 10/10 opponents).

## Agents

- PR 1 (user opened): submit/pavel-bc-opus-v12-vadim = v12_cond + Vadim opening + feasibility-first
  compiler (decision-identical to the old one, -33% compile). Local-LB: 333/400, Elo 1891.7.
- Branch 2 (pushed, PR not opened yet): submit/pavel-bc-opus-v12-herd = same network + X1 compiler
  (day 0 trims crops before animals; next-dawn reserve from day 1). Unseen seeds: +7.4k, wins
  85% -> 100% (selection); +5.8k, 87% -> 95% (holdout). Package submissions/sep25-bc-opus-v12-herd.
  Replay of the Local-LB seeds: reports/lbseeds/herd (compare with reports/lbseeds/base).

## Compiler state (source/compiler.cpp, day_policy_local)

- Defaults: feasibility-first hire search (DC10_HIRE_SCAN=1: old scan), X1 (DC10_TRIM_D0_ANIMALS=1,
  DC10_RESERVE_FROM_DAY=0: old rules). Knobs kept for experiments: DC10_TRIM_MODEL (rejected),
  DC10_LAND_FIRST (no-op), DC10_RESERVE_FARM (rejected vs X1), DC10_RACE / DC10_RACE_DEADLINE /
  DC10_SALE_TIE_NOW (melon race; rejected vs X1), DC10_TRIM_CROPS_FIRST (all days: -12.8k).
- Checks: scripts/opt_check.sh (8 recorded games, compares reports + per-day action hashes).

## Data and models

- Replay DB (xishengfeng/kaggriculture-replay-db): 41,423 verified episodes -> data/traces_db,
  corpus_v7db (57,607 perspectives), arrays_v7db (1.73M days; coverage gate 0 failures).
- v16_db: v12 recipe on arrays_v5 + arrays_v7db, 80k steps (training). Candidate folder must omit
  model.bin.style (style 0 = average unknown team, bad); copy model.bin.opening "6 8".
- v14_all (v13_w384b + v6x): not better than v12. v15_w512 stopped at 48k.

## Running / next

- reports/search_space: per-dawn search headroom, network pushes vs compiler variants (exact
  copies of ahmed_v25 / king_rc4), the RL-opportunity evidence.
- reports/panel/x1_gates: old gates (extra Python agents, C++ opponents, replays) with X1, to pair
  with reports/panel/v12_open6_vadim.
- Next: v16 + X1 on the broad panel; sale timing (graded receipts) and wages; expert iteration /
  RL on the network's whole-farm decisions (IDEAS_LEDGER "RL framing").
