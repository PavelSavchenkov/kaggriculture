# Review 32 — 2026-09-07 11:56 UTC

Original objective attachment reread at 11:55. Active work window still ends
2026-09-08 00:47 UTC. The goal covers dated crops and animals, fast whole-farm
value, practical service and placement, exact compilation, market timing,
feedback from forecast errors, league improvement, and continued replay learning.
The latest user clarification applies to general animal choice, including waiting.

## Progress and decisions

Promoted investment_context_guarded_001_best after complete paired checks.
It adds 23 V30 day plans for late sheep after six or eight earlier cows to r1.
Fresh 1300000..1300511, both seats: 1,008/1,024 teammate wins, 767 King, 798
new public v5. All 14 opponent mean margins and utilities improve or tie;
no individual same-opponent margin worsens. Existing equal-group utility rises
87.40% to 88.44%. Native audits improve. PASS J 151,983 to 152,324. Generic,
debug and 16-thread complete records match in 64 games; full PASS and self-play
pass. The multi-context day selector preserves 2,304 previous complete records.
Evidence: results/investment_context_validation.json.

Causal public audit: all 64 paired games preserve biology, services, production,
buy/sell quantities and hours, discards and rival actions/cash. Sixteen changed
games save 11–12 hires, $1,076–$1,165 and 11–14 faults. This is the execution
closure of the already useful general animal decision, not new production.
The preceding animal choice changed 20 eggs into 21 wool in selected games;
its economic benefit included reducing rival revenue through shared prices.

Parallel Atakan portfolio reuses three attributed top-player continuations
with an identical prefix through step 225. Preliminary discovery: demand
routing 221/320 wins versus 163/320 for the strongest fixed goose continuation;
mean margins improve against all five tested opponents. Margin valuation
211/320 is a useful different specialist but loses to the simple demand rule
in aggregate. It is not yet an incumbent replacement. Full operational,
continuation-identity and estimator-error checks are in progress.

Main next experiment expands the entry compiler from one original goose
purchase to a parameterized animal purchase and tile, starting with the earlier
day-3 cow opportunity. It includes all three replacement species and later
entry dates. Exact whole-day rebuilds preserve the rest of the farm; later
dependency failures must be diagnosed rather than treated as bad economics.
Current promoted code was frozen by binary and component hashes before this
API extension; default behavior regression is being checked again.

## Independent global invariant comparison

Global selection was made from the normal-game 11:13 leaderboard before local
testing: Mengfei #1, 自己找差距 #8, CropDusta #12; 18 player-games and 17 unique
episodes. All were analyzed, including cash reconciliation. Local values below
are 64 current-reference games against public_router. Sources differ in game
and opponent distributions, so differences are approximate targets, not causal
strength comparisons. Service ratios and trade hours are averaged per game.

| Metric | Global cohort | Current local agent |
|---|---:|---:|
| animal_days_COW | 189.722 | 198.938 |
| COW_d0 | 2.333 | 2.000 |
| COW_d2 | 2.944 | 3.000 |
| COW_d5 | 3.611 | 4.000 |
| COW_d8 | 7.778 | 7.562 |
| COW_d11 | 8.222 | 7.562 |
| COW_d17 | 8.056 | 7.562 |
| COW_d23 | 6.611 | 7.562 |
| COW_d29 | 4.667 | 7.562 |
| fed_COW | 0.752 | 0.886 |
| cared_COW | 0.749 | 0.952 |
| collected_fertilizer_COW | 0.948 | 0.952 |
| animal_days_SHEEP | 147.167 | 159.812 |
| SHEEP_d0 | 2.000 | 2.000 |
| SHEEP_d2 | 2.000 | 2.000 |
| SHEEP_d5 | 2.000 | 2.000 |
| SHEEP_d8 | 4.222 | 4.438 |
| SHEEP_d11 | 5.944 | 6.688 |
| SHEEP_d17 | 6.611 | 6.688 |
| SHEEP_d23 | 6.722 | 6.688 |
| SHEEP_d29 | 3.389 | 6.688 |
| fed_SHEEP | 0.787 | 0.934 |
| cared_SHEEP | 0.702 | 0.965 |
| collected_fertilizer_SHEEP | 0.928 | 0.889 |
| animal_days_GOOSE | 32.556 | 54.250 |
| GOOSE_d0 | 0.000 | 0.000 |
| GOOSE_d2 | 0.000 | 0.000 |
| GOOSE_d5 | 0.000 | 0.000 |
| GOOSE_d8 | 0.167 | 0.000 |
| GOOSE_d11 | 1.500 | 2.750 |
| GOOSE_d17 | 1.611 | 2.750 |
| GOOSE_d23 | 1.611 | 2.750 |
| GOOSE_d29 | 1.611 | 2.750 |
| fed_GOOSE | 0.911 | 0.899 |
| cared_GOOSE | 0.851 | 0.968 |
| collected_fertilizer_GOOSE | 0.931 | 0.804 |
| crop_days | 1472.056 | 1507.438 |
| crop_water_rate | 0.691 | 0.725 |
| crop_yield_day_maximized | 0.623 | 0.197 |
| harvest_WHEAT | 467.389 | 515.797 |
| harvest_CARROT | 115.667 | 81.953 |
| harvest_TOMATO | 83.056 | 0.000 |
| harvest_STRAWBERRY | 213.444 | 248.922 |
| harvest_MELON | 74.889 | 72.000 |
| buy_WHEAT | 225.278 | 153.000 |
| sell_WHEAT | 392.444 | 294.750 |
| net_WHEAT | 167.167 | 141.750 |
| buy_CARROT | 0.000 | 0.000 |
| sell_CARROT | 114.556 | 81.953 |
| net_CARROT | 114.556 | 81.953 |
| buy_TOMATO | 0.000 | 0.000 |
| sell_TOMATO | 82.000 | 0.000 |
| net_TOMATO | 82.000 | 0.000 |
| buy_STRAWBERRY | 0.000 | 0.000 |
| sell_STRAWBERRY | 212.222 | 248.922 |
| net_STRAWBERRY | 212.222 | 248.922 |
| buy_MELON | 0.000 | 0.000 |
| sell_MELON | 74.889 | 72.000 |
| net_MELON | 74.889 | 72.000 |
| buy_EGG | 0.000 | 0.000 |
| sell_EGG | 53.111 | 65.000 |
| net_EGG | 53.111 | 65.000 |
| buy_MILK | 0.000 | 0.000 |
| sell_MILK | 187.611 | 230.188 |
| net_MILK | 187.611 | 230.188 |
| buy_WOOL | 0.000 | 0.000 |
| sell_WOOL | 137.833 | 176.688 |
| net_WOOL | 137.833 | 176.688 |
| buy_FERTILIZER | 26.222 | 50.000 |
| sell_FERTILIZER | 210.778 | 356.281 |
| net_FERTILIZER | 184.556 | 306.281 |
| hires | 272.500 | 258.156 |
| hire_cost | 5115.333 | 3705.281 |
| land_buys | 2.000 | 2.000 |
| land_cost | 3000.000 | 3000.000 |
| weed_digs | 30.444 | 20.016 |
| unit_faults | 20.722 | 12.078 |
| discards | 11.111 | 4.797 |
| SELL_mean_hour | 5.664 | 10.429 |
| BUY_PRODUCT_mean_hour | 5.562 | 6.251 |
| cash | 92835.778 | 92535.344 |
| margin | 4311.222 | 6344.156 |

The persistent production gap is productive crop fertilization: 0.197 local
versus 0.623 global, despite comparable crop occupancy and plentiful animal
fertilizer. Species selection and labor rebuilding alone cannot close it.
Keep the crop/fertilizer conversion branch and complete dependencies in the
search queue: acquisition, collection, transport, application dates, harvest,
capacity and sales. Existing repeated-crop/cold-family machinery remains
available; do not let single-purchase improvements define the whole search.

Our animal feed/care rates exceed the cohort, especially late cow/sheep
service. Treat productive service as a strong default with local economic
exceptions, not a universal full-service requirement. Prior terminal rules
already omit some care. Further pruning must preserve or explicitly value
output, fertilizer and escaped animals; global ratios alone are not instructions.

Current whole-farm animal estimator costs about 2.05 microseconds per choice,
with margin ranking correlation about 0.60–0.64 in the recorded development
pool. The main identified gaps are fertilizer-to-crop conversion, conditional
labor, rival expansion/crop trades, and intraday funding/storage. Complete
Atakan flow models provide an independent diagnostic of these assumptions.
No GPU workload is justified; the user's other training job remains untouched.

One authorized Kaggle submission is already COMPLETE (56074695); no further
submission. Fresh promotion pool next 1350000+; final 900000 remains unused.
Next review 12:16; next fresh leaderboard/notebook metadata about 12:13.
