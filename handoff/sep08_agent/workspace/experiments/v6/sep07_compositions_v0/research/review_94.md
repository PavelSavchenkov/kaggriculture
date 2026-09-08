# Review 94 — 2026-09-08T09:30:19.984758+00:00

The previous turn completed the requested lineage summary; it added no new
optimization evidence. This continuation has now prepared the dated group
compiler and estimator in runs/animal_groups_sep08_001. No blocker. The global
reference remains empty_sale_slots_m2; cold reference early_melon_b98_m1.

The earlier single-animal compiler used opening_q32_b13_v1, so extending it
unchanged would discard later sheep-family, opponent, sale and slot improvements.
The new extractor uses the actual strongest agent with its normal 100000 node
budget. It requires equal full action hashes, rewards, production, sales and
discards against an independent normal-engine control, and exact crop biology.

The compiler now accepts multiple dated animal additions, retaining original
tile work before each birth. The estimator enumerates one, two and three animals
on released crop tiles, with observed-shop-conditioned 32-scenario shared-price
valuation. Initial labor is explicitly unpriced and recorded visit deficits are
not infeasibility proofs. The recorded source future is retrospective, so this
is a fixed-calendar screening experiment, not a deployable selector. Removed
old unguarded cross-scenario policy scoring. Full source controls and matched
solver/full-game outcomes are the next evidence required.

All82 comparisons below explicitly reuse review93's verified09:08 global72
player-games/58replays and unchanged local256 native games. No new population
comparison or matched causal inference is claimed. Nextreview09:48; hourly
replay/notebook refresh10:08. Goal active throughSep10 00:47; final900000 unused.
No Git, upload, official catalog change, subagent or GPU workload change.

| Metric | Global72 (09:08) | Current native256 |
| --- | ---: | ---: |
| cash | 96855.4444 | 96856.4492 |
| margin | 1676.6667 | 240.9453 |
| discards | 6.0556 | 2.6055 |
| buy_WHEAT | 247.4167 | 169.8672 |
| sell_WHEAT | 459.6250 | 298.7969 |
| net_WHEAT | 212.2083 | 128.9297 |
| buy_CARROT | 0.0000 | 0.0000 |
| sell_CARROT | 121.9583 | 79.3047 |
| net_CARROT | 121.9583 | 79.3047 |
| buy_TOMATO | 0.0000 | 0.0000 |
| sell_TOMATO | 39.0278 | 4.2188 |
| net_TOMATO | 39.0278 | 4.2188 |
| buy_STRAWBERRY | 0.0000 | 0.0000 |
| sell_STRAWBERRY | 228.3194 | 248.9219 |
| net_STRAWBERRY | 228.3194 | 248.9219 |
| buy_MELON | 0.0000 | 0.0000 |
| sell_MELON | 78.1389 | 72.0000 |
| net_MELON | 78.1389 | 72.0000 |
| buy_EGG | 0.0000 | 0.0000 |
| sell_EGG | 70.1806 | 70.9609 |
| net_EGG | 70.1806 | 70.9609 |
| buy_MILK | 0.0000 | 0.0000 |
| sell_MILK | 178.1944 | 233.3164 |
| net_MILK | 178.1944 | 233.3164 |
| buy_WOOL | 0.0000 | 0.0000 |
| sell_WOOL | 151.8472 | 176.1328 |
| net_WOOL | 151.8472 | 176.1328 |
| buy_FERTILIZER | 57.4861 | 50.1250 |
| sell_FERTILIZER | 278.5833 | 361.7266 |
| net_FERTILIZER | 221.0972 | 311.6016 |
| animal_days_COW | 179.5972 | 201.8438 |
| fed_COW | 0.7627 | 0.8863 |
| cared_COW | 0.7691 | 0.9458 |
| collected_fertilizer_COW | 0.9435 | 0.9494 |
| COW_d0 | 2.4167 | 2.0000 |
| COW_d2 | 3.0556 | 2.9961 |
| COW_d5 | 3.9722 | 3.9844 |
| COW_d8 | 6.2500 | 7.6094 |
| COW_d11 | 6.8611 | 7.6836 |
| COW_d17 | 7.0278 | 7.7148 |
| COW_d23 | 6.9444 | 7.7148 |
| COW_d29 | 6.2917 | 7.7148 |
| animal_days_SHEEP | 161.7778 | 159.2695 |
| fed_SHEEP | 0.8015 | 0.9346 |
| cared_SHEEP | 0.7394 | 0.9628 |
| collected_fertilizer_SHEEP | 0.9205 | 0.8883 |
| SHEEP_d0 | 1.9167 | 2.0000 |
| SHEEP_d2 | 1.9167 | 2.0000 |
| SHEEP_d5 | 2.4444 | 2.0000 |
| SHEEP_d8 | 4.6944 | 4.3711 |
| SHEEP_d11 | 6.6667 | 6.6680 |
| SHEEP_d17 | 6.9306 | 6.6680 |
| SHEEP_d23 | 6.8056 | 6.6680 |
| SHEEP_d29 | 4.7778 | 6.6680 |
| animal_days_GOOSE | 45.0694 | 57.1484 |
| fed_GOOSE | 0.9067 | 0.9019 |
| cared_GOOSE | 0.8796 | 0.9588 |
| collected_fertilizer_GOOSE | 0.9102 | 0.8156 |
| GOOSE_d0 | 0.0833 | 0.0000 |
| GOOSE_d2 | 0.0833 | 0.0000 |
| GOOSE_d5 | 0.1389 | 0.0000 |
| GOOSE_d8 | 0.8194 | 0.0000 |
| GOOSE_d11 | 2.0694 | 2.6289 |
| GOOSE_d17 | 2.0972 | 2.9375 |
| GOOSE_d23 | 2.0972 | 2.9375 |
| GOOSE_d29 | 2.0694 | 2.9375 |
| crop_days | 1498.9444 | 1497.9180 |
| crop_water_rate | 0.7071 | 0.7208 |
| crop_yield_day_maximized | 0.4689 | 0.2095 |
| harvest_WHEAT | 527.0278 | 508.5977 |
| harvest_CARROT | 122.9444 | 79.3555 |
| harvest_TOMATO | 39.1528 | 4.2188 |
| harvest_STRAWBERRY | 229.1389 | 249.0195 |
| harvest_MELON | 78.1389 | 72.0000 |
| hires | 280.9306 | 262.6875 |
| hire_cost | 5436.7500 | 3945.7070 |
| land_buys | 2.0139 | 2.0000 |
| land_cost | 3055.5556 | 3000.0000 |
| weed_digs | 18.2639 | 20.4570 |
| unit_faults | 48.1667 | 14.7109 |
| SELL_weighted_hour | 7.0576 | 10.2816 |
| BUY_PRODUCT_weighted_hour | 6.0372 | 5.7946 |
