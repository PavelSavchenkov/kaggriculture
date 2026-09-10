# Public sales and purchase patterns

This descriptive sample contains 48 selected player-games in 40 episodes. Selection, source IDs and replay names are in SELECTED_SEATS.json; prospective data collection is in PROTOCOL.json. These episodes have already been used. They are not a fresh test for subsequent policy changes, and different team names do not prove independent code families.

Counts use C++ reconstructed execution and stock after worker actions. Cash and market inventory match every recorded turn. An output product-turn means one of carrot, tomato, strawberry, melon, egg, milk or wool in one turn. Several products in one turn count separately. The ordering comparison includes wheat and fertilizer and uses quotes and stock at the start of market execution.

| Team | Output sale / stock product-turns | Waits with ten successful orders / all waits | Ineffective / requested orders |
| --- | ---: | ---: | ---: |
| SpaTaro | 495 / 3181 | 47 / 2686 | 781 / 3377 |
| Otter Vibe | 452 / 468 | 16 / 16 | 0 / 1784 |
| Himanshu Kumar | 421 / 2367 | 93 / 1946 | 339 / 2830 |
| Mengfei Li | 295 / 416 | 121 / 121 | 87 / 1834 |
| cooked | 439 / 2282 | 89 / 1843 | 630 / 3189 |
| pensukesan | 667 / 2361 | 84 / 1694 | 576 / 3286 |
| Unknown Mother-Goose | 478 / 4730 | 198 / 4252 | 42 / 2303 |
| mtmr_s1 | 427 / 2496 | 122 / 2069 | 136 / 2602 |
| binghua | 424 / 424 | 0 / 0 | 3 / 2320 |
| que la cuenten como quieran | 428 / 1840 | 84 / 1412 | 605 / 3064 |
| Squirrel | 436 / 2246 | 69 / 1810 | 0 / 2447 |
| supr3mum | 402 / 2276 | 55 / 1874 | 848 / 3286 |
| Tarang222 | 324 / 324 | 0 / 0 | 3 / 1877 |
| 自己找差距 | 300 / 397 | 97 / 97 | 76 / 1770 |
| Terry Luo | 424 / 2345 | 84 / 1921 | 674 / 3113 |
| DeeperNet | 524 / 1785 | 93 / 1261 | 511 / 3114 |

## What these observations support

- Otter Vibe: 16 output waits; 16 have ten successful orders, 0 have an unused or ineffective order slot. Quantity times current quote explains 200/200 multi-product sale turns; quote alone explains 109/200.
- binghua: 0 output waits; 0 have ten successful orders, 0 have an unused or ineffective order slot. Quantity times current quote explains 24/144 multi-product sale turns; quote alone explains 57/144.
- Unknown Mother-Goose: 4252 output waits; 198 have ten successful orders, 4054 have an unused or ineffective order slot. Quantity times current quote explains 66/140 multi-product sale turns; quote alone explains 73/140.
- Squirrel: 1810 output waits; 69 have ten successful orders, 1741 have an unused or ineffective order slot. Quantity times current quote explains 77/141 multi-product sale turns; quote alone explains 73/141.

The sample contains both rapid sellers and frequent holders among strong teams. A full successful order list explains the immediate reason some products cannot be sold that turn. A wait with a free slot has another cause, but the replay alone does not reveal whether it is intentional, profitable or forced by a broader schedule. These observations propose tests; they do not establish optimality or the author's reasoning.

For our pipeline, compare the same funded farm and delivery calendar under alternative orders. Count stock capacity, required input deadlines, available order positions, current rival production and possible already harvested rival stock. Then check complete games and shared market effects after the edited window. A sales-friendly farm permits useful timing choices; an observed gain from repairing its existing orders is not proof of a better composition.

Team names can change: where episode metadata is supplied, stable submission and team IDs determine the seat. The original replay name remains recorded. Current source-program identity is stronger evidence than a name match, but it still does not establish code-family independence.

Reproduction: scripts/summarize_patterns.py builds BY_TEAM.json and SELECTED_SEATS.json; scripts/report_public_patterns.py builds this report and BEHAVIOR_DETAILS.json. All repeated financial simulation runs in C++.
