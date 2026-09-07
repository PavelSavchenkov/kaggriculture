# Review 33 — 2026-09-07 12:19 UTC

Original user attachment reread at12:18. Goal remains active to2026-09-08 00:47.
Current strongest validated local reference remains
candidates/investment_context_guarded_001_best; no further official submission.

## New evidence and priorities

General animal entry compilation now accepts an original species, purchase
address, reserved tile, and alternative entry dates. This covers an earlier
cow opportunity as well as the prior day11 goose opportunity. It exposes two
compiler gaps that were fixed and recorded separately:

- An animal purchase at hour0 could be unfunded although the solver found a
  route. New insertion checks use exact accepted-market prefixes and preserve
  existing market obligations before choosing a purchase hour.
- Forcing feed and care on placement day changed the original cow's biology
  and disabled later schedules. The general compiler now follows the source
  service calendar, including no feed/care on this placement day. This is a
  concrete example of the user's productive-service default with exceptions.

The corrected bank has48 checked entry days: goose/cow/sheep × days3/6/9/15 ×
four source contexts (two seeds against public_router and King). The original
cow control enters384/384 and exactly reproduces both cash outcomes across all
six opponents. Its public64 also preserves production, sold quantities,
discards, fault counts and rival actions; own routes differ. Full fixed and
adaptive alternatives were tested, but none improves the incumbent. Best
adaptive loses about$3,383 mean margin across the six-opponent discovery panel.
Earlier negative forced-service results are not treated as economic evidence.
Current valid negative result still needs its labor/production/price errors
separated. Do not infer that early adaptivity is useless or that the compiler
already covers every feasible composition.

Atakan's three attributed complete suffixes provide independent positive
evidence for general shop selection. Demand routing wins221/320 versus163/320
for the best fixed course, improving mean margin by$4,702. Margin valuation
wins211/320. Against the current reference these win23/64 and25/64, so retain
as league specialists, not a new incumbent. Source6471actions, PASS256,
self8, generic/debug/thread parity and1280 selected-continuation outcome
matches pass. Exact product accounting reproduces960 fixed-course games.
Frozen evidence: runs/atakan_portfolio_001/FINAL_VALIDATION.json and REPORT.json.

The large estimator error is now measured rather than guessed. Across90
model/demand disagreements, predicted own advantage is$2,434 too high and
rival advantage$4,206 too low on average. In seed1028 against public v5, rival
milk revenue rises$15,483 between cow/goose suffixes; model predicts$2,949.
Milk volume is close (258 predicted,249 sold), no initial milk inventory and
no later cow placements. This points to price/timing and shop uncertainty,
not missing cow output in that case. A parallel OFFLINE oracle ablation will
separate future shops, own flows, rival flows and daily price approximation.
Oracle information will never enter a deployable policy.

Next model experiment: value possible future shop sequences explicitly with a
small deterministic sample, instead of applying the nonlinear price curve to
one discounted mean-demand trajectory. Compare decision quality and CPU time
against the existing approximation. Keep any extra detail only if exact-game
results justify it. Conditional whole-farm labor and fertilizer conversion
remain measured gaps; rebuilding source-preserving worker schedules remains
necessary after promising species changes.

## Latest independent global comparison

12:12 leaderboard: ymg#1, Mengfei#2, Jun#3, Atakan#12. Selected ranks1/3/12
before local execution:18player-games,17unique episodes. All replays analyzed:
12,535transactions,4,282crop instances,271animal instances, cash reconciled.
No newer public notebook than the already converted Thomas v5 controller.
Below compares this cohort with64 current-reference games against public_router;
these are different opponent/shop distributions, so global gaps are approximate
targets rather than causal strength claims. Ratios and trade hours are means
of per-game ratios, with matching definitions.

| Metric | Global cohort | Current local agent |
|---|---:|---:|
| animal_days_COW | 198.056 | 198.938 |
| COW_d0 | 2.333 | 2.000 |
| COW_d2 | 3.000 | 3.000 |
| COW_d5 | 3.667 | 4.000 |
| COW_d8 | 7.000 | 7.562 |
| COW_d11 | 7.556 | 7.562 |
| COW_d17 | 7.944 | 7.562 |
| COW_d23 | 7.778 | 7.562 |
| COW_d29 | 6.889 | 7.562 |
| fed_COW | 0.765 | 0.886 |
| cared_COW | 0.765 | 0.952 |
| collected_fertilizer_COW | 0.936 | 0.952 |
| animal_days_SHEEP | 133.056 | 159.812 |
| SHEEP_d0 | 2.000 | 2.000 |
| SHEEP_d2 | 2.000 | 2.000 |
| SHEEP_d5 | 2.000 | 2.000 |
| SHEEP_d8 | 3.944 | 4.438 |
| SHEEP_d11 | 5.111 | 6.688 |
| SHEEP_d17 | 5.611 | 6.688 |
| SHEEP_d23 | 5.611 | 6.688 |
| SHEEP_d29 | 3.778 | 6.688 |
| fed_SHEEP | 0.799 | 0.934 |
| cared_SHEEP | 0.746 | 0.965 |
| collected_fertilizer_SHEEP | 0.928 | 0.889 |
| animal_days_GOOSE | 29.167 | 54.250 |
| GOOSE_d0 | 0.000 | 0.000 |
| GOOSE_d2 | 0.000 | 0.000 |
| GOOSE_d5 | 0.000 | 0.000 |
| GOOSE_d8 | 0.444 | 0.000 |
| GOOSE_d11 | 1.056 | 2.750 |
| GOOSE_d17 | 1.444 | 2.750 |
| GOOSE_d23 | 1.444 | 2.750 |
| GOOSE_d29 | 1.444 | 2.750 |
| fed_GOOSE | 0.912 | 0.899 |
| cared_GOOSE | 0.855 | 0.968 |
| collected_fertilizer_GOOSE | 0.899 | 0.804 |
| crop_days | 1564.167 | 1507.438 |
| crop_water_rate | 0.719 | 0.725 |
| crop_yield_day_maximized | 0.397 | 0.197 |
| harvest_WHEAT | 497.167 | 515.797 |
| harvest_CARROT | 154.889 | 81.953 |
| harvest_TOMATO | 38.889 | 0.000 |
| harvest_STRAWBERRY | 233.167 | 248.922 |
| harvest_MELON | 86.278 | 72.000 |
| buy_WHEAT | 351.667 | 153.000 |
| sell_WHEAT | 553.611 | 294.750 |
| net_WHEAT | 201.944 | 141.750 |
| buy_CARROT | 0.000 | 0.000 |
| sell_CARROT | 152.667 | 81.953 |
| net_CARROT | 152.667 | 81.953 |
| buy_TOMATO | 0.000 | 0.000 |
| sell_TOMATO | 38.556 | 0.000 |
| net_TOMATO | 38.556 | 0.000 |
| buy_STRAWBERRY | 0.000 | 0.000 |
| sell_STRAWBERRY | 231.667 | 248.922 |
| net_STRAWBERRY | 231.667 | 248.922 |
| buy_MELON | 0.000 | 0.000 |
| sell_MELON | 86.167 | 72.000 |
| net_MELON | 86.167 | 72.000 |
| buy_EGG | 0.000 | 0.000 |
| sell_EGG | 48.167 | 65.000 |
| net_EGG | 48.167 | 65.000 |
| buy_MILK | 0.000 | 0.000 |
| sell_MILK | 206.333 | 230.188 |
| net_MILK | 206.333 | 230.188 |
| buy_WOOL | 0.000 | 0.000 |
| sell_WOOL | 127.056 | 176.688 |
| net_WOOL | 127.056 | 176.688 |
| buy_FERTILIZER | 21.722 | 50.000 |
| sell_FERTILIZER | 236.833 | 356.281 |
| net_FERTILIZER | 215.111 | 306.281 |
| hires | 271.667 | 258.156 |
| hire_cost | 5198.333 | 3705.281 |
| land_buys | 2.000 | 2.000 |
| land_cost | 3000.000 | 3000.000 |
| weed_digs | 16.611 | 20.016 |
| unit_faults | 15.722 | 12.078 |
| discards | 11.111 | 4.797 |
| SELL_mean_hour | 8.147 | 10.429 |
| BUY_PRODUCT_mean_hour | 7.905 | 6.251 |
| cash | 100785.500 | 92535.344 |
| margin | 3071.833 | 6344.156 |

Crop productive fertilization is0.197local versus0.397in this cohort, below
both this cohort and the preceding0.623cohort. The direction persists; the
magnitude and best crop mix vary. This cohort uses more wheat, carrots and
strawberries but fewer tomatoes than the previous group. Borrow complete
rotations and input/transport/sale dependencies; do not copy an aggregate ratio
as a universal rule. Animal service remains higher locally, especially late
cow/sheep care; service omission remains a priced local choice. Local labor,
faults and discards are lower, so additional production can justify some added
work if exact economics improve.

Preserve the original broader scope: dated farm occupancy, repeated crops,
cold construction, family insertion/deletion, large rebuilds, placement and
intraday trades remain in the search agenda. The latest API expansion is a
building block, not a completed general composition optimizer. No GPU job is
needed. Next review12:39; next metadata refresh about13:13. Next promotion seeds
1350000+, final900000 still unused.
