# Gap 1 (budget-blind plans): ideas and smoke tests (Sep 25, one-hour exploration)

Goal: close gap 1 of `GAPS.md` (the network asks for more than the day can fund on investment days
0, 6, 7, 9; the compiler cuts by a fixed rule). Main validation is the main session's job; this
file lists ideas, smoke-test results and recommendations. Probes are env flags in `exp_patch/`
(defaults reproduce the baseline; flags apply only to our agent).

Main session state checked at 15:20 (experiments/v10/sep24_BC_opus/PROGRESS.md): network-ordered
trims (loss only) are the default; `DC10_BUDGET_REVISION` (covering knapsack over decoder losses)
exists; `DC10_LAND_FIRST`, `DC10_RESERVE_FARM` (reserve counts overnight fertilizer and held product),
`DC10_RECOVERY` (survival level keeps harvests) exist. Their finding: day 6 fails on hour-0 cash
timing (land + hires + wheat at hour 0 vs ~$1,071), which a dollar-shortfall knapsack does not fix.
Ideas below avoid duplicating that work.

Smoke test (`smoke.sh`): Vadim candidate + X10 flags + probe; Local-LB cha22 and yannik-latest,
zoo_dsm and v13_w384b; seeds 700-707, both seats (64 games); compared game by game with X10 on
the same games (`smoke_compare.py`). Smoke tests only rank ideas; they are not significance tests.

## Ideas

| # | Idea | Concept | Status |
|---|---|---|---|
| 1 | Deferral: animals trimmed today stay pending and are added to the next day's intent (days <= 12) | purchase obligations persist across days (Sep 21 design, "obligations are idempotent") | smoke: LB +854 (13 better/9 worse), zoo -33: neutral, changes few games |
| 2 | Coherent smaller plan: re-decode at crop-total quantiles 0.4/0.3/0.2 | the network's own distribution defines smaller plans | never fired on seed 700 (lower crop totals mostly drop cheap wheat; still over budget) |
| 3 | Feasible style: on a trimmed day, re-decode with Majkel, DSM, Vadim, Mother-Goose styles; take the first plan that compiles untrimmed | teacher styles are coherent plans with different budgets; Majkel's opening fits day 0 | smoke: LB -9.2k (4 better/28 worse), zoo -3.1k: negative (one seed-700 game had shown +9.4k). Switching style for one day breaks multi-day coherence |
| 4 | Reserve from certain overnight income | reserve is safety, not a plan cut | exists in the main session (`DC10_RESERVE_FARM`) |
| 5 | Covering knapsack with the dollar shortfall | constrained MAP decode | exists (`DC10_BUDGET_REVISION`); main session found the shortfall abstraction wrong on day 6 |
| 6 | Economic value table (value per dollar by entity and day) for trims | compiler-side value | rejected: measured value per dollar ranks animals lowest (sheep ~5.6, strawberry ~10, wheat ~16) while keeping animals wins; herd value compounds (wool funds day 6) and a static table cannot see it |
| 7 | Skip day-0 feeding of new animals to save ~$100 | cost reduction before entity cuts | rejected: loses the day-0 care bank (-1 unit per animal at first production, ~$570 for 3 sheep), feed counts are network decisions |
| 8 | Retime land later in the day | purchases at the latest phase before use | covered by the funding variants (F1-F6 already defer land); day 6 fails on the total bill |
| 9 | Majkel style on day 0 only (then the Vadim opening) | an opening that fits the budget, no earlier plan to conflict with | smoke: LB -15.3k (one seed collapses to -191k: day 7-8 cash crunch, fixed in the main session by DC10_RECOVERY; other games 16/16 even), zoo -1.5k: negative; re-test with recovery if CPU allows |

## Conclusions so far (15:36)

- The network's own preference, applied as a trim order (X10), is still the best budget mechanism
  found; replacing whole plans (other quantiles, other styles, another day-0 style) is worse:
  the plan of one day must stay coherent with the days around it.
- Deferring trimmed animals is neutral: the network's next plans already ask for what it wants.
- Collapses (cash crunch on days 7-8 into a survival level without income) make any tighter-cash
  probe risky; the recovery level must be part of every budget change (ported as OPUS_RECOVERY).
