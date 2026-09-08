# Review75 — 2026-09-08T02:48:31.675445+00:00

The interval made concrete progress on the full composition loop.375 larger
farms were estimated over16 scenarios in0.210s,54 distinct complete policies
were tested in1104 profiled discovery games across two selection rounds.
Thirty-two old baseline records and208 repeated candidate records are exact.
All54 packages passed448 operational games including debug self/PASS and
shared generic/pair/thread checks on p362. No new strong-agent promotion.

Unconstrained estimated margins rank exact margins with correlations0.911 and
0.896 against public_router and currentbest. Restricting selection to no larger
mean minimum cash deficit than the original opening does not beat the retained
leaders. Small funding deficits are not feasibility proofs; negative estimated
cash remains explicit. The larger farms raise own cash from about47k to58–61k,
but every game still loses to the strong opponents. Exact best margins remain
roughly minus52k to minus57k. General competitive construction is incomplete.

Animal decomposition across40 games uses requested birth/full service, actual
birth/survival/full service, actual feed/care/collection masks with daily harvest,
and real harvested output. On p362 versus public, ideal eggs/milk/wool/fertilizer
[152,168,172,368] fall to[146,165.75,156,350.5] after actual births; observed
service then yields[117.25,147.5,136,339.5], actual[111.25,147.5,136,339.5].
Thus funding/birth delay is only part of the gap; feed and service matter.
This is a decomposition, not proof that maximal daily service always pays.

The day solver now works on a larger cold farm's own tasks. The first four-day
study reproduces the complete original game and solves12 day cases exactly;
two fewer hires save233/day with unchanged field output. No eligible missing
care was found on already-fed animals. Actual profiles instead expose unused
wheat while animals remain unfed. The second study adds8 missing feeds onday18
and3 onday26 from preserved wheat, computes correct new stored-output/care-bank
states, and still solves with two fewer hires. All12 cases replay exactly in
the root engine, no unit faults, expected cash and complete endpoints. Solves
use about0.07–0.10s. These are day-level results, not full-season strength yet.

Next: integrate guarded full-day schedules into p362, separate labor-only and
feed-service changes, measure activation, full-season output/cash and new
scenarios. Then generalize task generation and worker/input assignment to the
observed failure; avoid one-template-per-seed becoming a substitute for a
composition compiler. Keep strong-incumbent search and replay borrowing active.
Best remains observed_sale_lead_start_216. Final900000 unused.

All82 metrics carried fromReview74: global02:08 cohort versus current native256.
Next review03:08, coinciding with the next replay/notebook refresh. Goal active
throughSep10 00:47UTC. No Git, official catalog copy or submission.

| Metric | Global72 (02:08) | Current native256 |
| --- | ---: | ---: |
| cash | 102151.2500 | 95546.7109 |
| margin | 2566.2083 | 1625.5859 |
| discards | 9.2778 | 2.2305 |
| buy_WHEAT | 218.8611 | 169.8984 |
| sell_WHEAT | 425.4167 | 303.7656 |
| net_WHEAT | 206.5556 | 133.8672 |
| buy_CARROT | 0.0000 | 0.0000 |
| sell_CARROT | 97.6667 | 72.7852 |
| net_CARROT | 97.6667 | 72.7852 |
| buy_TOMATO | 0.0000 | 0.0000 |
| sell_TOMATO | 41.7361 | 4.3125 |
| net_TOMATO | 41.7361 | 4.3125 |
| buy_STRAWBERRY | 0.0000 | 0.0000 |
| sell_STRAWBERRY | 239.5278 | 247.4492 |
| net_STRAWBERRY | 239.5278 | 247.4492 |
| buy_MELON | 0.0000 | 0.0000 |
| sell_MELON | 75.5556 | 72.0000 |
| net_MELON | 75.5556 | 72.0000 |
| buy_EGG | 0.0000 | 0.0000 |
| sell_EGG | 63.7778 | 66.2188 |
| net_EGG | 63.7778 | 66.2188 |
| buy_MILK | 0.0000 | 0.0000 |
| sell_MILK | 199.8750 | 231.6719 |
| net_MILK | 199.8750 | 231.6719 |
| buy_WOOL | 0.0000 | 0.0000 |
| sell_WOOL | 148.7778 | 184.7461 |
| net_WOOL | 148.7778 | 184.7461 |
| buy_FERTILIZER | 45.4861 | 49.4844 |
| sell_FERTILIZER | 278.2778 | 362.1523 |
| net_FERTILIZER | 232.7917 | 312.6680 |
| animal_days_COW | 198.2500 | 199.7656 |
| fed_COW | 0.7679 | 0.8869 |
| cared_COW | 0.8004 | 0.9455 |
| collected_fertilizer_COW | 0.9432 | 0.9506 |
| COW_d0 | 2.1667 | 2.0000 |
| COW_d2 | 3.0000 | 3.0000 |
| COW_d5 | 4.1250 | 4.0000 |
| COW_d8 | 7.2222 | 7.5469 |
| COW_d11 | 7.8472 | 7.6016 |
| COW_d17 | 7.8750 | 7.6250 |
| COW_d23 | 7.5833 | 7.6250 |
| COW_d29 | 6.9306 | 7.6250 |
| animal_days_SHEEP | 154.5278 | 166.8320 |
| fed_SHEEP | 0.8228 | 0.9372 |
| cared_SHEEP | 0.7895 | 0.9634 |
| collected_fertilizer_SHEEP | 0.9195 | 0.8864 |
| SHEEP_d0 | 1.9167 | 2.0000 |
| SHEEP_d2 | 1.9167 | 2.0000 |
| SHEEP_d5 | 2.1250 | 2.0000 |
| SHEEP_d8 | 4.1806 | 4.4531 |
| SHEEP_d11 | 6.3056 | 7.0273 |
| SHEEP_d17 | 6.8472 | 7.0273 |
| SHEEP_d23 | 6.5278 | 7.0273 |
| SHEEP_d29 | 5.1111 | 7.0273 |
| animal_days_GOOSE | 41.4028 | 52.9102 |
| fed_GOOSE | 0.8971 | 0.9026 |
| cared_GOOSE | 0.8769 | 0.9557 |
| collected_fertilizer_GOOSE | 0.8774 | 0.8179 |
| GOOSE_d0 | 0.0833 | 0.0000 |
| GOOSE_d2 | 0.0833 | 0.0000 |
| GOOSE_d5 | 0.1111 | 0.0000 |
| GOOSE_d8 | 0.9167 | 0.0000 |
| GOOSE_d11 | 1.7778 | 2.3711 |
| GOOSE_d17 | 1.8889 | 2.7305 |
| GOOSE_d23 | 1.9167 | 2.7305 |
| GOOSE_d29 | 1.9167 | 2.7305 |
| crop_days | 1495.7222 | 1494.7930 |
| crop_water_rate | 0.6989 | 0.7213 |
| crop_yield_day_maximized | 0.4698 | 0.2087 |
| harvest_WHEAT | 529.7500 | 515.5586 |
| harvest_CARROT | 99.4028 | 72.7852 |
| harvest_TOMATO | 42.2500 | 4.3125 |
| harvest_STRAWBERRY | 240.4306 | 247.4492 |
| harvest_MELON | 75.5833 | 72.0000 |
| hires | 276.6944 | 261.6133 |
| hire_cost | 5119.2083 | 3845.6055 |
| land_buys | 2.0000 | 2.0000 |
| land_cost | 3000.0000 | 3000.0000 |
| weed_digs | 19.5000 | 20.1875 |
| unit_faults | 44.2222 | 11.8750 |
| SELL_weighted_hour | 7.2522 | 10.3942 |
| BUY_PRODUCT_weighted_hour | 6.6446 | 5.7633 |
