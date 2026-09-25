# Analysis tools of the weakness study

Copied from `work/sep25_bc_weakness/` (a second session that analysed the same agents on Sep 25;
its findings are in `../docs/weakness_analysis/`). The C++ tools read engine traces; the Python queries
are the exact scripts behind the numbers in those documents.

## Build

```bash
conda run -n kaggriculture cmake -S pipeline_sep25_handoff/analysis -B work/p25_analysis -DCMAKE_BUILD_TYPE=Release
conda run -n kaggriculture cmake --build work/p25_analysis -j 8
```

The wrapper builds the experiment through `add_subdirectory` (hand-linking against its static
libraries crashed or changed decisions). `forecast_audit.cpp` here differs from the experiment's tool
of the same name and builds as `forecast_audit_ops`. Verified: all 13 tools build; `ledger` runs on
`../traces/cpp_games/*.trace.gz` (gunzip first).

## C++ tools

| Tool | What it does |
|---|---|
| `cohort_audit.cpp` | Spatial dispersion of same-day cohorts: for each seat and day, the tiles planted that day (per crop type and all crops) and the animals placed that day; per cohort the tile count, quadrants spanned, M |
| `day_dump.cpp` | One seat's day, hour by hour, for one product: successful harvests of it, deposits, units carried, shed stock, own and opponent sales (units @ average price), market price before the market, and the o |
| `finalize_trace.cpp` | Raw Local-LB trace (tools/lb_trace_play.py) -> standard trace (load_replay format). Re-simulates the recorded actions in the C++ engine and checks the final money against the Python engine's. usage: f |
| `forecast_audit.cpp` | Opponent-forecast audit: for every seat and dawn, the seat's publicly visible supply per product (held product on animals, yield on harvestable crops: what the other player sees), its actual units sol |
| `full_games_reports.cpp` | Copy of experiments/v10/sep24_BC_opus/tools/full_games.cpp that also writes every dawn's report (BC_REPORTS=<dir>/<seed>_<seat>.txt: day, status, fallback, intent summary, reason). Full games: bc_opus |
| `hourly_audit.cpp` | Hour-by-hour opponent sales for forecasting: per seat, day and product, the dawn visible supply (held product on animals, yield on harvestable crops), the shed stock at dawn (private), and units sold  |
| `layout_audit.cpp` | Layout compactness per seat and day: tiles with successful farm work, their mean shed distance, and the Manhattan minimum spanning tree over them plus the shed access tiles (a lower bound proxy for th |
| `ledger.cpp` | Full-game ledger: replays traces in the exact engine and records, per seat, where money came from and went, what each crop cohort and animal produced, and every loss (decay, weeds, escapes, held-cap c |
| `market_audit.cpp` | Market of one product through a trace: per day, the dawn inventory and price, units sold by each seat, and consumption (shops and town) = dawn inventory + sales - next dawn inventory. usage: market_au |
| `melon_audit.cpp` | Melons per seat: plants by planting day with their shed distance, and on days 10-13 the hour of each melon harvest (yield taken) and the seat's melon sales by hour. usage: melon_audit list.txt out.csv |
| `route_audit.cpp` | Worker routes per seat and day (days 10-28): for each worker, moves made, work actions, and the Manhattan minimum spanning tree over its start tile and the tiles where it worked (a lower bound on the  |
| `shops_audit.cpp` | Shops unlocked in each trace: one row per trace and unlock day (3, 6, ..., 24) with the count of each shop type unlocked so far (replaying the recorded actions: the draw depends on empty tiles). usage |
| `trip_audit.cpp` | Worker trips: each worker's day split at shed interactions (successful PICKUP, DROP or PLACE of items). Per trip: moves, farm actions, distinct tiles worked, animal/crop mix, the Manhattan MST over th |

`lb_trace_play.py`: Local-LB games (like `experiment/scripts/lb_play.py`) that also record both seats'
actions as raw traces; `finalize_trace` turns them into engine traces. It imports the experiment's
`lb_play.py`: set `BC_EXPERIMENT=<working experiment folder>` (made by `setup_experiment.sh`), `BUILD`
and `BC_OPUS_MODEL`.

## Kaggle pipeline (`kaggle/`)

`list_subs.py` (our submissions), `fetch.py <submission id>` (episodes, replays, engine traces,
`meta_<id>.csv`; needs `BC_EXPERIMENT` for the replay converter), `overage.py <id>` (time use per
game), `lb.py` (current public leaderboard). Then `ledger` on the traces and `queries/q48.py`,
`q58.py`, `loss_report.py <episode>` for per-product and per-loss tables. Output of the Sep 25 run:
`../traces/kaggle/`.

## Queries (`queries/`)

Frozen reference scripts: they read the study's working files (`ledger/*.csv`, `labor/`, `zoo/`, ...),
so paths need adapting. Each one documents how a number in the study was computed.

| Script | Question |
|---|---|
| `analyze.py` | Loads ledger CSVs (tools/ledger) and builds per-perspective aggregates. import warnings warnings.filterwarnings("ignore") A perspective is one seat of one game. Top-team  |
| `cache.py` | Caches per-perspective (g) and per-day (d) tables plus animals/crops as pickles. |
| `compare.py` | Comparison tables: top-10 team perspectives vs our variants (and their opponents). usage: compare.py <ledger prefix glob> [...] e.g. compare.py 'ledger/top?' 'ledger/ours |
| `frozen_pair.py` | Frozen replays: probe vs base, paired per game, split into top-10 replays and our Kaggle games. |
| `loss_report.py` | One Kaggle game of our submission, ours vs opponent: revenue by product, costs, dawn money, herd and crops, melon sales, contested-product prices by hour, crash selling,  |
| `mirror_summary.py` | Mirror results: ours (X13 + probe) vs opponent (X13); seed-clustered CI of the margin. |
| `mk_top.py` |  |
| `our_list.py` | Finalizes raw Local-LB traces and writes ours_list.txt for tools/ledger: line = trace label_seat0 label_seat1 group; our seat is labelled ours:<variant>, the opponent opp |
| `paired.py` | Paired comparison of Local-LB game sets (json per game): B - A in margin and win rate per opponent and pooled, with seed-clustered 95% bootstrap CIs. usage: paired.py <di |
| `q1.py` |  |
| `q2.py` |  |
| `q3.py` |  |
| `q4.py` | our seat: label from list |
| `q5.py` |  |
| `q6.py` |  |
| `q7.py` |  |
| `q8.py` |  |
| `q9.py` |  |
| `q10.py` |  |
| `q11.py` |  |
| `q12.py` |  |
| `q13.py` |  |
| `q14.py` |  |
| `q15.py` |  |
| `q16.py` |  |
| `q17.py` |  |
| `q18.py` |  |
| `q19.py` |  |
| `q20.py` |  |
| `q21.py` |  |
| `q22.py` |  |
| `q23.py` |  |
| `q24.py` |  |
| `q25.py` |  |
| `q26.py` |  |
| `q27.py` |  |
| `q28.py` |  |
| `q29.py` |  |
| `q30.py` |  |
| `q31.py` |  |
| `q32.py` |  |
| `q33.py` |  |
| `q34.py` |  |
| `q35.py` |  |
| `q36.py` |  |
| `q37.py` |  |
| `q38.py` |  |
| `q39.py` | Slot order of SELL orders in top-team traces: for hours with >= 2 sell orders, is the list in fixed product order (ascending item id), and which products take the first s |
| `q40.py` |  |
| `q41.py` |  |
| `q42.py` |  |
| `q43.py` |  |
| `q44.py` | Per-product linear forecast of the opponent's daily sales: sold ~ a*trailing + b*visible (no intercept, non-negative), fitted on top-team perspectives (odd traces), evalu |
| `q45.py` |  |
| `q46.py` |  |
| `q47.py` | Gap 3: are units held overnight worth holding? Per (game, seat, day d, product), held units H = shed stock at dawn d+1; realized = average price of the first H units sold |
| `q48.py` | Kaggle games of our submission: per game, ours vs opponent by product (revenue, units, price), wages, animals at day 12, melons planted by day 3; sorted by margin. |
| `q49.py` | Kaggle games vs top teams: hires per day (distribution), wage per day, fertilizer missed by day. |
| `q50.py` | Mirror traces: ours minus opponent by product (revenue, units, price), wages, for base vs a probe. |
| `q51.py` | Labour: where does the wage gap come from? Days 10-28. Sources: ours:kaggle (Kaggle submission), ours:x13 (Local-LB), ours:base (C++ mirror), top (top-10 perspectives). P |
| `q52.py` | Action mix per day (days 10-28): ours vs top; watering vs need; moves per non-water action. |
| `q53.py` | Idle, moves and shed visits by hour (days 10-28): ours vs top. |
| `q54.py` | Selling into a crash: units sold on day d at an average price p when the market price reaches >= 1.5 p within the next 5 days (max hourly price). Per game: units, revenue |
| `q55.py` | Crash behaviour: on days when a product's mean market price is below half its base price, the share of available stock (shed at dawn + harvested that day) each seat sells |
| `q56.py` | Hour-by-hour opponent sales forecasts, made at dawn, scored on cumulative units sold by each hour (what decides races): CWAPE = sum_h /S_true(h) - S_pred(h)/ / sum_h S_tr |
| `q57.py` | 4th quadrant: how often and when top teams buy it, and outcomes with vs without (overall and within the same team). Also opponents with 4 quadrants in our Kaggle games. |
| `q58.py` | All Kaggle losses of our submission in one table: ours minus opponent by cause. |
| `q59.py` | Marginal value of each animal type: top-10 perspectives, own final money regressed on geese, cows, sheep at day 12 plus shop counts at day 12 (from the shops audit) and l |
| `smoke_compare.py` | Smoke results vs X10 on the same games: Local-LB (exp/x10_trim_loss) and zoo (zoo/x10). |
| `zoo_analysis.py` | Zoo panel analysis: our candidate (base / x10 / x12) vs unpatched BC zoo models. 1. Outcome per matchup; 2. gap decomposition (ours - opponent) by product revenue and spe |
| `zoo_list.py` | Ledger list for the zoo panel: zoo/<cand>/<model>/<seed>_<seat>.txt, our seat = <seat>. |
| `zoo_pair.py` | Paired zoo comparison: B vs A on identical (opponent, seed, seat), seed-clustered CIs. usage: zoo_pair.py A B |
| `zoo_paired.py` | Paired zoo comparison: candidate vs base on identical (opponent, seed, seat), seed-clustered CIs. |
