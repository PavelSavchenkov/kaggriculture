# Review 28 — 2026-09-07 10:24 UTC

Original attachment reread10:18. The user authorized exactly one official Kaggle
submission, retesting the frozen C++ version against teammate, and parallel main
goal continuation. Submission agent official_submission handles only that task.
The one upload is ID56074695, uploaded10:21:58UTC, platform status PENDING.
Frozen C++ won3980/4096teammate games (97.17%), mean margin$9426.5, lower-tail
margin$377.4, seeds1150000..1152047both. Packed official runtime62/64wins,
all64final cash pairs equal C++,48892actions matched including PASS/self.
File-path loader self-play passed. Platform validation/replay check still pending.
Do not submit another version without a new user request.

## Progress and priority

The user's milk/wool shop intuition works on fresh complete games. Primary
shop_herd_s6_m3_g1 wins599/1024against previousbest,+$1581.7margin. On common
fresh1100000seeds, teammate wins961→972, public803→891, Binghua710→884,
John720→864, King713→725; v4unchanged924. Terminalparent962→951is a small
regression, and some lower tails worsen. Native256:150previousbest,185King,
222public. PASSJ150732. All required local deployment checks pass.

Paired64profiles: in each of14changed games, the day7branch replaces two cows
with sheep, producing54less milk and48more wool; other output stays equal.
Changed-game owncash rises$5422 despite$1432extra labor,17hires and18faults
from invalidated old day plans. This is realized composition adaptation.
Fifteen new V30days for the changed herd now pass exact endpoint and both-cash
checks. All15are accepted by six-opponent sequential discovery, +$247.9mean
margin across32games peropponent. New shop_herd_guarded_001_best is unpromoted;
generic/debug/broad/fresh checks are next. Rebuild001stopped because seed1000
did not activate sheep. Rebuild002uses discovery seed1005, selected from branch
telemetry. Day7is excluded: physical state alone cannot guard its shop decision.

The separate whole-herd heuristic projection also produced positive discovery
choices: day7single changes +$973..1062average margin, day11goosechoice+$815..841.
Earlier owncash-only choices hurt margin despite increasing owncash. The model's
pre-structure decision misses later observed shops for compatible cow/sheep
choices; those should be delayed to purchase. Models remain unpromoted and are
not components of the submitted direct rule.

## Latest independent global invariant comparison

18actual player-games,16uniqueepisodes, selected by global rank before local
testing: ymg#1,Justin#8,Bohann#12from10:12:05leaderboard. All cash reconstruction
checks pass:15877transactions,4256crop instances,287animal instances. Compare
with64local profiles of current adaptive candidate vs public_router. Service
rates are averages of per-game rates. No local executability filter was used.

| Metric | Global cohort | Current adaptive agent |
|---|---:|---:|
| animal_days_COW | 180.167 | 198.938 |
| COW_d0 | 2.333 | 2.000 |
| COW_d2 | 2.833 | 3.000 |
| COW_d5 | 3.389 | 4.000 |
| COW_d8 | 6.278 | 7.562 |
| COW_d11 | 6.778 | 7.562 |
| COW_d17 | 7.000 | 7.562 |
| COW_d23 | 7.000 | 7.562 |
| COW_d29 | 7.000 | 7.562 |
| fed_COW | 0.808 | 0.886 |
| cared_COW | 0.860 | 0.952 |
| collected_fertilizer_COW | 0.934 | 0.952 |
| animal_days_SHEEP | 169.056 | 155.062 |
| SHEEP_d0 | 2.000 | 2.000 |
| SHEEP_d2 | 2.000 | 2.000 |
| SHEEP_d5 | 2.000 | 2.000 |
| SHEEP_d8 | 4.167 | 4.438 |
| SHEEP_d11 | 6.944 | 6.438 |
| SHEEP_d17 | 7.111 | 6.438 |
| SHEEP_d23 | 7.333 | 6.438 |
| SHEEP_d29 | 5.944 | 6.438 |
| fed_SHEEP | 0.872 | 0.934 |
| cared_SHEEP | 0.881 | 0.965 |
| collected_fertilizer_SHEEP | 0.891 | 0.893 |
| animal_days_GOOSE | 27.778 | 59.000 |
| GOOSE_d0 | 0.000 | 0.000 |
| GOOSE_d2 | 0.000 | 0.000 |
| GOOSE_d5 | 0.000 | 0.000 |
| GOOSE_d8 | 0.111 | 0.000 |
| GOOSE_d11 | 1.111 | 3.000 |
| GOOSE_d17 | 1.389 | 3.000 |
| GOOSE_d23 | 1.500 | 3.000 |
| GOOSE_d29 | 1.500 | 3.000 |
| fed_GOOSE | 0.906 | 0.898 |
| cared_GOOSE | 0.925 | 0.966 |
| collected_fertilizer_GOOSE | 0.858 | 0.797 |
| crop_days | 1516.778 | 1507.438 |
| crop_water_rate | 0.745 | 0.725 |
| crop_yield_day_maximized | 0.252 | 0.197 |
| output_WHEAT | 537.222 | 515.797 |
| output_CARROT | 83.778 | 81.953 |
| output_TOMATO | 8.611 | 0.000 |
| output_STRAWBERRY | 236.889 | 248.922 |
| output_MELON | 76.667 | 72.000 |
| buy_WHEAT | 341.389 | 152.969 |
| sell_WHEAT | 540.944 | 294.750 |
| net_WHEAT | 199.556 | 141.781 |
| buy_CARROT | 0.000 | 0.000 |
| sell_CARROT | 82.722 | 81.953 |
| net_CARROT | 82.722 | 81.953 |
| buy_TOMATO | 0.000 | 0.000 |
| sell_TOMATO | 8.611 | 0.000 |
| net_TOMATO | 8.611 | 0.000 |
| buy_STRAWBERRY | 0.000 | 0.000 |
| sell_STRAWBERRY | 232.833 | 248.922 |
| net_STRAWBERRY | 232.833 | 248.922 |
| buy_MELON | 0.000 | 0.000 |
| sell_MELON | 76.667 | 72.000 |
| net_MELON | 76.667 | 72.000 |
| buy_EGG | 0.000 | 0.000 |
| sell_EGG | 38.167 | 70.000 |
| net_EGG | 38.167 | 70.000 |
| buy_MILK | 0.000 | 0.000 |
| sell_MILK | 197.833 | 230.188 |
| net_MILK | 197.833 | 230.188 |
| buy_WOOL | 0.000 | 0.000 |
| sell_WOOL | 176.833 | 171.438 |
| net_WOOL | 176.833 | 171.438 |
| buy_FERTILIZER | 25.556 | 50.000 |
| sell_FERTILIZER | 280.944 | 356.281 |
| net_FERTILIZER | 255.389 | 306.281 |
| hires | 279.500 | 261.188 |
| hire_cost | 5648.611 | 3972.219 |
| land_buys | 2.000 | 2.000 |
| land_cost | 3000.000 | 3000.000 |
| weed_digs | 17.167 | 20.016 |
| discards | 25.389 | 4.766 |
| faults | 65.778 | 14.922 |
| SELL_mean_hour | 8.979 | 10.550 |
| BUY_PRODUCT_mean_hour | 10.121 | 6.251 |

The new global cohort has fewer cow/goose-days and more sheep-days than ours.
This supports wider demand-conditioned herd choices, not one universal target.
Crop occupancy is similar; wheat/strawberry output is close. Tomato output is
low in this cohort after being high previously, so retain tomato-family search
without treating zero tomatoes as universal error. Productive fertilization is
still lower locally; compile its inputs, routes, timing and shed consequences.
Global gross wheat flows are much higher; compare net sales and crop output to
avoid confusing turnover with production. Our discards and labor costs are
lower; high global discard means are not targets. Daily care and95%service are
not established universal rules; profitable service exceptions remain allowed.

## Faithful goal coverage and next work

Keep dated compositions central, including maintained crop occupancy and repeated
rotations; cheap endogenous economics and feasibility; concrete placement,
service, workforce and intraday markets; estimate/realization feedback. The
successful simple branch remains narrow: buy-nothing, arbitrary herd insertion
or deletion, large suffix rebuilds and rich rival responses are still gaps.
Preserve cold starts and large family moves while improving the adaptive source.
The day solver is useful but does not substitute for strategic adaptation.

Three updated notebooks were pulled statically. TTV1andMarket-Impact-v4share
Thomas's course family; TITANcontains Kaito43plus observed-farm-similarity sales.
Exact extracted sources and sanitized inspection copies are retained. Public
performance claims are not local evidence. Library remains180courses; new18
are not integrated yet. No GPU workload is justified; unrelated training is
untouched. No Git. Next review10:36. Goal continues until2026-09-08 00:47UTC.
Fresh1100000used;1150000used forsubmission; nextmainfresh1200000+; final900000unused.
