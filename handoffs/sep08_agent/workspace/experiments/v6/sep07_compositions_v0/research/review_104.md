# Review104 — 2026-09-08T13:17:24.283316+00:00

Concrete progress. Current accepted reference remains empty_sale_slots_m2;
no new promotion. Freshq24comparison finished12288games, all476frozen inputs
unchanged. Seven-neutral utility improves12.081pp (95%11.412..12.695), but mean
margin loses100.89 (95%-173.00..-29.87) and mean owncash also falls. The native
neutral subset covers only teammate/Yusuke and is not a broad field. Preserve
this important win-versus-margin tradeoff; it fails the old positive-margin
promotion rule. Do not call this fresh test a failure to gain wins or silently
waive a gate. Test combinations and broader field/cash-tail tradeoffs explicitly.
Exact per-opponent results in opening_funding_sep08_001/FRESH_RESULTS.md/json.

Animal+premium combination packages pass32required operations. Their independent
36864-game six-policy factor/native comparison is still running under exact
handle28950. This compares parent, sale priority, animal fixed/live-market modes,
and both combinations, with identical seeds/both seats. Do not restart it on
observation timeout. Parent/premium/animal sources remain frozen and unchanged.

Earlierday8cowpair/day10goosepair now independently pass all719actions and30
endpoints. Advancing two planned geese gains519own/595margin, extra labor432;
cheap own1032.59becomes600.59after labor, residual-81.59. Two early cows lose
4145own/857margin despite48extra milk/40fertilizer, labor1631. Cheap own was
already-457.03 but margin+4722.94 due modeled rival damage; afterlabor own
residual-2056.97. Separate future-shop uncertainty, market timing and rival-flow
error before claiming the cheap estimator is calibrated for early investment.

The corrected compiler maps land purchases to actual quadrant indices. The
mixedday8entry cannot fund three animals. Delayingday9/day10 exposed a previous
compiler restriction requiring natural harvest; compile_flexible now explicitly
harvests mature crops/removes remaining plants/weeds before animal entry.
Mixedday10 passes its entry with2extra hires and progresses through laterdays;
day9has a physicalschedule but liveendpoint failure. Exact handles96318 remain
live; this is construction work, no runtime agent/league win. Other occupied
animal/structure replacements remain unsupported and explicit.

Guardrepair still needs work: unexpected weed38causes missing wheat in one
animal-selector game while added cows produce normally. Source trace reproduces
both saved game hashes. Next solve or deliberately price that extra field task;
do not discard new animals or assert full guard coverage.

Globalrefresh1308completed72player-games/57unique replays.
All82metrics below update from that cohort; native256 remains unchanged.
Three changed notebook snapshots were downloaded without cell execution.
Yusuke archive's main.py,observation.py,model.json,actions.json all match the
verified priorport byte-for-byte; no new port needed. Its episode-visibility
notebook and NagataV5.7 controller still need content-level audit.

Next review13:36UTC, globalrefresh14:08UTC. Final900000unused. Full goal active:
continue composition selection/placement/labor/funding, broad league and cold
farms, while using new public components only with recoverable lineage.

| Metric | Global72 (13:08) | Current native256 |
| --- | ---: | ---: |
| cash | 96396.0417 | 96856.4492 |
| margin | 2661.7361 | 240.9453 |
| discards | 6.5278 | 2.6055 |
| buy_WHEAT | 267.1806 | 169.8672 |
| sell_WHEAT | 466.8056 | 298.7969 |
| net_WHEAT | 199.6250 | 128.9297 |
| buy_CARROT | 0.0000 | 0.0000 |
| sell_CARROT | 131.2083 | 79.3047 |
| net_CARROT | 131.2083 | 79.3047 |
| buy_TOMATO | 0.0000 | 0.0000 |
| sell_TOMATO | 30.8056 | 4.2188 |
| net_TOMATO | 30.8056 | 4.2188 |
| buy_STRAWBERRY | 0.0000 | 0.0000 |
| sell_STRAWBERRY | 234.1944 | 248.9219 |
| net_STRAWBERRY | 234.1944 | 248.9219 |
| buy_MELON | 0.0000 | 0.0000 |
| sell_MELON | 79.8472 | 72.0000 |
| net_MELON | 79.8472 | 72.0000 |
| buy_EGG | 0.0000 | 0.0000 |
| sell_EGG | 57.4444 | 70.9609 |
| net_EGG | 57.4444 | 70.9609 |
| buy_MILK | 0.0000 | 0.0000 |
| sell_MILK | 182.0833 | 233.3164 |
| net_MILK | 182.0833 | 233.3164 |
| buy_WOOL | 0.0000 | 0.0000 |
| sell_WOOL | 145.7083 | 176.1328 |
| net_WOOL | 145.7083 | 176.1328 |
| buy_FERTILIZER | 62.4306 | 50.1250 |
| sell_FERTILIZER | 281.7778 | 361.7266 |
| net_FERTILIZER | 219.3472 | 311.6016 |
| animal_days_COW | 191.3194 | 201.8438 |
| fed_COW | 0.7525 | 0.8863 |
| cared_COW | 0.7660 | 0.9458 |
| collected_fertilizer_COW | 0.9385 | 0.9494 |
| COW_d0 | 2.4167 | 2.0000 |
| COW_d2 | 2.9861 | 2.9961 |
| COW_d5 | 4.0139 | 3.9844 |
| COW_d8 | 6.8194 | 7.6094 |
| COW_d11 | 7.5278 | 7.6836 |
| COW_d17 | 7.6250 | 7.7148 |
| COW_d23 | 7.3333 | 7.7148 |
| COW_d29 | 6.5278 | 7.7148 |
| animal_days_SHEEP | 155.5833 | 159.2695 |
| fed_SHEEP | 0.8020 | 0.9346 |
| cared_SHEEP | 0.7428 | 0.9628 |
| collected_fertilizer_SHEEP | 0.9212 | 0.8883 |
| SHEEP_d0 | 2.0000 | 2.0000 |
| SHEEP_d2 | 2.0000 | 2.0000 |
| SHEEP_d5 | 2.4722 | 2.0000 |
| SHEEP_d8 | 4.2083 | 4.3711 |
| SHEEP_d11 | 6.0972 | 6.6680 |
| SHEEP_d17 | 6.7639 | 6.6680 |
| SHEEP_d23 | 6.6111 | 6.6680 |
| SHEEP_d29 | 4.7361 | 6.6680 |
| animal_days_GOOSE | 37.1389 | 57.1484 |
| fed_GOOSE | 0.9100 | 0.9019 |
| cared_GOOSE | 0.8837 | 0.9588 |
| collected_fertilizer_GOOSE | 0.9005 | 0.8156 |
| GOOSE_d0 | 0.0833 | 0.0000 |
| GOOSE_d2 | 0.0833 | 0.0000 |
| GOOSE_d5 | 0.1250 | 0.0000 |
| GOOSE_d8 | 0.5556 | 0.0000 |
| GOOSE_d11 | 1.6944 | 2.6289 |
| GOOSE_d17 | 1.7361 | 2.9375 |
| GOOSE_d23 | 1.7639 | 2.9375 |
| GOOSE_d29 | 1.7639 | 2.9375 |
| crop_days | 1510.4722 | 1497.9180 |
| crop_water_rate | 0.7068 | 0.7208 |
| crop_yield_day_maximized | 0.4444 | 0.2095 |
| harvest_WHEAT | 509.3889 | 508.5977 |
| harvest_CARROT | 132.3472 | 79.3555 |
| harvest_TOMATO | 31.0833 | 4.2188 |
| harvest_STRAWBERRY | 234.7639 | 249.0195 |
| harvest_MELON | 79.9167 | 72.0000 |
| hires | 282.0556 | 262.6875 |
| hire_cost | 5451.4306 | 3945.7070 |
| land_buys | 2.0139 | 2.0000 |
| land_cost | 3055.5556 | 3000.0000 |
| weed_digs | 19.2778 | 20.4570 |
| unit_faults | 41.9028 | 14.7109 |
| SELL_weighted_hour | 7.0672 | 10.2816 |
| BUY_PRODUCT_weighted_hour | 5.0646 | 5.7946 |
