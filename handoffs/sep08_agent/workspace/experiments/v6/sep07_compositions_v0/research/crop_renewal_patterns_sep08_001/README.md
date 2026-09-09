# Crop renewal learned from recent top-player replays

Source:07:08 top72player-games. All operations come from the replay analyzer's checked successful effects. Counts can include shared strategy families; frequency alone does not establish optimality.

| Crop | Harvest age | Harvested quantity | Fertilized | Instances | Teams | Mean water actions |
| --- | ---: | ---: | --- | ---: | ---: | ---: |
| WHEAT | 4 | 4 | False | 3136 | 11 | 4.21 |
| WHEAT | 3 | 3 | False | 2142 | 12 | 3.41 |
| WHEAT | 2 | 2 | False | 1643 | 12 | 2.69 |
| WHEAT | 4 | 6 | True | 1351 | 8 | 4.08 |
| WHEAT | 3 | 5 | True | 999 | 6 | 3.18 |
| WHEAT | 4 | 5 | True | 235 | 7 | 3.51 |
| CARROT | 3 | 3 | False | 1045 | 10 | 3.19 |
| CARROT | 3 | 4 | True | 768 | 10 | 3.01 |
| CARROT | 2 | 2 | False | 374 | 10 | 2.37 |
| CARROT | 2 | 3 | True | 170 | 7 | 2.19 |
| CARROT | 2 | 1 | False | 89 | 7 | 1.00 |
| CARROT | 3 | 3 | True | 40 | 2 | 2.02 |
| MELON | 10 | 6 | False | 884 | 12 | 9.07 |
| MELON | 10 | 5 | False | 22 | 4 | 7.86 |
| MELON | 11 | 6 | False | 13 | 2 | 8.00 |
| MELON | 10 | 6 | True | 7 | 3 | 7.57 |
| MELON | 12 | 6 | True | 6 | 1 | 10.00 |
| MELON | 12 | 6 | False | 2 | 1 | 8.00 |

Most common post-melon crop calendars (name, harvest/end age, total harvested, any fertilizer):
- 74 tiles across 6 teams: (('WHEAT', 3, 3, False), ('WHEAT', 4, 4, False), ('WHEAT', 4, 4, False)).
- 57 tiles across 7 teams: (('WHEAT', 3, 3, False), ('WHEAT', 3, 3, False), ('WHEAT', 4, 4, False)).
- 56 tiles across 6 teams: (('WHEAT', 2, 2, False), ('WHEAT', 4, 4, False), ('WHEAT', 4, 4, False)).
- 39 tiles across 3 teams: (('WHEAT', 4, 6, True), ('WHEAT', 4, 6, True), ('WHEAT', 4, 6, True)).
- 22 tiles across 6 teams: (('WHEAT', 4, 4, False), ('WHEAT', 4, 4, False), ('WHEAT', 3, 3, False)).
- 19 tiles across 2 teams: (('WHEAT', 3, 5, True), ('WHEAT', 3, 5, True), ('WHEAT', 3, 5, True)).
- 16 tiles across 3 teams: (('WHEAT', 4, 4, False), ('WHEAT', 4, 4, False), ('WHEAT', 4, 4, False)).
- 15 tiles across 2 teams: (('WHEAT', 4, 6, True), ('CARROT', 3, 4, True), ('CARROT', 3, 4, True)).

ANALYSIS.json stores exact episode/seat/tile calendars for each bucket and motif, plus transition gaps. Use them as proposals, then account for input cost, future prices and worker feasibility.
