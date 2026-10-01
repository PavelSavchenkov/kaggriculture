# 02 Architecture of the final agents

All four final agents share one design. Python `main.py` is a thin adapter: it loads a C++ shared library
(`libopus_lb_bridge.so`) with `ctypes`, converts the Kaggle observation into numbers, and returns the library's actions. All
decisions happen in C++.

```
Kaggle turn -> main.py -> libopus_lb_bridge.so (opus_new / opus_act)
                            |
   dawn (hour 0):           |  1. BC network (+ 4 ensemble members) reads the farm + market state -> DayIntent logits
                            |  2. decode (model.bin.decode): counts by exact MAP DP, plus day-specific pushes
                            |  3. day compiler (dc11 / dc12 keys in model.bin.dc11): tiles, routes, hires, purchases,
                            |     funding checks, trims -> an hourly plan for every worker
   every hour:              |  4. executor: follows the plan, repairs it when the state changes (failed actions, weeds, cash)
                            |  5. market seller: a dynamic program over today's hours that chooses what to sell when, given
                            |     the opponent-sales forecast (model.bin.forecast_tf*) and the price curves
```

- **The network decides WHAT**: how many crops / animals to add, whether to buy land, and how many of each group to water,
  fertilize, harvest, clear, feed, care for and collect.
- **The compiler decides HOW**: which tiles, which worker, in what order, how many hires, when to sell. It never changes the
  farm's goals.
- This split was the biggest single gain of the project ([01_PROGRESSION.md](01_PROGRESSION.md), phase 4).

## Source layout (`agents/kaggle_*/source/`)

| Path | What |
|---|---|
| `source/source/lb_bridge.cpp` | The C API used by `main.py` (`opus_new`, `opus_act`, `opus_delete`) and the per-dawn soft deadline |
| `source/agent/bc_opus/source/agent.cpp` | Network inference, ensemble averaging, decode, sidecar parsing, forecaster loading (`.forecast_tf`, `.2`, `.3` averaged) |
| `source/agent/bc_overhaul/` | The agent class that runs the dc11 compiler |
| `source/dc11/compiler.cpp`, `router.cpp`, `market.cpp` | The day compiler: funding / trims, routing (construct + local search), market DP seller. dc12 keys are patches on top |
| `source/dc11_local/fc_transformer/` | C++ inference for the transformer forecaster |
| `source/day_policy_local/` | Shared contract / route helpers |
| `standalone/CMakeLists.txt` | Builds only the bridge from `../source`. Same flags as the shipped build (`-O3 -march=x86-64`, static libstdc++) |

The base (`kaggle_56720080_base_m3`) and the m19 build (`kaggle_56720831_honest1`, also used by `locallb_f1_fc2ens`) differ
only in compiler patches. The m19 build adds the `hirecheck / latehire / splitfert / nearanimals` code paths, all off by default.
With the base's keys it plays the base's games exactly.

## The model folder, file by file

| File | Content in the final agents | Meaning |
|---|---|---|
| `model.bin` | network `v17g6ft5`, 29 MB (`weights/network/v17g6ft5/`) | Main DayIntent network (width 512 family, fine-tuned on fresh Kaggle games; [05_MODELS.md](05_MODELS.md)) |
| `model.bin.features` | `4` | Feature-set version the network expects |
| `model.bin.condition` | `1 0.7500 1.0250` | Conditioning inputs: style flag, strength 0.75, recency 1.025 (the training default; recency 1.10 / 1.20 lost on the exact LB) |
| `model.bin.ensemble` | `members/w384b.bin sw100.bin mw5k.bin mw2k.bin` | Four older members. Their logits are averaged with the main network (1/5 each) on days 6+; the main network alone decides days 0–5 |
| `members/*.bin*` | `weights/network/members/` | The member networks with their own `.condition` / `.features` |
| `model.bin.opening` | `6 7` | Opening template for days 0–5 (a DSM-style opening chosen on mirror games; flagged as lineage-selected) |
| `model.bin.decode` | see below | Decode rules applied to the network output |
| `model.bin.compiler` | see below | Switches of the older (dc10) compiler layer that still apply |
| `model.bin.dc11` | one line of `key=value` | dc11 / dc12 compiler options (table below) |
| `model.bin.forecast_tf` (+`.2`, `.3`) | base: `big2`; honest1: `fc3nvens` 3 seeds; f1: `fc2` 3 seeds | Opponent-sales forecaster (transformer over days, with an intra-day head). Extra files are seeds, and the runtime averages them |
| `model.bin.forecast_tf_next` | next-morning head | Predicts the opponent's next-morning sales (used for holding decisions) |
| `model.bin.forecast_tf_exact`, `_intra`, `_nextm` | 0-byte flags | Switch on exact-quote pricing, the intra-day re-forecast and the next-morning head. **Keep them**: they are files with no content |

### Decode (`model.bin.decode`)

```
reach_days 6 14         herd "reach": on days 6-14 push animal counts toward the forecast-expected herd...
reach 0.9 0.8 0.7       ...with these weights (lineage-selected; neutral on a real-game re-tune)
reach_stress 0
max_land 3              the network may ask for at most 3 land quadrants...
v219 10 0 0 0 0         ...the 4th quadrant comes only from this day-10 rule (real-opponent backed: -2.4k without it)
earlycow 2 4 1          early cow push (days 2-4; weak evidence)
earlycrop 4 6 8 2       melon push: up to 2 melons on days 6-8 ("m68"; 760 games +191 / +431, wide duels +0.46k / +1.62k)
qpush 3 3 0.8 0.5       day-3 crop push ("d3crop"; the step to the best Kaggle score 2878)
```

### Older compiler switches (`model.bin.compiler`)

`sell_order 2` lists sale orders by revenue at stake. The engine fills both players' orders slot by slot at the same quote, so
order matters; this won 39/40 mirror games. The others (`return_wages`, `hire_cap_stock`, `collect_all`, `forecast_blend`,
`forecast_select`, `race_dp`, `race_early`, `race_steps`, `melon_tie_now`) were chosen on mirror games against our own packages.
Several are inert.

### Day-compiler keys (`model.bin.dc11`)

Base: `timing=0.5 landtrim=4 rival=1 tieall=1 scen=16 menu=3 nightfix=30 pricefloor=1 futurefloor=1 collectmin=5
startcomplete=1 dropany=6 cashsell=1 wheatcash=1 reserve=0 saleslots=3 nighttrim=1 survivalfloor=1`.
honest1 adds `hirecheck=1 latehire=1 splitfert=10`; f1 adds those plus `nearanimals=6`.

Evidence class (from the Day compiler's provenance audit): REAL = checked on real-opponent data, LOCAL = local beds only,
SAFETY = a guard that changes nothing in normal games.

| Key | What it does | Evidence |
|---|---|---|
| `timing=0.5` | Mixes deposit-timing scenarios in the router's value of a shed return | REAL (panels +0.9–1.4k with the learned forecaster); released after a Kaggle A/B by user rule |
| `landtrim=4` | Trims land asks that cannot be funded | REAL, neutral; removes NoLand collapses |
| `rival=1` | Seller values the opponent's lost revenue (margin objective) | LOCAL as a single key; pays only with the learned forecaster |
| `tieall=1` | Sell now on price ties (integer prices made ties common) | REAL (+1.6k on 79 top-30 pinned games); lineage-negative |
| `scen=16`, `menu=3` | Compiler search effort (scenarios, plan menu) | LOCAL; more effort (`rounds 12 scen 32`) did not help (−0.55k) |
| `nightfix=30` | Fixes 4th-quadrant shed overflow at night | REAL (+0.75k on 40 real games); value from a local sweep |
| `pricefloor=1 futurefloor=1` | Price floors in the seller | LOCAL; never positive alone. Removing them on f1: −0.16k (SE 0.20k), so harmless weight |
| `collectmin=5` | Collect fertilizer only from 5 units | REAL (pinned +240) |
| `startcomplete=1` | Day 0: drop to trims until a complete plan is funded (the starved-opening cliff) | SAFETY |
| `dropany=6` | Hire search may dissolve the least-loaded route | REAL (pinned real games +533, p < 1e-4) |
| `cashsell=1 wheatcash=1 reserve=0` | Sell the morning's goods and wheat for same-day funding; no next-dawn reserve (M&M-like) | REAL (pinned +1.5k), wide bed −1.24k (judged an artifact) |
| `saleslots=3` | Dawn sale orders get engine order slots (10 orders per turn, one per hire) | LOCAL |
| `nighttrim=1` | Trims night work | package-level only |
| `survivalfloor=1` | Cash floor so an unfunded day still feeds / keeps animals | SAFETY (one real world went −186k without it) |
| `hirecheck=1 latehire=1` | Fixes a hire-funding bug (partial hire waves passed the funding check) | SAFETY / bug fix; +105 (SE 78) on live continuations |
| `splitfert=10` | Days 3–9: harvest stops do not carry the fertilizer collection, so products reach the shed early | REAL on non-fc2 forecasters (+320 on m3's; +104 on fc3nvens), −70 with fc2 |
| `nearanimals=6` | Days 0–9: animals take the nearest free sites in M&M's order | **lineage-only** (live continuations −106 / −66); in f1 only |

Full key list and history: `code/day_compiler/docs/KEYS.md`; the patches that add them: `code/day_compiler/patches/`.

## Time budget

- Kaggle allows 1 s per turn plus a 60 s overage bank per game.
- Dawn turns compile the day and take ~1–2 s locally, up to 3.5 s on Kaggle's 4-core Xeon. Other turns are fast.
- The bridge has a per-dawn soft deadline. Old builds (dc11 v59) sometimes hit it on day 6 and lost about 0.9k in those games;
  v62+ builds are fast enough.
- The final agents keep ≥ 44 s of overage on Kaggle (kernel checks in `agents/*/checks/`).
