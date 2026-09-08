# Review72 — 2026-09-08T01:57:54.597326+00:00

Due01:48; recorded after checking whether the compiled first-day schedules
actually execute in complete games. Best remains observed_sale_lead_start_216;
full objective stays active through Sep10 00:47UTC. This interval made concrete
progress in placement/financing diagnosis and direct task generation.

Placement study completed256games,64 exact old controls,64 unchanged dairy/wool
records andtwo exact full-game traces. Owned-first mixed placement buys sheep
onday0 instead ofday8 in all32 games. The witness moves its$1000land purchase
fromstep0 to168, so those funds buy the two sheep initially. Public opponent
owncash+$1660.375, margin+$4682.625; current opponent owncash+$2264.75,
margin+$6373.375. More wool/fertilizer comes with more walking and less wheat.
Goose saves land cost while producing fewer eggs/fertilizer. Both remain weak.

New cold_day_tasks_sep08_001 generates initial-day work from raw dated lives and
owned-first placement:19crop plant/water pairs,4animal structures/placements,
explicit service choices, fixed purchases and8hires. No source worker routes or
assignment hints enter the day problem. All three cases solve in51–67ms and
match the root full engine's next-morning tiles, stocks, seeds and zero faults
againstPASS. Physical day feasibility is still distinct from economic value.

Three complete C++ policies plus an unchanged explicit-layout control ran768
profiled games againstpublic_router,current andPASS. First-day improvements
are only about$1–$100cash. A separate96-game observer reproduces complete prior
records and confirms every game executes all24 compiled hours; none relies on
the unit-count fallback. Earlier concern about hidden fallback activation is
therefore resolved for that measured panel. Skipped initial service saves wheat
without changing animal output in this continuation, but better later service
could change its value. Do not claim biological equivalence from this result.

Operational generic/pair/debug/self/PASS/thread checks are running in86810.
The paired runner's initial command omitted output agent labels; the static
pair is correct, but its result metadata must be corrected by a labeled rerun
after completion, preserving the original. This does not change policy code.
No cold candidate is promoted as a strong agent.

Next: later-day task construction and exact resource/finance constraints, then
full-season error feedback. Do not continue tuning a first day that explains
little of the remaining gap. Connect placement explicitly to the cheap model;
keep larger/independent compositions, observed-shop/opponent choices and the
growing league. The original request includes a strong arbitrary composition
compiler, not just a successful initial-day example.

Latest external cohort remains01:08; all82 current/global metrics below are
explicitly carried forward fromReview70. Different cohorts are not a paired
top-player strength estimate. Next review02:08 and hourly refresh02:08.
Final900000 remains unused. No Git, officialagentcopy or Kaggle submission.

| Metric | Global72 (01:08) | Current native256 |
| --- | ---: | ---: |
| cash | 99774.1250 | 95546.7109 |
| margin | 2038.9167 | 1625.5859 |
| discards | 6.3472 | 2.2305 |
| buy_WHEAT | 220.9167 | 169.8984 |
| sell_WHEAT | 413.3194 | 303.7656 |
| net_WHEAT | 192.4028 | 133.8672 |
| buy_CARROT | 0.0000 | 0.0000 |
| sell_CARROT | 106.3611 | 72.7852 |
| net_CARROT | 106.3611 | 72.7852 |
| buy_TOMATO | 0.0000 | 0.0000 |
| sell_TOMATO | 33.3194 | 4.3125 |
| net_TOMATO | 33.3194 | 4.3125 |
| buy_STRAWBERRY | 0.0000 | 0.0000 |
| sell_STRAWBERRY | 239.5694 | 247.4492 |
| net_STRAWBERRY | 239.5694 | 247.4492 |
| buy_MELON | 0.0000 | 0.0000 |
| sell_MELON | 78.0833 | 72.0000 |
| net_MELON | 78.0833 | 72.0000 |
| buy_EGG | 0.0000 | 0.0000 |
| sell_EGG | 69.3333 | 66.2188 |
| net_EGG | 69.3333 | 66.2188 |
| buy_MILK | 0.0000 | 0.0000 |
| sell_MILK | 189.2917 | 231.6719 |
| net_MILK | 189.2917 | 231.6719 |
| buy_WOOL | 0.0000 | 0.0000 |
| sell_WOOL | 148.2500 | 184.7461 |
| net_WOOL | 148.2500 | 184.7461 |
| buy_FERTILIZER | 58.5139 | 49.4844 |
| sell_FERTILIZER | 290.6806 | 362.1523 |
| net_FERTILIZER | 232.1667 | 312.6680 |
| animal_days_COW | 186.9722 | 199.7656 |
| fed_COW | 0.7739 | 0.8869 |
| cared_COW | 0.8062 | 0.9455 |
| collected_fertilizer_COW | 0.9399 | 0.9506 |
| COW_d0 | 2.1667 | 2.0000 |
| COW_d2 | 2.9167 | 3.0000 |
| COW_d5 | 3.9861 | 4.0000 |
| COW_d8 | 6.9444 | 7.5469 |
| COW_d11 | 7.3472 | 7.6016 |
| COW_d17 | 7.3056 | 7.6250 |
| COW_d23 | 7.0833 | 7.6250 |
| COW_d29 | 6.4444 | 7.6250 |
| animal_days_SHEEP | 155.3611 | 166.8320 |
| fed_SHEEP | 0.8043 | 0.9372 |
| cared_SHEEP | 0.7771 | 0.9634 |
| collected_fertilizer_SHEEP | 0.9208 | 0.8864 |
| SHEEP_d0 | 2.0000 | 2.0000 |
| SHEEP_d2 | 2.0000 | 2.0000 |
| SHEEP_d5 | 2.2361 | 2.0000 |
| SHEEP_d8 | 4.3333 | 4.4531 |
| SHEEP_d11 | 6.4722 | 7.0273 |
| SHEEP_d17 | 6.9583 | 7.0273 |
| SHEEP_d23 | 6.4028 | 7.0273 |
| SHEEP_d29 | 4.2917 | 7.0273 |
| animal_days_GOOSE | 44.9722 | 52.9102 |
| fed_GOOSE | 0.9061 | 0.9026 |
| cared_GOOSE | 0.8902 | 0.9557 |
| collected_fertilizer_GOOSE | 0.8768 | 0.8179 |
| GOOSE_d0 | 0.0833 | 0.0000 |
| GOOSE_d2 | 0.0833 | 0.0000 |
| GOOSE_d5 | 0.1528 | 0.0000 |
| GOOSE_d8 | 0.8750 | 0.0000 |
| GOOSE_d11 | 2.0000 | 2.3711 |
| GOOSE_d17 | 2.0556 | 2.7305 |
| GOOSE_d23 | 2.0972 | 2.7305 |
| GOOSE_d29 | 2.0833 | 2.7305 |
| crop_days | 1492.1111 | 1494.7930 |
| crop_water_rate | 0.7137 | 0.7213 |
| crop_yield_day_maximized | 0.4336 | 0.2087 |
| harvest_WHEAT | 509.1944 | 515.5586 |
| harvest_CARROT | 107.0694 | 72.7852 |
| harvest_TOMATO | 33.6250 | 4.3125 |
| harvest_STRAWBERRY | 240.7083 | 247.4492 |
| harvest_MELON | 78.1389 | 72.0000 |
| hires | 277.1667 | 261.6133 |
| hire_cost | 5163.0833 | 3845.6055 |
| land_buys | 2.0000 | 2.0000 |
| land_cost | 3000.0000 | 3000.0000 |
| weed_digs | 19.1528 | 20.1875 |
| unit_faults | 43.5000 | 11.8750 |
| SELL_weighted_hour | 7.3582 | 10.3942 |
| BUY_PRODUCT_weighted_hour | 6.4420 | 5.7633 |
