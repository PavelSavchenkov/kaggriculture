# Architecture

Paths are relative to `experiment/`. The contracts are `docs/design/day_intent.md` (network output)
and `docs/design/day_compiler.md` (compiler duties and validation); read them first.

## 1. Runtime path of one game

```
Kaggle observation (dict)
  -> agent/main.py            flattens it into a double buffer (+ remainingOverageTime)
  -> libopus_lb_bridge.so     source/lb_bridge.cpp: per-seat game state, opus_new/opus_act/opus_delete
  -> bc_opus::Agent::act      agent/bc_opus/source/agent.cpp
       every step:  History::observe      source/history.hpp (opponent flow inferred from the market)
       hour 0:      decode_intent         network forward pass + constrained decoding -> DayIntent
                    compile_day           source/compiler.cpp -> DayPlan (routes, hires, purchases, returns)
                    herd / crop reach     optional re-decode + re-compile with larger new-entity totals
       every hour:  DayExecutor::act      follows the plan; hourly sale DP against the opponent forecast
  <- integer action -> main.py -> {"farmer": [...], "hands": [...], "market": [...]}
```

The agent is deterministic: no wall-clock limits inside the compiler (fixed counts only), so the same
game replays bit-exactly on any machine. The only time-dependent branch is the overage valve (section 6).

## 2. Network (v12_cond)

Training code: `scripts/train.py` (PyTorch, strict FP32). Native inference: `agent/bc_opus/source/agent.cpp`
(`Model::load`, `mlp`, `conv`); native and PyTorch agree on 100/100 decisions (`tools/parity.cpp`,
`scripts/parity.py`).

**Inputs** (`source/features.hpp`, shared by extraction and the live agent, version 4):

- Global, 256 floats: day and days left; own/opponent cash (log) and margin; land owned and next land
  price; shed contents; seeds; shops present; per farm (own, opponent) tile-kind counts, plants by
  crop, harvestable held units, animals by species, held animal units, fertilizer ready, empty
  coops/pastures, mean shed distance of producers, producers ready within 0-3 days per product; per
  product price, market inventory, expected opponent daily net sales (last 3 days), shop/town demand;
  hires affordable; site capacity (free tiles etc., slots 208-214). Slots 216-218: conditioning
  (known flag, strength / 200, replay day index / 40). Slots 224-255: team style one-hot.
- Crop group, 48 floats: crop type, ongoing flag, age (absolute and relative to first/last yield day),
  yield/held, max yield, dry days, fertilizer days, decaying, new-group flag, size, fixed-value masks
  of the 9 options, product price and value, days to decay or next production, shed distance.
- Animal group, 32 floats: species, age, held product, unfed days, care bonus, new flag, size,
  care-fixed flag, product price and held value, at-capacity flag, days to next production, shed
  distance.
- Grid: both 10x10 farms, 24 channels per tile.

**Model** (width 256, about 2.3M parameters): 3-layer MLP encoders for global, crop groups and animal
groups; mean, max and sum/10 pooling over existing groups (new groups are not pooled); a tile CNN (two
3x3 conv layers, 32 channels, shared by both farms, mean+max pooled); a 3-layer context MLP. Heads:

- Whole farm: 11 count fields x 101 classes, a land logit, and a factorization: total new crops (101)
  + crop-type shares (5), total new animals (101) + species shares (3).
- Group heads (3-layer MLPs on [group encoding, context, 12 decoded whole-farm values]): per field an
  independent 101-way count over 0..group size ("marginal" heads). One-shot crop groups: 9 options
  (8 water/fertilize/harvest combinations + clear). Ongoing: retain, clear, fertilize, harvest. Animals:
  feed, care, collect.

**Decoding** (`decode_intent`), always a valid DayIntent:

1. New-crop and new-animal totals at a quantile (default the median; `q_crop`/`q_animal`) of the total
   distribution, masked to the sites possible today; split by type shares with largest remainder.
   Argmax was wrong: it returns 0 whenever "none" is the single most likely value.
2. Land: logit + `BC_LAND_BIAS` > 0. Animal reserves (unplaced after tonight) from their count heads.
3. Site capacity: if the new entities do not fit the free tiles, decode again with a lower cap; units
   whose removal loses the least log-probability go first (`cap_counts`).
4. Group fields: exact MAP partition by dynamic programming over the marginal heads (the counts that
   sum to the group size with the largest summed log-probability; fixed values forced to 0); ongoing
   crops choose retain and clear jointly, fertilize <= retain; animals care <= feed.
5. Fixed values (day 29, impossible or worthless actions) are never decoded (mask from the dawn state).

**Conditioning** (sidecars, section 5): strength +150 rating points over the ladder reference and
replay day 41 (`model.bin.condition` "1 0.7500 1.0250"); style: days 0-5 use team style 7 (DSM,
`model.bin.opening` "6 7"), later days use no style (all-zero one-hot). Style indices:
`experiment/data/styles.json` (1 M&M, 2 Majkel, 3 DECEM, 7 DSM, 8 Vadim, 9 Mother-Goose; 0 = every
other team, do not use it as "no style").

## 3. Day compiler (`source/compiler.cpp`)

`compile_day(dawn, history, intent, options, solver)` returns a `DayPlan`. Order of work:

1. **Validate** the intent against the schema (`source/intent.cpp`): partitions, bounds, site
   capacity (`free_sites`: open tiles, one-shot harvests and clears, crops turning into weeds today,
   ongoing clears, land bought today).
2. **Fallback ladder** (`compile_once`), levels dropped in this order: KeepAll -> NoLand ->
   NoNewEntities -> NoCollection -> SurvivalOnly. With `recovery` (default), survival still does the
   network's harvests and collections and feeds from cash plus 80% of that income.
3. For each level, up to 7 **funding variants** (only after a funding failure): plain; purchases first
   or hires first, with financing returns (sellable stock, collected fertilizer, required harvests,
   most valuable first) due by hour 3, 6 or 10.
4. **Route solve** (`day_policy_local/`, the repository's day_policy copy): tile binding, worker
   routes, hires. Portfolio of route heuristics; feasibility-first hire search (search at the largest
   affordable workforce, walk down); hires capped by what dawn cash (+90% of hour-0 sellable stock with
   `hire_cap_stock`) can pay; at most 13 hires. `route_search` widens the portfolio.
5. **Funding check**: the plan runs in an engine copy with a passive opponent under two forecasts:
   expected (economic) and stress (the opponent also sells all its visible output at hour 2). Purchases
   move to the hour before first use. **Next-dawn reserve**: from `reserve_from_day` (default day 1),
   end-of-day cash plus sellable goods must cover tomorrow's first feed (a farm that ends a day with
   neither cannot hire or feed, and everything dies).
6. **Return ladder** (ordinary days): the base plan leaves harvested units in workers' hands until the
   night deposit. The compiler then tries to add shed returns: a market part (units the sale DP would
   sell today, due by `market_deadline` 20, or per-product hours chosen by the sale DP with `race_dp`)
   plus a capacity part (units beyond tonight's shed room, due by the last hour); then the capacity
   part alone, then 75/50/25% of it. With `return_wages`, the market part is kept only if its sale
   gain covers the extra Fibonacci wages of the hires it adds. Day 29: returns by the last market
   with quotas 100/75/50/25/0%.
7. **Trims** (`trim_new_entity`, outer loop in `compile_day`): when the day compiles only by dropping
   every new entity, remove one unit at a time and recompile: day 0 drops crops before animals; other
   days drop the most expensive entity first (sheep, cow, goose, then crops by seed price);
   `trim_model` uses the network's least-likely unit instead. Fixed number of trims (no clock).

`DayPlan` records status, fallback level, funding variant, return percent, hires, the full reason log
(`reason`: every failed attempt, e.g. `L0F0:stress order 4 item 0 short at hour 0`), which the traces
print.

## 4. Executor and sales (`DayExecutor`, `choose_sales`)

Every hour the executor issues the plan's worker actions and builds market orders:

- **Sales**: per product, a DP over today's remaining market hours with price impact of our own units,
  town/shop demand per step, the expected opponent sales per hour, a value for units held overnight
  (`hold_discount` 0.95 x tomorrow's price) and a per-unit charge that keeps room for tonight's deposit.
  Inputs needed later today or at tomorrow's first pickups are kept (1 wheat per animal).
- **Order list**: sales first, then the plan's purchases and hires. `sell_order` 2 lists sales by
  revenue at stake (price drop our quantity causes x units): the engine fills both players' orders
  slot by slot at one quote per slot, so an earlier slot sells at the higher price.
- **Cash guard**: seed/animal purchases never leave less than a few dollars for tomorrow's hires.
- **Opponent forecast** (`rival[hour][product]`): default the trailing 3-day hourly mean of the
  opponent's inferred net sales (`History::expected`); inferred = market inventory change + known
  town/shop demand - our actual fills (never our requested quantities). Options replace or reshape it
  (`forecast_*`, section 5). Day 29 also expects the opponent to liquidate the stock it holds.

## 5. Configuration: sidecars and options

A model folder holds `model.bin` plus sidecar text files read by `Agent::reset`. Each agent in one
process keeps its own settings, so two versions can play each other. Precedence: code defaults, then
`<model>.compiler`, then `DC10_*` environment variables (experiments only; they leak into every agent
in the process, including opponents' bridges).

| Sidecar | Format | Meaning |
|---|---|---|
| `model.bin.features` | `4` | input feature version |
| `model.bin.condition` | `known strength/200 day/40` | conditioning inputs; `1 0.7500 1.0250` = strength +150, day 41 |
| `model.bin.opening` | `days style` | style for days < days (`6 7` = DSM days 0-5) |
| `model.bin.style` | `style` | whole-game style (zoo clones) |
| `model.bin.opening_model` | path | another network for the opening days |
| `model.bin.ensemble` | paths | average whole-farm logits with these networks |
| `model.bin.decode` | keys below | decoding quantiles and reach |
| `model.bin.compiler` | `key value` lines | compiler options below |

`model.bin.decode` keys: `days a b` with `q_crop x` / `q_animal x` (fixed quantiles on those days);
`reach_days a b` + `reach q1 q2 ...` (herd reach: after a full compile, try these new-animal quantiles in
order and keep the first plan that also compiles in full); `creach_days`/`creach` (the same for new
crops); `reach_stress 0|1` (a reached plan must also be funded under the stress forecast; fixes the
collapses of WEAKNESSES 0.5); `stress_down q ...` (on reach days, replace a base plan funded only under
the expected forecast by a smaller stress-funded one; rejected); `land_push a b bias` (add bias to the
land logit on days a..b; 4th-quadrant probe). `model.bin.land_model` ("path style"): the network and
style used on the land-push days and once the farm owns 4 quadrants (4th-quadrant fine-tune probe).

Compiler options (`CompileOptions` in `source/compiler.hpp`; sidecar key = field name; env var
`DC10_<KEY>` unless noted):

| Key | Default | In `forecast` | Meaning | Evidence |
|---|---|---|---|---|
| `sell_order` | 0 | 2 | 0 product index, 1 price, 2 revenue at stake | 39/40 vs 0 in the mirror |
| `return_wages` | 0 | 1 | market returns only if sale gain > extra wages | +0.9k exact Local-LB |
| `hire_cap_stock` | 0 | 1 | hire cap counts 90% of hour-0 sellable stock | rescues crunch days, otherwise identical |
| `collect_all` | 0 | 1 | collect every animal's fertilizer | 88-32 |
| `forecast_blend` | 0 | 1 | opponent forecast from visible supply + trailing | 101-19 |
| `blend_visible` | 50 | 50 | percent weight of visible supply (non-melon) | 70: 46-34; 85, 100, 30 worse |
| `recovery` | 1 | 1 | survival keeps harvests/collections (env `DC10_NO_RECOVERY`) | 3 collapses -> 0 |
| `reserve_from_day` | 1 | 1 | first day of the next-dawn cash reserve | 0 = robust rules |
| `forecast_select` | 0 | - | per product blend or fitted forecast by recent error | C++ panel +0.7k [+0.1k, +1.3k] (`select`) |
| `forecast_fit` | 0 | - | fitted linear opponent-sales model (`FORECAST_FIT`) | mirror 55-65, C++ +1.2k [-0.2k, +2.6k] |
| `forecast_prior` | 0 | - | strong-player hourly prior for melon/milk/wool (`source/opp_prior.hpp`) | part of `anticipate` |
| `race_dp` | 0 | - | per-product delivery hour chosen by the sale DP | 86-64-10, +50% compile |
| `race_early`, `race_steps`, `melon_tie_now` | 0 | - | race_dp from hour 6; retries n steps later; melons sell on ties | part of `anticipate` |
| `route_search` | 0 | - | 1 wide search every solve, 2 only on >= 12-hire days, 3 medium | 1: 73-47 but 73 s/game |
| `forecast_stock`, `forecast_adapt`, `forecast_intraday`, `forecast_timing` | 0 | - | other forecast variants | neutral or worse |
| `trim_model` | 0 | - | network chooses the trimmed units | neutral on slots |
| `race`, `race_deadline`, `sale_tie_now` | 0, 10, 0 | - | fixed melon race | neutral |
| `market_deadline` | 20 | 20 | hour the market part of returns is due | 22 neutral |
| `level_hires` | 0 | - | defer waitable work on >= N-hire days | 5-33 |
| `full_care`, `slack_care`, `fert_value` | 0 | - | care/fertilizer rules | lost or neutral |

Experiment-only environment switches with no sidecar: `DC10_TRIM_D0_ANIMALS` (old day-0 trim
order, needed for `vadim`), `DC10_TRIM_CROPS_FIRST`, `DC10_NO_RESERVE`, `DC10_RESERVE_FARM`,
`DC10_NO_CASH_GUARD`, `DC10_NO_TRIM`, `DC10_LAND_FIRST`, `DC10_BUDGET_REVISION`, `DC10_HIRE_SCAN`
(old ascending scan), `DC10_NO_HIRE_CAP`, `DC10_NO_WARM_START`, `DC10_LADDER_EFFORT`,
`DC10_LADDER_MAX_EXEC`. Diagnostics: `DC10_DEBUG` (full attempt log), `DC10_PROFILE` (time per
phase), `DC10_SALE_DEBUG=<hour>` (sale DP of one hour).

Network/decoding switches (`BC_*`): `BC_OPUS_MODEL` (model path; the bridge reads it when a game
starts), `BC_OPUS_STYLE`, `BC_OPUS_STRENGTH`, `BC_OPENING_DAYS`/`_STYLE`/`_MODEL`, `BC_ENSEMBLE`,
`BC_Q_CROP`, `BC_Q_ANIMAL`, `BC_LAND_BIAS`, `BC_PUSH_DAYS=a-b`, `BC_FEED_ALL`, `BC_CARE_ALL`,
`BC_COLLECT_ALL`, `BC_HARVEST_ALL`, `BC_SAMPLE`, `BC_TIME_LEFT` (test the valve), `BC_COUNT_MODE=argmax`,
`BC_PER_TYPE`, `BC_NO_MODES`.

## 6. Bridge, packaging, time

- `source/lb_bridge.cpp`: C API `opus_new(config, n, seat)`, `opus_act(game, fields, n, out, cap)`,
  `opus_delete`. Field order = `fast_game_engine/export_trace.py`. `OPUS_DUMP=<file>` records every
  input buffer; `tools/lb_replay <dump>` replays a Local-LB game without the Python opponent (add a day
  number for `DC10_DEBUG` on that day).
- `agent/main.py`: finds the bridge via `sys.path` or `/kaggle_simulations/agent` (Kaggle `exec()`s
  main.py without `__file__`), starts a new game when the step goes backwards, sets `BC_OPUS_MODEL`
  just before `opus_new` (never at import: other agents in the process read the same variable), and
  appends `remainingOverageTime`.
- Portable build (`BC_PORTABLE=ON`): `-march=x86-64`, static libstdc++/libgcc, conda-forge GCC 14
  against a glibc 2.28 sysroot. Kaggle's kernel image has glibc 2.35, the Local-LB image 2.36.
- Time valve: remaining overage < 25 s -> fast search for the market-return attempt; < 10 s -> capacity
  returns only and capped route executions; `route_search` needs > 40 s, `race_dp` > 30 s. On an
  idle machine decisions never depend on time.
- Compile time of `forecast` on the 8 recorded Local-LB games (loaded E-cores): 21 s per game, worst
  dawn 4.3 s (budget: 1 s per turn + 60 s overage per game). ~94% of compile time is the return
  ladder's re-solves; inside the solver ~75% is job reordering in `routes.cpp`.

## 7. Tools and scripts

C++ tools (`tools/`, one executable each): `full_games` (vs a registered C++ opponent or `bc:<model>`),
`search_games` (mirror games; with search: per-dawn decision search with full-game rollouts),
`replay_games` (frozen replays), `continuation` (replay continuations), `extract` (dataset + coverage
gate), `decode_eval` (teacher-forced decode check), `parity`, `lb_replay`, `forecast_audit`,
`sales_audit`, `farm_audit`, `melon_race`, `site_audit`, `trace_check`, `db_trace`. Opponents are
built from `agents/*/agent.json` manifests (`opponents/adapter.cpp.in`). Scripts: EVALUATION.md and
TRAINING.md.
