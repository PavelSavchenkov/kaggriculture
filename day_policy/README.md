# Day policy

Deterministic C++ worker scheduling and tile placement for a supplied whole-day
plan. The default is **Balanced-4 + Staged placement**, with hire minimization
and a hard cap of 13 hires plus the farmer. Use cap 11 when it is mandatory.
**Full-8** searches more broadly at higher cost; it is a separate call, not an
automatic fallback. **day_policy_80p** is the third option: it uses 11 hires,
skips hire minimization and searches a smaller workload-gated set of complete
schedules for higher throughput. **unrestricted_day_policy** is the fourth
option. It preserves the exact hour of every purchase, can buy and pick up
fertilizer, allows hires at hours 0-23, and performs bounded cheap hire reduction
from a cap-13 schedule. All four return only complete, verified schedules.

Policy code, replay data and test tools are in this folder. They use five shared
headers already committed and pushed under `fast_game_engine/` and
`agents/common/`; their commit and hashes are recorded in
`REPOSITORY_DEPENDENCIES.json`. No experiment files are required.
Build prerequisites are a C++20 compiler, CMake, a Linux toolchain and Python 3
with its standard library.

## Coverage and timings

### Earlier restricted contract

Two comparison cohorts from the recorded top 30 players, using exact original
dawn layouts and our placement for new products. At each cap, include only days
with **original hires <= that cap**. Hires exclude the farmer. Timings are
**median / average milliseconds**, including failed solves and hire minimization
when the profile enables it.

| Hire cap | Balanced-4 solved | Median / average ms | Full-8 solved | Median / average ms | day_policy_80p solved | Median / average ms |
|---:|---:|---:|---:|---:|---:|---:|
| 10 | 971/975 | 0.96 / 1.74 | 972/975 | 0.57 / 2.14 | — | — |
| 11 | 1,145/1,183 | 1.06 / 4.84 | 1,162/1,183 | 0.61 / 16.22 | 1,145/1,183 | 0.20 / 2.77 |
| 13 | 1,275/1,279 | 1.16 / 7.41 | 1,275/1,279 | 0.78 / 29.94 | — | — |

`day_policy_80p` is defined only for 11 hires. On the cap-11 filtered cohort it
matches Balanced-4 coverage while reducing average solve time by 43%, and Full-8
solves 17 more days while taking 5.9 times as long on average. The unfiltered
cap-11 comparison, including every eligible comparison day, is:

| Profile | All days solved | Late days 20–28 solved | Median / average ms |
|---|---:|---:|---:|
| Balanced-4 | 1,223/1,284 (95.2%) | 95/115 (82.6%) | 1.17 / 6.19 |
| Full-8 | 1,246/1,284 (97.0%) | 106/115 (92.2%) | 0.81 / 25.93 |
| day_policy_80p | 1,221/1,284 (95.1%) | 98/115 (85.2%) | 0.23 / 4.08 |

The throughput profile therefore stays above 80% on both all days and late days.
It trades two all-day solves against Balanced-4 for 34% lower average time, while
solving three more late days. It does this by accepting non-minimal 11-hire
schedules, not by removing large cases from the denominator.

### Unrestricted contract

The new extractor no longer excludes or normalizes late seed/animal purchases,
fertilizer buys, fertilizer pickups, or hires after hour 1. This expands dev from
914 to 2,239 eligible days and validation from 1,284 to 3,071. Every cap uses the
same full denominator; cases are not filtered by original hire count. Fixed-cap
rows disable hire minimization so the cap comparison is direct.

| Cohort | Profile / cap | All days solved | Late days 20-28 solved | Median / average ms | Mean hires on success |
|---|---|---:|---:|---:|---:|
| dev | Balanced-4 cap 10 | 1,387/2,239 (61.9%) | 228/708 (32.2%) | 3.34 / 14.70 | 10.00 |
| dev | Balanced-4 cap 11 | 1,925/2,239 (86.0%) | 508/708 (71.8%) | 0.41 / 10.68 | 11.00 |
| dev | Balanced-4 cap 13 | 2,170/2,239 (96.9%) | 669/708 (94.5%) | 0.35 / 4.24 | 13.00 |
| dev | unrestricted_day_policy | 2,170/2,239 (96.9%) | 669/708 (94.5%) | 0.61 / 4.47 | 11.43 |
| validation | Balanced-4 cap 10 | 1,879/3,071 (61.2%) | 282/959 (29.4%) | 3.71 / 14.89 | 10.00 |
| validation | Balanced-4 cap 11 | 2,580/3,071 (84.0%) | 661/959 (68.9%) | 0.47 / 11.26 | 11.00 |
| validation | Balanced-4 cap 13 | 2,985/3,071 (97.2%) | 911/959 (95.0%) | 0.35 / 4.29 | 13.00 |
| validation | unrestricted_day_policy | 2,985/3,071 (97.2%) | 911/959 (95.0%) | 0.61 / 4.51 | 11.41 |

The unrestricted policy preserves cap-13 coverage while saving 3,405 hires on
dev and 4,736 on validation. Its bounded reduction raises mean latency by 5.5%
on dev and 5.0% on validation. It only runs after a first-attempt success with at
most 60 work units; harder calls do not pay minimization cost.

Coverage on validation cases using each removed restriction is:

| Exact late seed/animal input | Fertilizer buy | Fertilizer pickup | Hire after hour 1 | Any removed restriction |
|---:|---:|---:|---:|---:|
| 2,744/2,820 (97.3%) | 1,154/1,218 (94.7%) | 1,355/1,431 (94.7%) | 420/427 (98.4%) | 2,867/2,953 (97.1%) |

The unrestricted 11-hire row remains above 80% overall but not on late days.
Late exact-timing cases need the cap-13 policy to retain main-solver-like
coverage. Lower caps are also slower on average because failed Balanced-4 calls
exhaust their search.

Measured on **Intel Core i7-14700K, Linux x86-64**, with one solver process pinned
to logical CPU 14. Each configuration combines two sequential runs. Build:
**GCC 13.3, C++20, O3, native CPU tuning and LTO**, without PGO or instrumentation.
Other machine activity was not controlled. Times include the solver's internal
verifier and exclude the evaluator's second verification.

All 30 [original-agent splits](#per-original-agent) appear below. The
[full cap report](measurements/CAPS.md) also covers development and late days,
and results with our placements carried from game start.

## Contents

| Path | Purpose |
|---|---|
| `SPEC.md`, `source/policy.hpp` | Input/output contract and C++ types |
| `source/` | Worker search, placement, native verifier and executor |
| `docs/ALGORITHM.md` | Search profiles, placement rules and integration limits |
| `docs/TESTING.md` | Cohort construction, exclusions and failure interpretation |
| `docs/REPLAY_PATTERNS.md` | Placement evidence from top-player games |
| `measurements/REPORT.md`, `SUMMARY.json` | Total/per-player coverage, hires and timings |
| `measurements/CAPS.md`, `CAPS.csv`, `CAPS.json` | Caps 10/11/13 filtered to original hires <= the tested cap, by cohort and original agent |
| `measurements/rows.zip` | Compressed per-call measurement evidence |
| `measurements/unrestricted/` | Expanded cohorts, cap 10/11/13 results and exact per-call evidence |
| `data/` | Compressed replay traces, selected cohorts and format documentation |
| `tests/`, `tools/` | Regressions, complete-game smoke tests and replay benchmarks |
| `PROVENANCE.json`, `REPOSITORY_DEPENDENCIES.json` | Source hashes and the committed shared engine/API dependencies |
| `MANIFEST.json`, `PACKAGE_CHECK.json` | File integrity/size inventory and delivery checks |

## Use

```cpp
#include "policy.hpp"
#include <memory>
using namespace kag::agents::day_policy_contract;

auto solver = std::make_unique<Solver>(); // One instance per thread.
DayInput input;                          // Fill exact dawn grid and declared jobs.
SolveOptions options;                   // Balanced-4, Staged, cap 13.
options.max_hires = 11;
auto result = solver->solve(input, options);
if (result.status == SolveStatus::Success) {
    // Execute result.schedule[0..23]; propagate result.state.
}
// Broader independent call: options.effort=SearchEffort::Full; options.variants=8;
// Independent throughput call: auto throughput = day_policy_80p();
// Expanded contract plus bounded cheap hire reduction:
// auto unrestricted = unrestricted_day_policy();
```

Initialize all 100 cells, including locked land and overnight weeds. Timestamps
are relative to dawn, as documented in `policy.hpp`. Success means all requested
events and cumulative worker deposits passed native replay. `InvalidInput` and
`NoScheduleFound` expose no executable partial plan; the latter is not an
infeasibility proof. Minimum hires are not proved.

`buy_seeds[hour][crop]`, `buy_animals[hour][animal]`, `buy_wheat[hour]`, and
`buy_fertilizer[hour]` preserve exact purchase timing. This is an API change from
the earlier dawn-normalized seed/animal arrays.

The caller handles purchase funding, sales, shed capacity and night settlement.
Adding sales must preserve scheduled purchases/hires and fit remaining market
slots; the policy reserves no sale slots or financing. Result state is after
hour 23 actions/decay, before automatic night deposits and random changes.
Propagate this actual state, including repaired crop sites. The shortened final
game day needs separate integration. `agent.json` wraps the executor and a
smoke-test planner; it is not a competitive purchase/sale decision engine.

## Build and reproduce

Commands below run from the repository root. Generated files go to
`work/day_policy`; nothing is generated inside the delivery folder.

```sh
conda run -n kaggriculture cmake -S day_policy -B work/day_policy/build -DCMAKE_BUILD_TYPE=Release
conda run -n kaggriculture cmake --build work/day_policy/build -j 4
conda run -n kaggriculture python -B day_policy/tools/check_package.py
conda run -n kaggriculture ctest --test-dir work/day_policy/build --output-on-failure
conda run -n kaggriculture python -B day_policy/tools/build_unrestricted.py --work work/day_policy/unrestricted --build work/day_policy/build
taskset -c 14 work/day_policy/build/contract_evaluate work/day_policy/unrestricted/dev.bin work/day_policy/unrestricted/dev_h13.csv 1000000 13 3 0 0 4 1 0 1 0 1
conda run -n kaggriculture python -B day_policy/tools/report_unrestricted.py --archive day_policy/measurements/unrestricted/rows.zip --output work/day_policy/unrestricted_report
conda run -n kaggriculture python -B day_policy/tools/test.py smoke --work work/day_policy/check --build work/day_policy/build --run smoke
```

`build_unrestricted.py` validates and expands every packaged trace, then creates
the 2,239-case dev and 3,071-case validation binaries. The evaluator arguments
above select cap 13, Balanced-4, four variants, no full hire minimization, and
bounded opportunistic hire reduction. Use caps 10 or 11 and set the last argument
to 0 for the fixed-cap comparison rows.

`measurements/unrestricted/rows.zip` stores both repeats for all eight rows in
the unrestricted table plus the case metadata. `report_unrestricted.py`
recomputes the tables and checks exact non-time parity between repeats.

`tools/test.py` also retains the older placement/progression suites. The files
directly under `measurements/` are historical restricted-contract baselines;
do not compare them with `--check-reference` after this API expansion.

For timings, choose an available core with `--cpu N` and run benchmarks
sequentially without concurrent builds. Times will vary by machine and load.
New `SUMMARY.json` files contain total/per-player coverage and timings, plus
original-hire-filtered totals at each cap. All successful schedules receive
a second native verification outside the reported solver time.

Regenerate the saved report from its archived CSV evidence:

```sh
conda run -n kaggriculture python -B day_policy/tools/test.py unpack-reference --work work/day_policy/check
conda run -n kaggriculture python -B day_policy/tools/report.py --results work/day_policy/check/reference --output work/day_policy/report
conda run -n kaggriculture python -B day_policy/tools/report_caps.py --output work/day_policy/caps_report
```

Use `--suite caps` for the complete cap-10/11/13 matrix: original dawns with
Balanced-4/Full-8, and our carried placements with Balanced-4 under strict and
hour-23 deadlines. `CAPS.md` applies `original_hires <= tested cap` before
aggregating coverage and timings. `CAPS.csv` includes every original agent in
each cohort and in the two comparison cohorts combined.

Release uses O3, native CPU tuning and LTO, without PGO or fast-math. Set
`-DDAY_POLICY_NATIVE=OFF` for a portable CPU build. For sanitizers, use a separate
build directory with `-DCMAKE_BUILD_TYPE=Debug -DDAY_POLICY_SANITIZE=ON`, then
run CTest, smoke and replay checks against it. To embed just the library, use
`-DBUILD_TESTING=OFF` and link the `day_policy::policy` CMake target.

## Per original agent

The same original-hire filter and machine/timing conditions as the table above
apply. Each agent has four games across the two comparison cohorts; eligible
day counts vary after filtering. Timings are median / average milliseconds.

### Hire cap 10; original hires <= 10

| Original agent | Balanced-4 solved | Median / average ms | Full-8 solved | Median / average ms |
|---|---:|---:|---:|---:|
| Majkel1337 | 41/43 | 0.84 / 4.07 | 41/43 | 0.92 / 10.45 |
| ymg_aq | 46/46 | 0.64 / 1.59 | 46/46 | 0.53 / 1.68 |
| SpaTaro | 45/47 | 1.89 / 3.31 | 46/47 | 1.24 / 8.21 |
| Mengfei Li | 28/28 | 1.05 / 1.22 | 28/28 | 0.56 / 0.86 |
| Orbital Terraformer | 45/45 | 1.16 / 2.49 | 45/45 | 0.87 / 3.32 |
| Otter Vibe | 40/40 | 0.70 / 1.33 | 40/40 | 0.45 / 1.01 |
| redblackbst | 27/27 | 0.96 / 1.37 | 27/27 | 0.57 / 0.94 |
| feel the agi | 35/35 | 1.17 / 1.91 | 35/35 | 0.60 / 1.23 |
| Catalyst | 28/28 | 0.96 / 1.43 | 28/28 | 0.57 / 1.01 |
| Thomas Tschinkel | 31/31 | 0.96 / 1.54 | 31/31 | 0.58 / 1.08 |
| HowardLeeTW | 35/35 | 0.75 / 1.12 | 35/35 | 0.51 / 0.64 |
| Artem The Farmer 🍅 | 41/41 | 1.10 / 2.38 | 41/41 | 0.92 / 3.20 |
| アルモンド | 21/21 | 0.76 / 0.93 | 21/21 | 0.51 / 0.59 |
| 𝕯𝖊𝖔𝖉𝖎𝖒𝖘 & 𝕮𝖔 | 29/29 | 0.81 / 1.07 | 29/29 | 0.51 / 0.73 |
| leave you | 32/32 | 1.03 / 1.66 | 32/32 | 0.59 / 1.14 |
| keiz | 24/24 | 0.91 / 1.04 | 24/24 | 0.60 / 0.75 |
| Cow Boy | 28/28 | 0.96 / 1.37 | 28/28 | 0.58 / 0.99 |
| Zhenghongshuang | 28/28 | 0.96 / 1.45 | 28/28 | 0.58 / 1.06 |
| THIRD FARM CLUB | 31/31 | 0.74 / 2.38 | 31/31 | 0.56 / 6.23 |
| AI是我的豆包 | 28/28 | 0.96 / 1.39 | 28/28 | 0.57 / 1.00 |
| Kaggriculture Agent | 34/34 | 1.05 / 1.55 | 34/34 | 0.60 / 1.08 |
| Knight of Favonius | 25/25 | 0.95 / 1.31 | 25/25 | 0.57 / 0.88 |
| THUNDER THUNDER | 42/42 | 0.82 / 1.43 | 42/42 | 0.60 / 1.03 |
| Phoenix750 | 32/32 | 0.96 / 1.42 | 32/32 | 0.59 / 0.98 |
| Jeryos | 28/28 | 0.95 / 1.35 | 28/28 | 0.58 / 0.98 |
| nilochan | 30/30 | 0.96 / 1.50 | 30/30 | 0.58 / 1.06 |
| nofreewill42 | 29/29 | 0.96 / 1.47 | 29/29 | 0.57 / 1.00 |
| lucaskna | 30/30 | 0.96 / 1.38 | 30/30 | 0.57 / 0.97 |
| Emile Andrieu | 28/28 | 0.96 / 1.48 | 28/28 | 0.57 / 0.99 |
| Zhongyi Dai | 30/30 | 0.96 / 1.49 | 30/30 | 0.57 / 1.04 |

### Hire cap 11; original hires <= 11

| Original agent | Balanced-4 solved | Median / average ms | Full-8 solved | Median / average ms |
|---|---:|---:|---:|---:|
| Majkel1337 | 88/111 | 9.36 / 19.99 | 101/111 | 7.23 / 83.44 |
| ymg_aq | 51/51 | 0.95 / 1.77 | 51/51 | 0.76 / 1.36 |
| SpaTaro | 61/65 | 2.81 / 7.69 | 62/65 | 2.03 / 34.13 |
| Mengfei Li | 28/28 | 1.05 / 1.24 | 28/28 | 0.56 / 0.85 |
| Orbital Terraformer | 97/108 | 5.16 / 13.50 | 100/108 | 4.08 / 59.21 |
| Otter Vibe | 41/41 | 0.71 / 1.41 | 41/41 | 0.46 / 1.07 |
| redblackbst | 31/31 | 0.96 / 1.74 | 31/31 | 0.58 / 1.23 |
| feel the agi | 36/36 | 1.17 / 2.05 | 36/36 | 0.60 / 1.35 |
| Catalyst | 28/28 | 0.96 / 1.50 | 28/28 | 0.57 / 1.13 |
| Thomas Tschinkel | 32/32 | 1.05 / 1.75 | 32/32 | 0.59 / 1.25 |
| HowardLeeTW | 35/35 | 0.74 / 1.12 | 35/35 | 0.51 / 0.64 |
| Artem The Farmer 🍅 | 47/47 | 1.14 / 2.29 | 47/47 | 0.96 / 1.69 |
| アルモンド | 25/25 | 0.95 / 1.53 | 25/25 | 0.57 / 0.96 |
| 𝕯𝖊𝖔𝖉𝖎𝖒𝖘 & 𝕮𝖔 | 32/32 | 0.90 / 1.34 | 32/32 | 0.56 / 0.96 |
| leave you | 35/35 | 1.05 / 1.97 | 35/35 | 0.60 / 1.40 |
| keiz | 28/28 | 0.92 / 2.38 | 28/28 | 0.62 / 4.57 |
| Cow Boy | 29/29 | 0.96 / 1.50 | 29/29 | 0.58 / 1.09 |
| Zhenghongshuang | 28/28 | 0.96 / 1.53 | 28/28 | 0.58 / 1.12 |
| THIRD FARM CLUB | 34/34 | 1.10 / 2.58 | 34/34 | 0.57 / 5.91 |
| AI是我的豆包 | 28/28 | 0.96 / 1.47 | 28/28 | 0.57 / 1.08 |
| Kaggriculture Agent | 35/35 | 1.05 / 1.66 | 35/35 | 0.60 / 1.20 |
| Knight of Favonius | 29/29 | 0.96 / 1.73 | 29/29 | 0.57 / 1.19 |
| THUNDER THUNDER | 49/49 | 1.11 / 1.80 | 49/49 | 0.73 / 1.30 |
| Phoenix750 | 36/36 | 0.96 / 1.82 | 36/36 | 0.60 / 1.27 |
| Jeryos | 29/29 | 0.96 / 1.53 | 29/29 | 0.58 / 1.11 |
| nilochan | 31/31 | 0.96 / 1.69 | 31/31 | 0.58 / 1.19 |
| nofreewill42 | 30/30 | 0.96 / 1.63 | 30/30 | 0.57 / 1.15 |
| lucaskna | 30/30 | 0.96 / 1.45 | 30/30 | 0.58 / 1.05 |
| Emile Andrieu | 31/31 | 0.95 / 1.80 | 31/31 | 0.57 / 1.24 |
| Zhongyi Dai | 31/31 | 0.96 / 1.65 | 31/31 | 0.58 / 1.18 |

### Hire cap 13; original hires <= 13

| Original agent | Balanced-4 solved | Median / average ms | Full-8 solved | Median / average ms |
|---|---:|---:|---:|---:|
| Majkel1337 | 111/111 | 9.35 / 24.23 | 111/111 | 7.23 / 96.27 |
| ymg_aq | 58/58 | 1.32 / 9.94 | 58/58 | 1.01 / 51.52 |
| SpaTaro | 69/69 | 3.07 / 12.99 | 69/69 | 2.46 / 67.17 |
| Mengfei Li | 28/28 | 1.05 / 1.24 | 28/28 | 0.56 / 0.85 |
| Orbital Terraformer | 108/108 | 5.16 / 16.57 | 108/108 | 4.08 / 67.82 |
| Otter Vibe | 43/43 | 0.71 / 4.82 | 43/43 | 0.48 / 20.54 |
| redblackbst | 33/33 | 1.05 / 1.82 | 33/33 | 0.60 / 1.29 |
| feel the agi | 36/36 | 1.17 / 2.05 | 36/36 | 0.60 / 1.35 |
| Catalyst | 28/28 | 0.95 / 1.50 | 28/28 | 0.57 / 1.09 |
| Thomas Tschinkel | 32/32 | 1.01 / 1.73 | 32/32 | 0.59 / 1.25 |
| HowardLeeTW | 49/49 | 0.98 / 2.73 | 49/49 | 0.67 / 1.73 |
| Artem The Farmer 🍅 | 48/52 | 2.18 / 18.07 | 48/52 | 1.74 / 129.59 |
| アルモンド | 26/26 | 0.95 / 1.62 | 26/26 | 0.57 / 1.02 |
| 𝕯𝖊𝖔𝖉𝖎𝖒𝖘 & 𝕮𝖔 | 33/33 | 0.95 / 1.46 | 33/33 | 0.57 / 1.03 |
| leave you | 36/36 | 1.05 / 2.06 | 36/36 | 0.60 / 1.45 |
| keiz | 67/67 | 3.25 / 10.81 | 67/67 | 2.42 / 42.00 |
| Cow Boy | 32/32 | 1.00 / 4.11 | 32/32 | 0.59 / 15.98 |
| Zhenghongshuang | 28/28 | 0.95 / 1.53 | 28/28 | 0.57 / 1.12 |
| THIRD FARM CLUB | 36/36 | 1.11 / 5.57 | 36/36 | 0.58 / 16.11 |
| AI是我的豆包 | 28/28 | 0.96 / 1.47 | 28/28 | 0.57 / 1.08 |
| Kaggriculture Agent | 35/35 | 1.05 / 1.66 | 35/35 | 0.60 / 1.21 |
| Knight of Favonius | 30/30 | 0.96 / 1.86 | 30/30 | 0.58 / 1.27 |
| THUNDER THUNDER | 56/56 | 1.29 / 2.26 | 56/56 | 0.81 / 1.59 |
| Phoenix750 | 38/38 | 0.96 / 1.99 | 38/38 | 0.61 / 1.36 |
| Jeryos | 29/29 | 0.96 / 1.53 | 29/29 | 0.58 / 1.10 |
| nilochan | 35/35 | 1.05 / 3.13 | 35/35 | 0.60 / 8.18 |
| nofreewill42 | 30/30 | 0.96 / 1.63 | 30/30 | 0.57 / 1.15 |
| lucaskna | 30/30 | 0.96 / 1.50 | 30/30 | 0.57 / 1.05 |
| Emile Andrieu | 32/32 | 1.01 / 1.91 | 32/32 | 0.59 / 1.32 |
| Zhongyi Dai | 31/31 | 0.98 / 1.65 | 31/31 | 0.58 / 1.18 |

## Test limitations

Changed-layout tests preserve recorded prefix service outcomes, including
excluded/unsolved prefix days. They test geometry and execution capacity, not
a fully feasible economic rollout. Some inherited deadlines become physically
impossible after relocation; hour-23 diagnostics relax those deadlines and do
not establish early-return coverage. The comparison games were already exposed
during worker-policy development. These results do not establish full-agent
win rate or gold-medal strength.
