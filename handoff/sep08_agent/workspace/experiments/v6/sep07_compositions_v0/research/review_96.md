# Review96 — 2026-09-08T10:15:55.953506+00:00

Concrete progress; no blocker. Global empty_sale_slots_m2 and cold
early_melon_b98_m1 remain. Goal throughSep10 00:47UTC; final900000 unused.

Corrected group screen:281256proposals/18scenarios,4242exact crop lifetimes,
317exact animal lifetimes and36full source/control games.14.695estimator seconds,
52.25us per proposal including32conditional shop samples. Initial screen omitted
already planned animals on some released melon tiles; those scores are invalid.
The compiler also duplicated their build/place tasks. New full-lifecycle model
subtracts their output, feed, fertilizer, work and cancelable future buys. Sunk
animal stock is retained. Goosepair estimate2450.69->1032.59; original failed
artifacts retained. Scores still use retrospective calendars and unpriced labor.

Actual406cash bought1 of2requested cows. Funded purchase insertion fixes day8
with no extra hire; day9 passes with1. Earlyday10 remainsUNKNOWN30s.42strict
source-day controls pass with noerrors after removing duplicate explicit sales
from the physical checker.10nohint/softhint light-exact comparisons yield no
schedules; originalday10 has an exact known-feasible witness. Duplicated-goose
models are malformed, not proof that intended replacement is impossible.

28isolated engine/model cases show a fully serviced cow can omit care on
birth+1 andbirth+2 with identical daily milk/fertilizer, saving2field actions.
The initial oracle forgot daily feed pickup; failure and correction retained.
No full-farm profit claim follows.

Later sheepday20 and cowday15 groups plus singles are now compiling. Three sheep
singles reachday28 exactly, then fail to find day29 schedules. Cow single reaches
day26 and then requires unavailable carrot inventory from displaced crops.
Next correct the continuation's inventory targets and final useful service,
reusing certified prefixes; finish matched economics before a runtime policy.

Fresh72player-games/55replays update all82global metrics below. Local256
is unchanged; unmatched populations, no causal comparison. Two notebooks audited
statically: funding experiment discloses donor0/48wins againstAhmedV23, so no
port prioritized; Salem contains720-action8cow/4sheep course with weed replay and
next-turn sales. Source/tape decoded for C++ conversion; donor score claims are
not our validation. No notebook/policy code executed in this audit.

Next review10:28; next replay/notebook refresh11:08. No Git, upload, official
catalog, subagents or GPU workload changes. Goal remains active.

| Metric | Global72 (10:08) | Current native256 |
| --- | ---: | ---: |
| cash | 92486.8611 | 96856.4492 |
| margin | 113.4722 | 240.9453 |
| discards | 7.8611 | 2.6055 |
| buy_WHEAT | 242.1528 | 169.8672 |
| sell_WHEAT | 455.4722 | 298.7969 |
| net_WHEAT | 213.3194 | 128.9297 |
| buy_CARROT | 0.0000 | 0.0000 |
| sell_CARROT | 132.0139 | 79.3047 |
| net_CARROT | 132.0139 | 79.3047 |
| buy_TOMATO | 0.0000 | 0.0000 |
| sell_TOMATO | 28.8889 | 4.2188 |
| net_TOMATO | 28.8889 | 4.2188 |
| buy_STRAWBERRY | 0.0000 | 0.0000 |
| sell_STRAWBERRY | 225.3056 | 248.9219 |
| net_STRAWBERRY | 225.3056 | 248.9219 |
| buy_MELON | 0.0000 | 0.0000 |
| sell_MELON | 78.4444 | 72.0000 |
| net_MELON | 78.4444 | 72.0000 |
| buy_EGG | 0.0000 | 0.0000 |
| sell_EGG | 69.5278 | 70.9609 |
| net_EGG | 69.5278 | 70.9609 |
| buy_MILK | 0.0000 | 0.0000 |
| sell_MILK | 184.9306 | 233.3164 |
| net_MILK | 184.9306 | 233.3164 |
| buy_WOOL | 0.0000 | 0.0000 |
| sell_WOOL | 132.4583 | 176.1328 |
| net_WOOL | 132.4583 | 176.1328 |
| buy_FERTILIZER | 61.5417 | 50.1250 |
| sell_FERTILIZER | 286.5972 | 361.7266 |
| net_FERTILIZER | 225.0556 | 311.6016 |
| animal_days_COW | 190.4028 | 201.8438 |
| fed_COW | 0.7542 | 0.8863 |
| cared_COW | 0.7666 | 0.9458 |
| collected_fertilizer_COW | 0.9401 | 0.9494 |
| COW_d0 | 2.4167 | 2.0000 |
| COW_d2 | 3.0139 | 2.9961 |
| COW_d5 | 4.0139 | 3.9844 |
| COW_d8 | 6.5000 | 7.6094 |
| COW_d11 | 7.2778 | 7.6836 |
| COW_d17 | 7.6667 | 7.7148 |
| COW_d23 | 7.5000 | 7.7148 |
| COW_d29 | 6.5833 | 7.7148 |
| animal_days_SHEEP | 146.1806 | 159.2695 |
| fed_SHEEP | 0.7802 | 0.9346 |
| cared_SHEEP | 0.7230 | 0.9628 |
| collected_fertilizer_SHEEP | 0.9137 | 0.8883 |
| SHEEP_d0 | 1.9167 | 2.0000 |
| SHEEP_d2 | 1.9167 | 2.0000 |
| SHEEP_d5 | 2.3611 | 2.0000 |
| SHEEP_d8 | 4.4167 | 4.3711 |
| SHEEP_d11 | 6.1528 | 6.6680 |
| SHEEP_d17 | 6.3472 | 6.6680 |
| SHEEP_d23 | 6.0694 | 6.6680 |
| SHEEP_d29 | 3.8750 | 6.6680 |
| animal_days_GOOSE | 45.6389 | 57.1484 |
| fed_GOOSE | 0.9091 | 0.9019 |
| cared_GOOSE | 0.8726 | 0.9588 |
| collected_fertilizer_GOOSE | 0.8857 | 0.8156 |
| GOOSE_d0 | 0.0833 | 0.0000 |
| GOOSE_d2 | 0.0833 | 0.0000 |
| GOOSE_d5 | 0.1528 | 0.0000 |
| GOOSE_d8 | 0.7222 | 0.0000 |
| GOOSE_d11 | 2.1111 | 2.6289 |
| GOOSE_d17 | 2.1528 | 2.9375 |
| GOOSE_d23 | 2.1389 | 2.9375 |
| GOOSE_d29 | 2.1389 | 2.9375 |
| crop_days | 1500.7778 | 1497.9180 |
| crop_water_rate | 0.7104 | 0.7208 |
| crop_yield_day_maximized | 0.4203 | 0.2095 |
| harvest_WHEAT | 520.1667 | 508.5977 |
| harvest_CARROT | 133.1111 | 79.3555 |
| harvest_TOMATO | 28.9444 | 4.2188 |
| harvest_STRAWBERRY | 226.1667 | 249.0195 |
| harvest_MELON | 78.4861 | 72.0000 |
| hires | 280.6528 | 262.6875 |
| hire_cost | 5410.2500 | 3945.7070 |
| land_buys | 2.0139 | 2.0000 |
| land_cost | 3055.5556 | 3000.0000 |
| weed_digs | 17.8333 | 20.4570 |
| unit_faults | 66.3611 | 14.7109 |
| SELL_weighted_hour | 7.6377 | 10.2816 |
| BUY_PRODUCT_weighted_hour | 6.0459 | 5.7946 |
