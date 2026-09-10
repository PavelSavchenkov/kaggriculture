# Public sales and purchase patterns

This descriptive sample contains 48 selected player-games in 38 episodes. Selection, source IDs and replay names are in SELECTED_SEATS.json; prospective data collection is in PROTOCOL.json. These episodes have already been used. They are not a fresh test for subsequent policy changes, and different team names do not prove independent code families.

Counts use C++ reconstructed execution and stock after worker actions. Cash and market inventory match every recorded turn. An output product-turn means one of carrot, tomato, strawberry, melon, egg, milk or wool in one turn. Several products in one turn count separately. The ordering comparison includes wheat and fertilizer and uses quotes and stock at the start of market execution.

| Team | Output sale / stock product-turns | Waits with ten successful orders / all waits | Ineffective / requested orders |
| --- | ---: | ---: | ---: |
| SpaTaro | 418 / 2288 | 46 / 1870 | 838 / 3314 |
| Otter Vibe | 463 / 505 | 42 / 42 | 0 / 1859 |
| Himanshu Kumar | 428 / 2454 | 93 / 2026 | 338 / 2826 |
| Mengfei Li | 307 / 443 | 136 / 136 | 90 / 1831 |
| cooked | 441 / 2451 | 91 / 2010 | 624 / 3199 |
| デワンシュ | 419 / 2620 | 61 / 2201 | 806 / 3270 |
| pensukesan | 611 / 2363 | 101 / 1752 | 518 / 3113 |
| mtmr_s1 | 390 / 1971 | 113 / 1581 | 95 / 2560 |
| Unknown Mother-Goose | 452 / 4881 | 190 / 4429 | 36 / 2217 |
| binghua | 446 / 446 | 0 / 0 | 2 / 2322 |
| que la cuenten como quieran | 421 / 2210 | 85 / 1789 | 615 / 3064 |
| Christoffer Thimsen | 448 / 2437 | 97 / 1989 | 391 / 2918 |
| Squirrel | 487 / 2739 | 86 / 2252 | 0 / 2457 |
| DeeperNet | 527 / 1756 | 93 / 1229 | 516 / 3124 |
| Terry Luo | 379 / 2068 | 74 / 1689 | 617 / 2950 |
| feel the agi | 356 / 405 | 49 / 49 | 35 / 2010 |

## What these observations support

- Otter Vibe: 42 output waits; 42 have ten successful orders, 0 have an unused or ineffective order slot. Quantity times current quote explains 211/211 multi-product sale orders; quote alone explains 91/211.
- binghua: 0 output waits; 0 have ten successful orders, 0 have an unused or ineffective order slot. Quantity times current quote explains 31/148 multi-product sale orders; quote alone explains 57/148.
- Unknown Mother-Goose: 4429 output waits; 190 have ten successful orders, 4239 have an unused or ineffective order slot. Quantity times current quote explains 75/129 multi-product sale orders; quote alone explains 73/129.
- Squirrel: 2252 output waits; 86 have ten successful orders, 2166 have an unused or ineffective order slot. Quantity times current quote explains 72/144 multi-product sale orders; quote alone explains 61/144.
- feel the agi: 49 output waits; 49 have ten successful orders, 0 have an unused or ineffective order slot. Quantity times current quote explains 158/180 multi-product sale orders; quote alone explains 102/180.

The sample contains both rapid sellers and frequent holders among strong teams. A full successful order list explains the immediate reason some products cannot be sold that turn. A wait with a free slot has another cause, but the replay alone does not reveal whether it is intentional, profitable or forced by a broader schedule. These observations propose tests; they do not establish optimality or the author's reasoning.

For our pipeline, compare the same funded farm and delivery calendar under alternative orders. Count stock capacity, required input deadlines, available order positions, current rival production and possible already harvested rival stock. Then check complete games and shared market effects after the edited window. A sales-friendly farm permits useful timing choices; an observed gain from repairing its existing orders is not proof of a better composition.

Team names can change: where episode metadata is supplied, stable submission and team IDs determine the seat. The original replay name remains recorded. Current source-program identity is stronger evidence than a name match, but it still does not establish code-family independence.

Reproduction: scripts/summarize_patterns.py builds BY_TEAM.json and SELECTED_SEATS.json; scripts/report_public_patterns.py builds this report and BEHAVIOR_DETAILS.json. All repeated financial simulation runs in C++.
