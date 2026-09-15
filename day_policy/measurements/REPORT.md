# Final day policy measurements

Default: Balanced-4 with Staged placement. Broader option: Full-8.
Hires exclude the farmer; late means days 20–28. Native GCC 13.3, O3,
native CPU tuning and LTO; this release uses no PGO. Each selected-profile
time combines two sequential runs pinned to CPU14. Times include failed
calls and hire minimization, and exclude the evaluator’s second verification.
Other machine activity was not controlled. See ../docs/TESTING.md for
cohort construction, exclusions and the geometry-test limitations.

## Exact original dawn layouts; our placement for new products

| Cohort | Pipeline | Cap | Solved | Late | Median / mean ms | Mean hires on success |
|---|---|---:|---:|---:|---:|---:|
| dev | balanced4 | 11 | 860/914 | 54/83 | 1.11 / 6.02 | 6.08 |
| dev | full8 | 11 | 882/914 | 67/83 | 0.74 / 25.98 | 6.43 |
| dev | balanced4 | 13 | 911/914 | 83/83 | 1.11 / 8.00 | 6.42 |
| dev | full8 | 13 | 911/914 | 83/83 | 0.74 / 33.89 | 6.61 |
| 639 | balanced4 | 11 | 609/639 | 48/58 | 1.17 / 6.03 | 6.18 |
| 639 | full8 | 11 | 621/639 | 53/58 | 0.78 / 26.47 | 6.50 |
| 639 | balanced4 | 13 | 636/639 | 58/58 | 1.17 / 7.55 | 6.43 |
| 639 | full8 | 13 | 636/639 | 58/58 | 0.78 / 32.43 | 6.63 |
| 645 | balanced4 | 11 | 614/645 | 47/57 | 1.16 / 6.35 | 6.17 |
| 645 | full8 | 11 | 625/645 | 53/57 | 0.82 / 25.41 | 6.49 |
| 645 | balanced4 | 13 | 641/645 | 57/57 | 1.17 / 8.28 | 6.43 |
| 645 | full8 | 13 | 641/645 | 57/57 | 0.81 / 34.17 | 6.63 |

## Our placement carried from game start: Balanced-4

The full original eligible denominator is retained, including mapping failures.
Timing covers calls actually made. Hour-23 results relax deadlines and do
not establish early-return coverage. Legacy rows use one timing run.

| Cohort | Returns | Cap | Legacy solved | Staged solved | Staged late | Median / mean ms |
|---|---|---:|---:|---:|---:|---:|
| dev | Strict | 11 | 728/914 | 807/914 | 27/83 | 0.93 / 5.69 |
| dev | Strict | 13 | 766/914 | 859/914 | 63/83 | 0.93 / 9.56 |
| dev | Hour 23 | 11 | 882/914 | 876/914 | 56/83 | 1.03 / 5.22 |
| dev | Hour 23 | 13 | 911/914 | 912/914 | 83/83 | 1.05 / 6.58 |
| 639 | Strict | 11 | 508/639 | 570/639 | 25/58 | 0.94 / 5.75 |
| 639 | Strict | 13 | 530/639 | 596/639 | 44/58 | 0.94 / 9.03 |
| 639 | Hour 23 | 11 | 613/639 | 610/639 | 41/58 | 1.10 / 4.77 |
| 639 | Hour 23 | 13 | 636/639 | 635/639 | 56/58 | 1.10 / 5.43 |
| 645 | Strict | 11 | 510/645 | 574/645 | 29/57 | 0.94 / 6.08 |
| 645 | Strict | 13 | 540/645 | 604/645 | 44/57 | 0.94 / 8.70 |
| 645 | Hour 23 | 11 | 611/645 | 617/645 | 41/57 | 1.11 / 5.60 |
| 645 | Hour 23 | 13 | 644/645 | 644/645 | 57/57 | 1.12 / 6.16 |

## Hires on days completed by both placement policies, cap 13

Shared solved days only, with all returns due at hour 23. This avoids
comparing hire averages on different sets of successful days.

| Cohort | Shared days | Legacy mean hires | Staged mean hires | Total hires saved |
|---|---:|---:|---:|---:|
| dev | 911 | 6.36 | 6.23 | 118 |
| 639 | 635 | 6.36 | 6.26 | 66 |
| 645 | 644 | 6.41 | 6.30 | 69 |

## Per player, cap 11: four comparison games per player

| Player | Days / late | Original B4 solved / late | Original Full solved / late | B4 median / mean ms | Full median / mean ms | Own strict B4 solved / late | Own hour-23 B4 solved / late |
|---|---:|---:|---:|---:|---:|---:|---:|
| Majkel1337 | 111 / 33 | 88 / 22 | 101 / 29 | 9.36 / 19.99 | 7.23 / 83.44 | 72 / 12 | 89 / 21 |
| ymg_aq | 58 / 8 | 55 / 6 | 57 / 8 | 1.33 / 9.21 | 1.00 / 49.59 | 54 / 5 | 56 / 6 |
| SpaTaro | 69 / 11 | 61 / 11 | 63 / 11 | 3.08 / 10.00 | 2.48 / 52.89 | 47 / 7 | 67 / 11 |
| Mengfei Li | 28 / 0 | 28 / 0 | 28 / 0 | 1.05 / 1.24 | 0.56 / 0.85 | 28 / 0 | 28 / 0 |
| Orbital Terraformer | 108 / 31 | 97 / 27 | 100 / 27 | 5.16 / 13.50 | 4.08 / 59.21 | 73 / 13 | 89 / 17 |
| Otter Vibe | 43 / 0 | 41 / 0 | 41 / 0 | 0.72 / 3.46 | 0.48 / 15.99 | 42 / 0 | 42 / 0 |
| redblackbst | 33 / 3 | 33 / 3 | 33 / 3 | 1.05 / 1.82 | 0.60 / 1.29 | 32 / 2 | 33 / 3 |
| feel the agi | 36 / 2 | 36 / 2 | 36 / 2 | 1.17 / 2.05 | 0.60 / 1.35 | 36 / 2 | 36 / 2 |
| Catalyst | 28 / 0 | 28 / 0 | 28 / 0 | 0.96 / 1.50 | 0.57 / 1.13 | 28 / 0 | 28 / 0 |
| Thomas Tschinkel | 32 / 0 | 32 / 0 | 32 / 0 | 1.05 / 1.75 | 0.59 / 1.25 | 32 / 0 | 32 / 0 |
| HowardLeeTW | 49 / 0 | 49 / 0 | 49 / 0 | 0.98 / 2.73 | 0.67 / 1.73 | 46 / 0 | 48 / 0 |
| Artem The Farmer 🍅 | 52 / 0 | 48 / 0 | 48 / 0 | 2.18 / 6.77 | 1.74 / 61.41 | 48 / 0 | 52 / 0 |
| アルモンド | 26 / 4 | 26 / 4 | 26 / 4 | 0.95 / 1.61 | 0.57 / 1.02 | 24 / 3 | 24 / 3 |
| 𝕯𝖊𝖔𝖉𝖎𝖒𝖘 & 𝕮𝖔 | 33 / 1 | 33 / 1 | 33 / 1 | 0.95 / 1.46 | 0.57 / 1.03 | 33 / 1 | 33 / 1 |
| leave you | 36 / 0 | 36 / 0 | 36 / 0 | 1.05 / 2.06 | 0.60 / 1.45 | 36 / 0 | 36 / 0 |
| keiz | 67 / 17 | 63 / 15 | 66 / 17 | 3.29 / 10.22 | 2.42 / 41.30 | 49 / 8 | 65 / 16 |
| Cow Boy | 32 / 2 | 31 / 1 | 31 / 1 | 1.01 / 4.06 | 0.59 / 15.95 | 29 / 0 | 30 / 0 |
| Zhenghongshuang | 28 / 0 | 28 / 0 | 28 / 0 | 0.96 / 1.53 | 0.58 / 1.12 | 28 / 0 | 28 / 0 |
| THIRD FARM CLUB | 41 / 0 | 36 / 0 | 36 / 0 | 1.26 / 10.01 | 1.02 / 69.00 | 35 / 0 | 38 / 0 |
| AI是我的豆包 | 28 / 0 | 28 / 0 | 28 / 0 | 0.96 / 1.47 | 0.57 / 1.08 | 28 / 0 | 28 / 0 |
| Kaggriculture Agent | 35 / 0 | 35 / 0 | 35 / 0 | 1.05 / 1.66 | 0.60 / 1.20 | 35 / 0 | 35 / 0 |
| Knight of Favonius | 30 / 0 | 30 / 0 | 30 / 0 | 0.96 / 1.86 | 0.57 / 1.27 | 30 / 0 | 30 / 0 |
| THUNDER THUNDER | 56 / 0 | 56 / 0 | 56 / 0 | 1.29 / 2.26 | 0.81 / 1.59 | 56 / 0 | 56 / 0 |
| Phoenix750 | 38 / 0 | 38 / 0 | 38 / 0 | 0.97 / 1.98 | 0.60 / 1.37 | 38 / 0 | 38 / 0 |
| Jeryos | 29 / 0 | 29 / 0 | 29 / 0 | 0.96 / 1.53 | 0.58 / 1.11 | 29 / 0 | 29 / 0 |
| nilochan | 35 / 3 | 35 / 3 | 35 / 3 | 1.05 / 3.12 | 0.60 / 8.18 | 33 / 1 | 34 / 2 |
| nofreewill42 | 30 / 0 | 30 / 0 | 30 / 0 | 0.96 / 1.63 | 0.57 / 1.15 | 30 / 0 | 30 / 0 |
| lucaskna | 30 / 0 | 30 / 0 | 30 / 0 | 0.96 / 1.45 | 0.58 / 1.05 | 30 / 0 | 30 / 0 |
| Emile Andrieu | 32 / 0 | 32 / 0 | 32 / 0 | 1.00 / 1.91 | 0.59 / 1.32 | 32 / 0 | 32 / 0 |
| Zhongyi Dai | 31 / 0 | 31 / 0 | 31 / 0 | 0.96 / 1.65 | 0.58 / 1.18 | 31 / 0 | 31 / 0 |

## Per player, cap 13: four comparison games per player

| Player | Days / late | Original B4 solved / late | Original Full solved / late | B4 median / mean ms | Full median / mean ms | Own strict B4 solved / late | Own hour-23 B4 solved / late |
|---|---:|---:|---:|---:|---:|---:|---:|
| Majkel1337 | 111 / 33 | 111 / 33 | 111 / 33 | 9.35 / 24.23 | 7.23 / 96.27 | 91 / 24 | 111 / 33 |
| ymg_aq | 58 / 8 | 58 / 8 | 58 / 8 | 1.32 / 9.94 | 1.01 / 51.52 | 58 / 8 | 58 / 8 |
| SpaTaro | 69 / 11 | 69 / 11 | 69 / 11 | 3.07 / 12.99 | 2.46 / 67.17 | 47 / 7 | 69 / 11 |
| Mengfei Li | 28 / 0 | 28 / 0 | 28 / 0 | 1.05 / 1.24 | 0.56 / 0.85 | 28 / 0 | 28 / 0 |
| Orbital Terraformer | 108 / 31 | 108 / 31 | 108 / 31 | 5.16 / 16.57 | 4.08 / 67.82 | 90 / 24 | 107 / 30 |
| Otter Vibe | 43 / 0 | 43 / 0 | 43 / 0 | 0.71 / 4.82 | 0.48 / 20.54 | 43 / 0 | 42 / 0 |
| redblackbst | 33 / 3 | 33 / 3 | 33 / 3 | 1.05 / 1.82 | 0.60 / 1.29 | 33 / 3 | 33 / 3 |
| feel the agi | 36 / 2 | 36 / 2 | 36 / 2 | 1.17 / 2.05 | 0.60 / 1.35 | 36 / 2 | 36 / 2 |
| Catalyst | 28 / 0 | 28 / 0 | 28 / 0 | 0.95 / 1.50 | 0.57 / 1.09 | 28 / 0 | 28 / 0 |
| Thomas Tschinkel | 32 / 0 | 32 / 0 | 32 / 0 | 1.01 / 1.73 | 0.59 / 1.25 | 32 / 0 | 32 / 0 |
| HowardLeeTW | 49 / 0 | 49 / 0 | 49 / 0 | 0.98 / 2.73 | 0.67 / 1.73 | 46 / 0 | 48 / 0 |
| Artem The Farmer 🍅 | 52 / 0 | 48 / 0 | 48 / 0 | 2.18 / 18.07 | 1.74 / 129.59 | 48 / 0 | 52 / 0 |
| アルモンド | 26 / 4 | 26 / 4 | 26 / 4 | 0.95 / 1.62 | 0.57 / 1.02 | 24 / 3 | 24 / 3 |
| 𝕯𝖊𝖔𝖉𝖎𝖒𝖘 & 𝕮𝖔 | 33 / 1 | 33 / 1 | 33 / 1 | 0.95 / 1.46 | 0.57 / 1.03 | 33 / 1 | 33 / 1 |
| leave you | 36 / 0 | 36 / 0 | 36 / 0 | 1.05 / 2.06 | 0.60 / 1.45 | 36 / 0 | 36 / 0 |
| keiz | 67 / 17 | 67 / 17 | 67 / 17 | 3.25 / 10.81 | 2.42 / 42.00 | 56 / 11 | 67 / 17 |
| Cow Boy | 32 / 2 | 32 / 2 | 32 / 2 | 1.00 / 4.11 | 0.59 / 15.98 | 31 / 2 | 32 / 2 |
| Zhenghongshuang | 28 / 0 | 28 / 0 | 28 / 0 | 0.95 / 1.53 | 0.57 / 1.12 | 28 / 0 | 28 / 0 |
| THIRD FARM CLUB | 41 / 0 | 38 / 0 | 38 / 0 | 1.27 / 21.68 | 1.01 / 123.23 | 38 / 0 | 41 / 0 |
| AI是我的豆包 | 28 / 0 | 28 / 0 | 28 / 0 | 0.96 / 1.47 | 0.57 / 1.08 | 28 / 0 | 28 / 0 |
| Kaggriculture Agent | 35 / 0 | 35 / 0 | 35 / 0 | 1.05 / 1.66 | 0.60 / 1.21 | 35 / 0 | 35 / 0 |
| Knight of Favonius | 30 / 0 | 30 / 0 | 30 / 0 | 0.96 / 1.86 | 0.58 / 1.27 | 30 / 0 | 30 / 0 |
| THUNDER THUNDER | 56 / 0 | 56 / 0 | 56 / 0 | 1.29 / 2.26 | 0.81 / 1.59 | 56 / 0 | 56 / 0 |
| Phoenix750 | 38 / 0 | 38 / 0 | 38 / 0 | 0.96 / 1.99 | 0.61 / 1.36 | 38 / 0 | 38 / 0 |
| Jeryos | 29 / 0 | 29 / 0 | 29 / 0 | 0.96 / 1.53 | 0.58 / 1.10 | 29 / 0 | 29 / 0 |
| nilochan | 35 / 3 | 35 / 3 | 35 / 3 | 1.05 / 3.13 | 0.60 / 8.18 | 35 / 3 | 35 / 3 |
| nofreewill42 | 30 / 0 | 30 / 0 | 30 / 0 | 0.96 / 1.63 | 0.57 / 1.15 | 30 / 0 | 30 / 0 |
| lucaskna | 30 / 0 | 30 / 0 | 30 / 0 | 0.96 / 1.50 | 0.57 / 1.05 | 30 / 0 | 30 / 0 |
| Emile Andrieu | 32 / 0 | 32 / 0 | 32 / 0 | 1.01 / 1.91 | 0.59 / 1.32 | 32 / 0 | 32 / 0 |
| Zhongyi Dai | 31 / 0 | 31 / 0 | 31 / 0 | 0.98 / 1.65 | 0.58 / 1.18 | 31 / 0 | 31 / 0 |

## Original-hire-filtered comparisons

Only days with successful original hires <= the tested cap. Cap 10 and
cap 11 have different denominators. The JSON includes every player.

| Cap | Pipeline | Solved | Late | Median / mean ms |
|---:|---|---:|---:|---:|
| 10 | balanced4 | 971/975 | 16/19 | 0.96 / 1.74 |
| 10 | full8 | 972/975 | 17/19 | 0.57 / 2.14 |
| 11 | balanced4 | 1145/1183 | 73/88 | 1.06 / 4.84 |
| 11 | full8 | 1162/1183 | 80/88 | 0.61 / 16.22 |

## Placement cost and remaining limits

The isolated placement measurement includes copying the compiled plan and
a result checksum, but excludes job compilation. It is an upper estimate
of the placement function alone; whole-solver times above are primary.

- dev, days with new products: 1.72 / 1.99 microseconds median / mean.
- dev: impossible inherited non-wheat return bounds: 144 Legacy, 43 Staged.
- 639, days with new products: 1.72 / 1.95 microseconds median / mean.
- 639: impossible inherited non-wheat return bounds: 103 Legacy, 29 Staged.
- 645, days with new products: 1.68 / 1.94 microseconds median / mean.
- 645: impossible inherited non-wheat return bounds: 100 Legacy, 33 Staged.

The changed-grid test still preserves recorded prefix service outcomes;
the 639 cohort has two missing clearing targets and two cases blocked by
an excluded prefix with multiple land purchases. All stay in the denominator.
This is not a fully executed economic rollout. No minimum-worker proof or
full-agent win-rate claim follows from these results. Final integration
must plan returns against its actual resulting placement and state.

An additional diagnostic Full-8 call solved the sole remaining 645-cohort
hour-23 case at 13 hires (Otter Vibe, episode 109059154, seat 1, day 15,
144 requested field operations). This does not change the Balanced tables
or establish the runtime of a chained pipeline. No policy was tuned on it.
