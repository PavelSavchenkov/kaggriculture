# Offline Atakan estimator ablation

Future-shop treatment is the largest gap on this three-course panel. Supplying future shop identities reduces mean branch-choice regret from $3,249 to $475. Supplying all future own/rival trade quantities as well reduces it to $15.44. These are offline oracle diagnostics, not achievable agent improvements: no policy receives those future inputs, and no candidate or submission is created here.

The panel is the existing 320 discovery contexts: seeds 1000–1031 in both seats against five opponents, with three fixed Atakan continuations each. The unchanged cow prefix is replayed in C++ through step 225 against each opponent using frozen dependencies. All 320 decision states and all 960 baseline forecasts exactly match the previous experiment. Python only extracts saved data and computes statistics. The previous `atakan_portfolio_001` policy and report remain frozen.

| Forecast inputs replaced | Correct pairs / 960 | Best branch / 320 | Mean regret | Pair-margin error, MAE |
| --- | ---: | ---: | ---: | ---: |
| None: original baseline | 736 | 202 | $3,249.14 | $8,546 |
| Rival future daily trades | 745 | 215 | $2,859.97 | $7,628 |
| Own future daily trades | 752 | 200 | $3,299.66 | $8,491 |
| Both future trade plans | 752 | 198 | $2,989.81 | $6,905 |
| Future shops | 806 | 288 | $475.16 | $5,946 |
| Rival trades + future shops | 856 | 300 | $225.47 | $3,130 |
| Own trades + future shops | 848 | 282 | $712.08 | $5,583 |
| All three inputs | 907 | 313 | $15.44 | $1,533 |
| All three + rival fixed costs | 909 | 313 | $15.44 | $1,509 |
| Above + exact consumption calendar | 912 | 314 | $11.46 | $1,492 |

Regret is actual final margin of the best of the three courses minus actual margin of the selected course, averaged over contexts. Correct pairs count all three pair comparisons per context. These measure different properties: some remaining ranking errors exchange two inferior branches. After all oracle inputs, only six top-branch mistakes remain, costing $194–$1,238 each. This supports the adequacy of daily valuation for choosing among these three courses once inputs are accurate; it does not validate daily valuation for every strategy family.

Own-flow accuracy alone does not improve selection here. Its value depends on the other forecasts: after rival flows and shops are exact, adding exact own flows lowers regret from $225.47 to $15.44. Do not interpret the negative isolated result as evidence that own realization is unimportant. Averaging marginal contributions across the eight input combinations attributes $2,786 of regret reduction to shops, $455 to rival flows and −$8 to own flows; those interaction-sensitive averages describe this fixed panel only.

The original baseline discounts expected unknown-shop demand to 35% of its mean and evaluates that one mean-flow path through a nonlinear price curve. The shop oracle changes both that approximation and the unavailable future identities. Its gain therefore does not distinguish correctable expectation bias from irreducible future uncertainty. The useful next deployable test is integration over legal future-shop scenarios with common samples across branches, followed by exact gameplay validation.

Seed 1028, seat 0 against public router V5, makes the mechanism concrete. At the decision, Ice Cream, Smoothie and Brunch are revealed. Later shops are Brunch, Ice Cream, Smoothie, Bakery and Yarn; the day-15 and day-18 shops double milk-consuming shops from two to four. Rival milk sales are unchanged between our cow and goose continuations.

| Diagnostic | Predicted extra rival milk revenue, goose versus cow | Predicted margin difference | Choice |
| --- | ---: | ---: | --- |
| Baseline | $2,949 | +$3,558 | Goose |
| Rival trades only | $4,706 | −$1,942 | Cow |
| Own trades only | $2,949 | +$2,814 | Goose |
| Future shops only | $13,914 | −$5,137 | Cow |
| All three | $13,391 | −$12,852 | Cow |
| All three + costs/calendar | $13,649 | −$12,292 | Cow |
| Exact realized result | $15,483 | −$13,695 | Cow |

Future shops explain most of the milk-price miss. Exact rival sales timing improves the baseline enough to correct this one choice, but leaves substantial price error without future demand. The residual after all quantities, costs and consumption are known comes from the retained daily midpoint price, intraday timing, internal trade netting and price-floor stock approximation. Their individual effects are not separated here.

Rival-flow oracles replace future daily sales and product purchases, including their purchase cost. Own fixed costs already match the donor exactly in all 960 continuations. Rival fixed costs remain omitted in the eight primary combinations and are supplied only in the last two diagnostics. The final calendar diagnostic removes the baseline's fictitious town-center fertilizer consumption and uses exact event counts for the partial current day. It improves regret by only $3.98 on this panel. Neither change touches the frozen policy.

`REPORT.json` contains complete metrics, per-opponent results and product revenue errors. `cases.json` retains each branch choice and regret. `seed1028_v5.json` also retains the per-day predicted milk quotes. `INPUT_LINEAGE.json`, `build/BUILD.json` and `FINAL_VALIDATION.json` record input, source, binary and output hashes and commands. `LEARNINGS.md` records the conclusions and remaining limits.
