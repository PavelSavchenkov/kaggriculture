# Review61 — 2026-09-07T22:12:54.122275+00:00

Due22:08UTC. Current reference remains wool_family_context_v2. The previous
turn's promotion and this turn's public-state/bridge experiment are concrete
progress. Goal active through00:47UTC; no Git or new submission.

The one-hour delayed family bridge is implemented and verified in
runs/family_delay_001.1024 full instrumented baseline games exactly match their
previous records. Public states atday6h0 match betweenV5/2 andJunghoon in all256
paired contexts; afterh0 actual market purchases differ. The bridge shares the
parent's7-hire h0, defers source purchases, commutes farmer movement and moves
the extra early worker east. Of256 paired tests,255activate. Every actual own
physical field matches the original forced transfer at h2,h3,h4; all256 have
identical production and hire cost. Trades differ in28 games, and own mean
cash falls$2.293, so physical equality alone is not full economic parity.

12288 discovery games compare unchanged earlyv2, delayedv2 and delayedrival
selection across4opponents. Delayedv2 reproduces utilities with tiny costs.
Rival-observed expansion raisesV5/2 utility79.8828%→87.2070% on1024games. A hybrid
keeps existing early choices and delays only new alternatives, avoiding that
small unnecessary cost. Its first implementation is rival_wool_context_v1.

Fresh53248games/26opponents at1910000 fail promotion. V5/2 utility gains8.0078pp
and mean margin$314.97, but John loses2.0508pp despite mean+$133.09. Otherpaired
contrasts are exactlyzero. Current grouped95%gain is+0.0814..+0.3296pp;
historical95%gain-0.2075..-0.0610pp. The preregistered zero individual utility
loss rule fails. Candidate remains experimental; all1024 operational checks
pass, including9custom and5native cases where the new branch changes actions.
Pair/debug/thread checks now useV5/2, because parent-only checks would miss the
new branch entirely. Generic/pair/debug complete records match.

The original direct-parent gate also incorrectly required no raw losses. The
candidate actually reproduces every one of1024 parent-self full records:
94wins,836ties,94losses and zero mean is the parent's own seat/stochastic
asymmetry. The historical failed result is preserved. Future targeted-response
gates must compare paired outcomes with the parent-self control, not assume
identical policies always draw each individual game. This correction does not
rescue the failed John gate or justify promotion.

Diagnosis: the wheat signal aliases John's plan. Static source inspection
showsJohn's h0 buys1wheat and sells1fertilizer; V5/2 wool buys1wheat and seeds,
with no fertilizer sale. Since our common h0 contains only hires, fertilizer
market change is a directly observed rival net flow. Test that richer market
observation before restricting composition choices further. Failed1910000 is
now diagnosis; a changed policy needs a new pool.

Fresh22:02 retrieval completed72playergames, no changed public notebooks among
100listed. Currentleader ymg_aq, score2973.9 at22:03:42 snapshot. Its latest
sample includes166483cash againstJunghoon,1failedunitaction and no discards;
this is one favorable match, not an estimate of universal strength. Latest
cohort differs from21:02: meanwool+8.01, milk-17.76, eggs+5.78, crop productive
fertilizer coverage+0.040. Continue inspecting full composition/timing patterns.
The general raw-composition estimator, placement/labor model and cold compiler
remain unfinished original requirements. Exact source calendars and response
branches advance the strong-agent objective but do not close these gaps.

All82metrics below use newglobal72 at22:02 and the currently promotedv2 native256
profile against its parent. Differentcohorts; no paired rank claim. Nextreview
22:28UTC, refresh23:02UTC. Final900000 unused. Candidate fresh1930000 reserved
only after checking no prior use.

| Metric | Global72 (22:02) | Promotedv2 native256 |
| --- | ---: | ---: |
| cash | 99897.8750 | 97954.6562 |
| margin | 2009.6250 | 450.5078 |
| discards | 7.5000 | 2.6836 |
| buy_WHEAT | 342.5694 | 170.0156 |
| sell_WHEAT | 530.0000 | 303.3789 |
| net_WHEAT | 187.4306 | 133.3633 |
| buy_CARROT | 0.0000 | 0.0000 |
| sell_CARROT | 100.6250 | 75.2969 |
| net_CARROT | 100.6250 | 75.2969 |
| buy_TOMATO | 0.0000 | 0.0000 |
| sell_TOMATO | 44.4583 | 4.8750 |
| net_TOMATO | 44.4583 | 4.8750 |
| buy_STRAWBERRY | 0.0000 | 0.0000 |
| sell_STRAWBERRY | 226.1111 | 247.8438 |
| net_STRAWBERRY | 226.1111 | 247.8438 |
| buy_MELON | 0.0000 | 0.0000 |
| sell_MELON | 78.5000 | 72.0000 |
| net_MELON | 78.5000 | 72.0000 |
| buy_EGG | 0.0000 | 0.0000 |
| sell_EGG | 65.8750 | 63.8945 |
| net_EGG | 65.8750 | 63.8945 |
| buy_MILK | 0.0000 | 0.0000 |
| sell_MILK | 194.5972 | 230.4336 |
| net_MILK | 194.5972 | 230.4336 |
| buy_WOOL | 0.0000 | 0.0000 |
| sell_WOOL | 147.6111 | 184.6719 |
| net_WOOL | 147.6111 | 184.6719 |
| buy_FERTILIZER | 47.3750 | 49.6797 |
| sell_FERTILIZER | 277.7361 | 360.5156 |
| net_FERTILIZER | 230.3611 | 310.8359 |
| animal_days_COW | 196.9028 | 199.0664 |
| fed_COW | 0.7675 | 0.8870 |
| cared_COW | 0.7613 | 0.9471 |
| collected_fertilizer_COW | 0.9430 | 0.9502 |
| COW_d0 | 2.2500 | 2.0000 |
| COW_d2 | 2.8750 | 2.9922 |
| COW_d5 | 3.9583 | 3.9883 |
| COW_d8 | 7.0278 | 7.4727 |
| COW_d11 | 7.8611 | 7.5586 |
| COW_d17 | 7.9861 | 7.6055 |
| COW_d23 | 7.4722 | 7.6055 |
| COW_d29 | 6.5417 | 7.6055 |
| animal_days_SHEEP | 151.4167 | 166.9414 |
| fed_SHEEP | 0.8208 | 0.9355 |
| cared_SHEEP | 0.7732 | 0.9637 |
| collected_fertilizer_SHEEP | 0.9249 | 0.8873 |
| SHEEP_d0 | 2.0000 | 2.0000 |
| SHEEP_d2 | 2.0000 | 2.0000 |
| SHEEP_d5 | 2.2500 | 2.0000 |
| SHEEP_d8 | 4.0000 | 4.5156 |
| SHEEP_d11 | 5.5556 | 7.0273 |
| SHEEP_d17 | 6.5972 | 7.0273 |
| SHEEP_d23 | 6.7222 | 7.0273 |
| SHEEP_d29 | 4.5556 | 7.0273 |
| animal_days_GOOSE | 42.1250 | 51.5820 |
| fed_GOOSE | 0.9050 | 0.9015 |
| cared_GOOSE | 0.8865 | 0.9608 |
| collected_fertilizer_GOOSE | 0.9115 | 0.8154 |
| GOOSE_d0 | 0.0833 | 0.0000 |
| GOOSE_d2 | 0.0833 | 0.0000 |
| GOOSE_d5 | 0.1111 | 0.0000 |
| GOOSE_d8 | 0.8056 | 0.0000 |
| GOOSE_d11 | 1.8472 | 2.4023 |
| GOOSE_d17 | 1.9167 | 2.6445 |
| GOOSE_d23 | 1.9722 | 2.6445 |
| GOOSE_d29 | 1.9722 | 2.6445 |
| crop_days | 1466.7917 | 1497.5703 |
| crop_water_rate | 0.7183 | 0.7213 |
| crop_yield_day_maximized | 0.4814 | 0.2081 |
| harvest_WHEAT | 508.4306 | 513.0742 |
| harvest_CARROT | 101.0139 | 75.3203 |
| harvest_TOMATO | 44.8611 | 4.8750 |
| harvest_STRAWBERRY | 227.2917 | 247.9141 |
| harvest_MELON | 78.5000 | 72.0000 |
| hires | 276.0972 | 262.2109 |
| hire_cost | 5206.2639 | 3898.5977 |
| land_buys | 1.9861 | 2.0000 |
| land_cost | 2972.2222 | 3000.0000 |
| weed_digs | 19.0556 | 20.3477 |
| unit_faults | 46.2500 | 13.5820 |
| SELL_weighted_hour | 7.4893 | 10.3812 |
| BUY_PRODUCT_weighted_hour | 6.1469 | 5.7719 |
