# Public sales patterns — 04:23 UTC snapshot

The sample contains 38 episodes and 48 selected player appearances from the top 16 teams. Four episodes were already exposed; all 34 new episodes were evaluated with the frozen sale-timing rule before this descriptive profile. The full extracted corpus now contains 297 exposed episodes.

| Selected agent | Observed behavior in its three games |
| --- | --- |
| Otter Vibe | All 45 waiting product-turns coincide with ten successful orders. All 198 multi-product sale turns follow descending quantity times current quote; 95 also follow quote alone. |
| binghua | Sells in all 431 output-stock observations. |
| feel the agi | All 51 waiting product-turns coincide with ten successful orders. |
| Mengfei Li | All 113 waiting product-turns coincide with ten successful orders. |
| 自己找差距 | All 76 waiting product-turns coincide with ten successful orders. |
| Squirrel | 2,212 waiting product-turns, including 2,124 with unused or failed order capacity. |
| Unknown Mother-Goose | 3,608 waiting product-turns, including 3,423 with unused or failed order capacity. |
| kanno | 1,274 waiting product-turns, including 1,184 with unused or failed order capacity. |

A waiting product-turn means a nonbuyable output product is in the shed after worker actions but none is sold that turn. One turn may contain several such products. Counts describe actual executed trades, not requests. These patterns establish neither optimality nor the authors' reasons.

The native C++ miner checks cash and market transitions and takes 0.251 seconds for these 38 episodes; replay acquisition, extraction and Python reporting are outside that time. Selected seats resolve uniquely by replay team name in this sample; stable submission and team IDs are recorded as provenance.

The notebook metadata now lists a new run of Georgy Mamarin's `Kaggriculture Daily Replays: The Live Meta Report`, timestamp September 10, 04:07:43 UTC. Source inspection is recorded separately under `research/notebook_live_meta_0425`. A new notebook run or selected program ID does not establish an independent strategy family or a replicated performance claim.

Evidence: `PROTOCOL.json`, `SELECTED_SEATS.json`, `BY_TEAM.json`, `BEHAVIOR_DETAILS.json`, `TIMING.json`, and `research/refresh_sep10_0425` relative to the experiment root.
