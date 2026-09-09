# Fresh global replay and notebook review

Snapshot: **2026-09-07 13:43:00 UTC**. Reviewed all current top 12 teams, taking six recent completed games from each team's highest public-score submission returned by the API. This is 72 selected player-games and 58 unique raw replays. Exact offline action reconstruction reconciles all selected cash outcomes: 56,985 transactions, 16,152 crop instances and 1,197 animal instances. No new gameplay policy or official submission was created.

Global order at selection: 3정훈, get some fries, ymg_aq, SpaTaro, Mengfei Li, Suliman Tadros, 自己找差距, binghua, Crop Dusta, Atakan Aldemir, Jiro2, Bohann Wang. `top_replay_manifest.csv` retains exact ranks, scores, submission IDs, dates and episodes. Selection may use a stronger earlier submission rather than the team's latest uploaded file. `REFRESH.json` records retrieval/analysis commands and raw replay hashes.

## First priority: a larger observed sheep-expansion suffix

自己找差距, submission 56068263, episodes **106458227** and **106463571**, supplies a two-course family change with **288 identical raw and normalized actions**. At step 288/day 12, its own physical farm matches exactly except cash ($11,950 versus $12,444); private state differs by shed wheat (55 versus 65). Public shops and market state differ. Both already hold nine cows and eight sheep before the next purchase.

At step 289, the first course buys one sheep with one Yarn Store revealed (wool demand two/cycle); the other buys four with three Yarns (wool demand six/cycle). The second course continues sheep batches on days 14,15,16 and later. This is not merely one animal substitution:

| Observed whole-game quantity | Smaller course | Expansion course |
|---|---:|---:|
| Total sheep purchases | 9 | 27 |
| Sheep-days | 198 | 452 |
| Wool sales | 226 | 486 |
| Wheat harvest | 521 | 344 |
| Wheat purchases | 205 | 407 |
| Tomato harvest | 54 | 32 |
| Strawberry harvest | 264 | 218 |
| Hires | 262 | 282 |
| Labor cost | $3,947 | $6,764 |
| Final cash | $95,329 | $128,740 |

The different shop/opponent realizations prevent causal interpretation of the cash difference. More observed Yarn demand is a simple legal-feature hypothesis consistent with these two cases, not a recovered private formula. Existing own output, market inventory, future labor and rival flow may also matter.

`sheep_expansion/{small,expansion}/` preserves both full 719-action raw/normalized courses, exact day-12 observations, successful service/trade calendars, crop/animal lifetimes, tile transitions, donor metadata and hashes. `COMPARISON.json` lists exact state differences. The expansion builds pastures on exhausted wheat/melon cells, converts a former empty coop, and later uses pastures after cows have escaped. Three later sheep placements at steps 465/474/475 reuse existing cow pastures after state456. Do not infer that escape was intentionally planned merely from this observation.

Implementation dependencies: preserve the full crop-to-animal suffix, land and seed changes, wheat funding/pickup/feed, all sheep service and fertilizer collection, harvest/deposit/sales, and worker schedules. Compare both fixed courses first, then legal observation-driven selection. Prefix equality does not certify financial or inventory parity. This family is a useful independent portfolio candidate beyond the current one-additional-animal compiler.

## Second priority: productive wheat within a complete crop rotation

Current global rank2 **get some fries**, submission **56048814**, averages **5.0395 harvested wheat per seed and 1.0200 per occupied wheat-day**, versus the local reference's 3.1656 and 0.7555. This is an efficiency distinction; its total wheat output is 509.8/game versus local 515.8, so the observation does not imply it simply grows more wheat.

Across its six games, 123 wheat lives reach six output using WATER ages **0,2,3,4**, FERTILIZE age **2 before productive WATER**, and harvest age4. Another 168 reach six with an extra age1 water. The latter may be avoidable service, but no exact removal/route ablation has been run here.

Concrete full-cell example: episode **106441638**, cell **(0,3)**:

- CARROT days0→3, output3.
- MELON days3→13, output6, no fertilizer.
- WHEAT days13→17, output6; FERT step378/day15/hour18, WATER step379, subsequent WATER steps399/417, HARVEST418.
- WHEAT days17→21, output6; then21→25, output6; then25→29, output5.

This donor also staggers melon starts across days0,3,4,5,8,9,10 instead of keeping only the usual initial twelve. It averages 20.33 melon plantings and 118.83 harvested melons/game, versus local12 and72. An example at cell(2,2) is CARROT0→3, MELON3→13, WHEAT18→22, WHEAT22→26, CARROT26→29; the five-day gap is retained, not silently filled.

Fertilizer is not free: this player applies 140.83/game, buys152.17 and sells318, with animal collection supplying the remaining balance. The current reference applies substantially less and has only50 fertilizer purchases. Full crop-family evaluation must price displaced fertilizer sales, feed replacement, seed/land cost, transport and labor, rather than compare output per seed alone. Crops' harvested output is measured from successful HARVEST events, not gross market sales.

## Third priority: another complete tomato block

Current rank1 **3정훈**, submission **56063995**, episode **106446230**, cell **(1,3)** provides MELON0→10=6, WHEAT10→14=4, TOMATO14→25=8, WHEAT25→29=4. Tomato fertilizer is at steps513/day21 and587/day24; harvests steps536/561/589/606 collect two each on days22–25, then DIG607. This confirms a tomato family in a second globally strong donor, in addition to the earlier Mengfei block. It is an uncompiled proposal, not a proven profitable replacement in our farm.

`larger_crop_examples.json` contains these exact tile lifecycles, service addresses, raw replay hashes, seats and complete donor day actions. `crop_lifecycles.json` and `crop_service_templates.json` provide the broader support. Adjacent worker days and market plans must be rebuilt; the donor's whole-day actions are evidence and are not directly transferable into a different farm.

## Global comparison and animal null findings

The fresh 72-game cohort versus unchanged `investment_context_guarded_001_best` profile64: crop occupancy1493.90 versus1507.44 days; water rate73.19% versus72.48%; nominal productive fertilizer coverage40.18% versus19.70%. Wheat output523.54 versus515.80, carrot97.67 versus81.95, tomato33.63 versus0, strawberry224.67 versus248.92, melon78.64 versus72. Local labor is already lower ($3,705 versus$5,226), as are unit faults12.08 versus59.11 and discards4.80 versus13.18. These are approximate targets across different environments, not paired superiority measurements. Preserve the earlier crop-mix correction: local strawberry fertilizer coverage is already90.85%; much of the actionable within-crop gap is wheat/carrot.

`animal_decisions/` has 952 animal orders, 1,197 placements and147 same-player purchase-pair candidates. Five fresh Atakan step226 cases repeat the old pattern: cow when milk demand is2or3 and no Yarn, goose when milk demand is1 and no Yarn. They do not provide a new sheep or waiting test. Two get-some-fries games have identical revealed shops but a one-step purchase timing difference and materially different cash; this is not evidence of deliberate information-seeking waiting. No profitable waiting rule is established by this refresh.

## Public notebook audit

Only two notebook metadata changes were found relative to refresh1212:

- Updated `flexonafft/kaggriculture-smart-farm-strategy-lab` (12:17). Statically decoded `MAIN_B64` SHA-256 **2b97e2c653018ac4aeffb8463ec91c8f26b097b4cef81289acc25ccdbc68f916** exactly equals the already ported `public_capacity_router` source. No distinct agent or new port is needed.
- New `rogerrogerroger3r/kaggriculture-a-ruler-for-agent-changes` (13:16). An evaluation/compiler notebook that explicitly contains no agent. It adds no gameplay component to port. Its external claims were not independently verified or adopted as rules; the experiment already uses same-seed both-seat comparisons and separate fresh audits.

Notebook cells were read/static-decoded only. Original notebooks, metadata, extracted source, notes, pull logs and hashes are retained. No cell execution or new installation occurred.

## Reproduction

Run `refresh.py` for a new network snapshot only in a new directory; rerunning it here would overwrite this evidence. The completed commands are in `REFRESH.json`. Offline products were made by `scripts/review.py --research-dir research/refresh_1342` (with the full experiment-relative path), then `animal_audit.py`, `crop_templates.py`, and `export_sheep_expansion.py`, all through `conda run -n kaggriculture`. The local helper scripts derive paths from their own locations and do not modify policy headers or the league catalog. `FINAL_AUDIT.json` hashes final artifacts and records scope.
