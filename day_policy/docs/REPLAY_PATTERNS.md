# Placement in top-player replays

Audit of 22,524 successful establishments in 87 development player-games.
Distance is Manhattan distance to the nearest shed-access tile. Reuse
means that tile was cleared or one-shot harvested earlier the same day.
This placement audit includes games/days excluded from solver tests.

| Stage | Product | Establishments | Mean shed distance | Same-day reuse |
|---|---|---:|---:|---:|
| Opening | wheat | 733 | 5.82 | 0.0% |
| Opening | carrot | 2 | 7.50 | 0.0% |
| Opening | melon | 867 | 3.97 | 0.0% |
| Opening | goose | 9 | 0.67 | 0.0% |
| Opening | cow | 187 | 0.65 | 0.0% |
| Opening | sheep | 179 | 1.55 | 0.0% |
| Early | wheat | 2104 | 5.95 | 50.3% |
| Early | carrot | 25 | 5.92 | 36.0% |
| Early | tomato | 5 | 3.00 | 20.0% |
| Early | strawberry | 1878 | 4.81 | 34.6% |
| Early | melon | 155 | 4.02 | 20.6% |
| Early | goose | 40 | 1.93 | 5.0% |
| Early | cow | 430 | 1.55 | 2.1% |
| Early | sheep | 270 | 2.09 | 1.9% |
| Middle | wheat | 5589 | 5.29 | 80.2% |
| Middle | carrot | 340 | 4.89 | 87.1% |
| Middle | tomato | 324 | 4.19 | 57.1% |
| Middle | strawberry | 893 | 2.92 | 14.6% |
| Middle | melon | 79 | 4.30 | 59.5% |
| Middle | goose | 154 | 3.12 | 68.2% |
| Middle | cow | 46 | 2.96 | 63.0% |
| Middle | sheep | 178 | 2.56 | 58.4% |
| Late | wheat | 4969 | 4.70 | 96.7% |
| Late | carrot | 3060 | 5.05 | 95.4% |
| Late | tomato | 3 | 3.67 | 100.0% |
| Late | sheep | 4 | 3.25 | 0.0% |

## Opening examples

- Majkel (three games): cows at 44/24; sheep at 43/34/42. Six day-0
  melons per game, in the middle, then more on days 1–2. Across these
  games: 18 day-0, 12 day-1 and six day-2 melon establishments.
- Orbital shows almost the same opening. These may be related agent
  styles; their agreement is not independent evidence of optimality.
- ymg starts with two melons per game and more wheat, and adds melons
  later in the game. A large mandatory melon reserve would be too rigid.
- keiz starts with 12 melons and seven outer wheat tiles per game.

These are observations, not recovered source code. They support giving
opening melons middle sites, leaving room for later establishments, and
strongly preferring cleared-site reuse later. They do not establish that
players optimize placements and worker routes jointly, nor that a fixed
animal mask or large permanent melon reserve is necessary.

The policy sees the current dawn and declared jobs only. It must not use
future replay purchases to decide which cells to reserve. Opening logic
is triggered by an empty initial quadrant, without a calendar-day input.
