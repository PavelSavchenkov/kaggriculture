# Handoff: sep24_BC_opus, Sep 25 evening (supersedes HANDOFF_sep25b.md where they differ)

Read PROGRESS.md from "Audit 15:25" on and IDEAS_LEDGER.md from "Sep 25 15:40" on.

## Standing rules

- Goal (24 h from Sep 25 12:15): strongest BC + day-compiler agent vs the Local-LB and its
  strongest agents; iterate on the highest-ROI improvement; audits every ~30 min.
- No PPO/RL training of our agents (user, Sep 25). "PPO" in notes = JJ's agent ppo-v6n-it60
  (Local-LB branch add-ppo-v6n-it60), an opponent only.
- CPU: i7-14700K (8P + 12E). Keep ~16 game threads, decision-critical jobs first, bulk jobs
  paused or niced (memory cpu-budget). RAM: one training process at a time.
- Another Claude session works in work/sep25_bc_weakness (read only for us).

## Local-LB state

- Merged: pavel-bc-opus-v12-slots is #1 (399-1-0, rating 2802), exactly as predicted locally.
- Pushed, PR up to the user: submit/pavel-bc-opus-v12-wages (predicted 386/400 on the df4172a set,
  28/40 vs slots). Worktree work/local_lb_prs/pavel-bc-opus-v12-wages.
- scripts/lb_seeds4.sh replays a challenger's exact Local-LB games for the df4172a active set
  (opponent i alphabetical from 1 plays seeds 1000*i..+19, both seats). Update the agent list when
  the active set changes. data/localLB_main holds the LB agents (slots, ppo-v6n-it60 added).

## Agent (models/cand_*; weights v12_cond + sidecars)

- model.bin.opening "6 7": DSM style days 0-5 (was Vadim "6 8"); 82-38 vs Vadim under the wages
  compiler; strength stays 150 (model.bin.condition "1 0.7500 1.0250").
- model.bin.compiler keys: sell_order 2 (sales by revenue at stake: engine fills orders slot by slot),
  return_wages 1 (same-day market returns only if the sale gain covers the extra Fibonacci wages),
  hire_cap_stock 1 (pre-solve hire cap counts hour-0 sellable shed stock; neutral except crunch days).
- Merged since: pavel-bc-opus-v12-wages (Local-LB PR #184; ranking rebuild pending at 17:30).
- Best broad candidate: models/fin_D = DSM opening + hire_cap_stock + herd reach (<model>.decode
  "reach_days 6 14", "reach 0.8 0.7 0.6": after the plan compiles in full, try larger new-animal
  quantiles and keep the first that also compiles in full). vs wages 126-74 over 5 seed sets, but
  18-22 on the Local-LB seeds of the wages pairing (7000-7019): not pushed. Packaged anyway as
  submissions/sep25-bc-opus-v12-reach.
- Pushed Sep 25 17:45: submit/pavel-bc-opus-v12-forecast = models/fin_I (DSM opening, herd reach 6-14,
  sell_order 2, return_wages, hire_cap_stock, collect_all, forecast_blend). vs wages 164-36 (5 seed
  sets); exact 10-agent roster replay 137/140; full roster (69 agents) 627/630 so far; replay gate
  143/145. Worktree work/local_lb_prs/pavel-bc-opus-v12-forecast.
- Local-LB rules changed (memory local-lb-rules-sep26): 7 seeds per pair, top-30 roster;
  scripts/lb_seeds_roster.sh replays a roster file.
- On top of fin_I (mirror, 40 games per set): racedp +0.3k (102-86-12, +50% compile), route_search 2
  +0.2k (44-24-12, +31% compile), blend weight 0.7 46-34, crop reach 60-60 (dropped), melon race
  41-39 (dropped), slack care 9-31, Vadim opening 13-27, route_search 1 73-47 but 3.4x compile.
  Bundle fin_P = fin_I + blend_visible 70 + race_dp + route_search 2 under test; forecast_stock
  (opponent inferred shed stock in the forecast, fin_Q) under test.
- Sidecars also available: opening_model, ensemble, decode (paths relative to the model folder).

## Rejected on top of slots/wages (mirror vs current best, 40+ games)

melon race, network-ordered trims, fertilizer value rule, full care, load leveling (deferring
harvests delays replanting: tiles bind, not wages), market deadline 22 (neutral), strength 100 with
DSM, other opening styles/lengths, zoo fine-tunes as opening models, v13/v14/v16/v18/v12_aug
networks, replay- or on-policy-trained value models for decision ranking.

## Evaluation tools

- C++ mirror: scripts/cand_mirror.sh <ref cand> <build> <threads> <seed_start> <cand...> (search_games
  without search, both agents with their own sidecars; BC rivals now use their model's opening).
- scripts/knob_mirror.sh (opening/condition variants), scripts/net_mirror.sh (networks).
- Python harness: scripts/lb_play.py sets our BC_OPUS_MODEL only around our opus_new (our older LB
  agents set it at import; before this fix every sidecar h2h vs them was invalid). LB_ACTIONS=1 logs
  hourly market orders; scripts/ppo_ledger.py and sale_contest.py analyse them.
- Selection seeds 1300-1319; confirm on 1400/1500/1600; exact Local-LB replay last.
- Network strength does not follow validation loss (v12 recipe seeds: 8, 19, 5, 20 wins of 40 vs v12);
  select networks by games.
