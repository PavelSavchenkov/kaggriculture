# Historical rival cash: broader confirmation

Do not promote this change to the plan chooser. It removes forecast failures, but the fresh comparison does not establish a gain over the previous selector or the demand chooser.

The frozen test uses 128 new seeds, 12 opponent packages, both seats, independent shops and the same three supplied courses: 3,072 choices and 9,216 fixed branches per variant. The sole change is whether a borrowed rival calendar keeps its historical starting cash or receives the observed current rival's cash. Actual played opponents receive no extra cash.

| Comparator | Mean margin change | 95% seed interval | Win utility change | Gains / losses / unchanged |
| --- | ---: | ---: | ---: | ---: |
| Previous cash-splice selector | +$87.47 | -$245.16 to +$426.84 | +0.59 percentage points | 207 / 218 / 2,647 |
| Demand chooser | +$411.71 | -$233.40 to +$1,112.07 | +0.65 percentage points | 392 / 342 / 2,338 |
| Older model chooser | +$838.41 | +$169.27 to +$1,530.06 | +3.03 percentage points | 673 / 423 / 1,976 |

Win-utility intervals include zero for all three comparisons. All intervals resample whole seeds across opponents and seats. These are related opponent packages and one family of three courses, not independent-family confirmation.

Against the previous selector, own cash falls $624.53 on average and rival cash falls $712.00. The worst margin change is -$18,251, both seats against titan_frontier on seed 2026091070127: the choice switches from branch 1 to branch 0. There are 28 cases with more own work faults. Five opponent means are negative: parent, public_router_v52, atakan_demand, yusuke_sep08_m2 and ahmed_v24. Worst-decile margin improves by $309.54; that does not override the other failed gates.

## What changed

The old evaluator has 5,490 branches with rival financial failures and 1,830 decisions with no eligible branch. Historical cash removes all of those failures. All 9,216 actual fixed-branch cash, production, worker-day and fault outcomes match between variants.

Numeric branch rankings change in zero of 3,072 decisions. Every one of the 425 changed choices previously used the no-eligible-branch fallback. The change therefore alters eligibility and fallback use; it does not improve relative price predictions. Do not bypass feasibility checks on the strength of this observation.

Mean compilation plus eight-world valuation takes 4.29 ms per course with historical cash and 4.39 ms with current cash. Scenario loading and actual complete-game evaluation are outside those times.

## Meaning of the scenarios

Keeping a historical account together makes a different stress case. It does not make that farm a continuation of the currently observed rival. The separate public-state audit in `../scenario_public_compatibility_v0` finds no source matching all five checked features for 908 of 1,280 used-cohort decisions. Even a match is only a necessary condition: ages, services and private inputs also matter.

Retain both evaluator modes as explicit diagnostics. The existing selector remains unchanged by default. Better conditioned scenarios and positive complete plan-choice results are still required for pipeline promotion.

Evidence: `PROTOCOL.json`, `RANK_ATTRIBUTION.json`, `SELECTION_AUDIT.json`, `SELECTED_LOSSES_VS_CHOSEN.json`, and `historical_cash/PAIRED_EFFECTS.json`. Reproduce summaries with the experiment's `scripts/summarize_portfolio.py`, `scripts/compare_portfolio_variants.py --reference current_cash`, and `scripts/audit_portfolio_choices.py`, through the kaggriculture conda environment. Exact arena commands are in each variant's `*_COMMAND.json` files.
