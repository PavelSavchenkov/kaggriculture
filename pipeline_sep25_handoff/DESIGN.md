# Design: DayIntent + day compiler

This document explains the approach (why the agent is split this way), the two contracts it is
built on, and the exact implementation and API. The contracts themselves are in `docs/design/`:

| File | What it is |
|---|---|
| `docs/design/day_intent.md` | The network's output contract: fields, groups, fixed values, validity, exact label conversion, coverage gate (repo `designs/`, Sep 24) |
| `docs/design/day_compiler.md` | The compiler's contract: what it decides, market planning, failure handling, validation of economic value, development gates (repo `designs/`, Sep 24) |
| `docs/design/day_policy_SPEC.md` | The route solver's contract (copy of the repository's `day_policy/SPEC.md`) |
| `docs/design/network_compiler_design_sep21.txt` | The full Sep 21 design this approach grew from (system boundary, compiler, forecasts, learning plan) |
| `agent_sep23/DESIGN.md`, `agent_sep23/design.pdf` (committed) | The Sep 23 generation of the same idea (older DayIntent with procedures and population targets) |

## 1. Why split the agent into a network and a compiler

A game is 720 turns; each turn up to 14 workers act and 10 market orders run. Most of that is
mechanics with hard rules and long dependencies: tile legality, paths, carrying, shed capacity, order
slots, funding hour by hour. Strategy is a much smaller set of choices: what the farm should grow,
which animals to keep and serve, when to expand, what to harvest today.

- **Behaviour cloning on raw actions** would have to learn route legality and would copy experts'
  incidental paths and order lists. **End-to-end RL** on this action space was judged brittle.
- So the agent is split at the day boundary. Every dawn the **network** emits a DayIntent: what the
  farm should accomplish today. A deterministic **day compiler** finds a legal, funded execution and
  owns every mechanical choice: tiles, workers, routes, hires, purchases, shed returns, sales.
- The Sep 21 design's final principle: *the network chooses decisions whose value depends on the
  future farm; the compiler uses exact mechanics, joint logistics and short-horizon market arithmetic
  to realize them. When a local choice changes future resources or production, carry that resulting
  state into the value comparison.*
- The boundary rule used on Sep 24-25: keep the compiler; move a decision from the compiler to the
  network only when evidence shows the compiler fails at it consistently for a conceptual reason
  (typically: its value appears on later days and the compiler has no continuation value). The
  boundary study (`docs/weakness_analysis/BOUNDARY.md`) applied it: "what to drop when over budget"
  qualified; overnight stock did not yet (the compiler's day boundary must be fixed first); routes,
  placement, workforce and within-day sale timing stay in the compiler.

Consequences of the split, which the contracts make explicit:

1. **Groups and counts, not tiles or ids.** Crops and animals identical except for location form a
   group, rebuilt from each dawn observation. The network outputs counts per group (how many to
   water, harvest, feed, ...); the compiler chooses which members and tiles. Members are exchangeable,
   so the output is small, and experts' days convert to it exactly.
2. **Exact labels, no silent fallback.** Every expert day is replayed in the exact engine and
   converted to its DayIntent by definition. The converter never guesses, clamps or defaults; a day it
   cannot represent fails the interface-coverage gate and stops training until the interface or a
   fixed-value rule is changed (with proof). Only two cases may be ignored (more than one land
   quadrant in a day; a discarded animal). Result on 1.45M expert days: 0 failures.
3. **Fixed values** (a field has only one sensible value in this state: day 29, impossible or
   worthless actions) are computed from the dawn state alone and used identically in dataset
   generation, training (no loss), decoding (never chosen) and compilation.
4. **Valid intents only.** Partitions sum to group sizes, care <= feed, fertilize <= retain, new animals
   + reserves >= animals in the shed at dawn, and the new crops and animals fit today's free sites.
   Training and decoding must enforce these, so the compiler never receives an impossible request.
5. **The compiler's objective is long-term margin**, not cash tonight. It must not stop at the first
   legal plan. Its value estimates are validated by continuing controlled alternatives with the same
   policy against reacting opponents; replay continuations against a fixed opponent are diagnostics.
6. **Failure is explicit and ordered.** A plan that cannot meet the intent drops requirements in a fixed
   order (land, new entities, collection, down to survival) and reports why; a failed plan never
   becomes an idle day.

## 2. The DayIntent (network output)

| Scope | Fields |
|---|---|
| Whole farm | new wheat / carrot / tomato / strawberry / melon counts (planted today and alive after tonight); new goose / cow / sheep counts; unplaced goose / cow / sheep after tonight (animals kept in the shed); buy the next land quadrant today (bool) |
| One-shot crop group (wheat, carrot, melon) | 9 counts partitioning the group: the 8 combinations of {water, fertilize, harvest} and `clear` (dig without harvest) |
| Ongoing crop group (tomato, strawberry) | retain after tonight, clear today, fertilize today (<= retain), harvest today; abandon = size - retain - clear |
| Animal group | feed, care (<= feed), collect |

Groups: existing crop groups keyed by (type, age, yield or held product, dry days, remaining
fertilizer days), existing animal groups by (species, age, held product, unfed days, care bonus),
sorted lexicographically; then one new group per crop type and species. New groups have no fields
beyond their count (the compiler waters new crops so they survive; new animals are fed as requested).
Retention of animals is not an output: feeding decides it (an animal with one unfed day escapes
tonight unless fed).

## 3. Implementation map

```
experiment/
  source/world.hpp        engine helpers on fast_game_engine/sim.hpp: cells, shed distance, Sim from an
                          observation, trace loading (load_replay / save_replay)
  source/intent.hpp/.cpp  DayIntent, groups (Schema), fixed-value rules, free sites, validate()
  source/convert.hpp/.cpp replay day -> DayIntent label (the coverage gate's converter)
  source/history.hpp      causal opponent market flow, visible supply, inferred opponent stock
  source/features.hpp     network inputs (shared by extraction and the live agent)
  source/dataset.hpp      training targets and fixed-value masks
  source/compiler.hpp/.cpp compile_day, CompileOptions, DayPlan, DayExecutor, choose_sales
  source/opp_prior.hpp    strong-player hourly sale prior (anticipation stack)
  source/lb_bridge.cpp    C API for Python (Local-LB / Kaggle)
  day_policy_local/       route solver (copy of the repository's day_policy + local options)
  agent/bc_opus/source/   the agent: network inference, decoding, sidecars, per-dawn loop
  tools/ scripts/         evaluation, extraction, training (EVALUATION.md, TRAINING.md)
```

## 4. API

### 4.1 DayIntent and groups (`source/intent.hpp`, namespace `dc10`)

```cpp
constexpr int OPTIONS = 9, CLEAR = 8;   // one-shot options 0-7 = bits water 4 | fertilize 2 | harvest 1; 8 = clear
constexpr int MAX_GROUPS = 110;
struct CropGroup   { uint8_t crop; int16_t age; int8_t yield, dry, fert_days; bool decaying, fresh; int size;
                     std::array<uint8_t, 100> cells; };
struct AnimalGroup { uint8_t species; int16_t age; int8_t held, unfed, bonus; bool fresh; int size;
                     std::array<uint8_t, 100> cells; };
struct Schema {       // groups and dawn facts, derived deterministically from the dawn observation
    int day, n_crops, n_animals; std::array<CropGroup, MAX_GROUPS> crops; std::array<AnimalGroup, MAX_GROUPS> animals;
    int unplaced_dawn[3], new_crop_group[5], new_animal_group[3], open_sites, land_sites; };
struct DayIntent {
    int16_t new_crop[5], new_animal[3], reserve[3]; bool buy_land;
    std::array<std::array<int16_t, 9>, MAX_GROUPS> options;                 // one-shot groups
    std::array<int16_t, MAX_GROUPS> retain, clear, fertilize, harvest;      // ongoing groups
    std::array<int16_t, MAX_GROUPS> feed, care, collect;                    // animal groups
    std::array<int8_t, 32> trim_order; int8_t trim_count, trim_next;        // decoder hints for budget trims
    std::array<std::array<float, 32>, 8> drop_loss; bool has_drop_loss;     // log-prob lost per removed unit
};
Schema describe(const agent::AgentObservation& dawn);
void size_new_groups(Schema&, const DayIntent&);         // new groups get the intent's new counts
bool option_fixed(const CropGroup&, int day, int option); // fixed-value rules (true = fixed to 0)
bool ongoing_fertilize_fixed(const CropGroup&, int day);
bool care_fixed(const AnimalGroup&, int day);
int  free_sites(const Schema&, const DayIntent&);         // open sites + tiles freed today + land bought today
std::string validate(const Schema&, const DayIntent&);    // empty = valid (incl. site capacity)
```

Index conventions: crops 0-4 = wheat, carrot, tomato, strawberry, melon; animals 0-2 = goose, cow,
sheep; products 0-8 = wheat, carrot, tomato, strawberry, melon, egg, milk, wool, fertilizer.

### 4.2 Label conversion (`source/convert.hpp`)

```cpp
enum class ConvertStatus { Ok, IgnoredLand, IgnoredDiscard, Failed };
struct Conversion { ConvertStatus status; std::string failure; Schema schema; DayIntent intent; int dropped_actions; };
Conversion convert_day(const std::vector<Sim>& states, const std::vector<std::array<Action,2>>& turns, int seat, int day);
Conversion convert_steps(const Sim* states, const std::array<Action,2>* turns, int steps, int seat);
int intent_distance(const Schema&, const DayIntent& a, const DayIntent& b);
```

`convert_steps` works on any trajectory (states[0] = dawn, states[steps] = after tonight's update), so
it also labels our own or search-improved games (expert iteration). `dropped_actions` counts actions
removed by fixed-value rules (e.g. fertilizer before a same-day harvest without water).

### 4.3 Network inputs and targets (`source/features.hpp`, `source/dataset.hpp`, namespace `bcopus`)

```cpp
constexpr int GLOBAL = 256, CROP = 48, ANIMAL = 32, GRID_CHANNELS = 24, FEATURES_VERSION = 4;
void global_features(const agent::AgentObservation&, const Schema&, const double rival[24][9], float out[256], int version);
void grid_features(const agent::PublicFarm&, int day, float out[24 * 100]);
void crop_features(const Schema&, const CropGroup&, float out[48], double price);
void animal_features(const Schema&, const AnimalGroup&, float out[32], double price);
// dataset.hpp: 12 global targets/masks, 14 crop targets/masks, 4 animal targets / 3 masks, counts 0-100
```

### 4.4 Agent (`agent/bc_opus/source/agent.hpp`, namespace `kag::agents::bc_opus`)

Implements the repository's `kag::agent::LocalAgent` concept (`agents/common/api/agent_api.hpp`):
`static AgentInfo info()`, `void reset(const AgentInit&)`, `void act(const AgentObservation&, const
DecisionBudget&, Action&)`; manifest `agent/bc_opus/agent.json`.

```cpp
dc10::DayIntent decode_intent(const Model&, const agent::AgentObservation& dawn, const dc10::History&,
                              std::string* invalid = nullptr, std::vector<float>* global_logits = nullptr,
                              const DecodeKnobs& = DecodeKnobs::from_env());
class Agent {
  public:
    std::string model_path;              // set before reset; empty: BC_OPUS_MODEL, else the build default
    DecodeKnobs knobs;                   // quantiles, land bias, service pushes (experiments, search)
    void set_time_left(double seconds);  // remaining overage; drives the time valve
    dc10::CompileOptions& compile_options();          // per-dawn search over compiler variants
    const std::vector<DayReport>& reports() const;    // per dawn: status, fallback, returns, hires, ms, reason
    const dc10::DayIntent& last_intent() const;
    void observe(const AgentObservation&, const Action& submitted);  // feed history without acting
    Agent(const Agent&);                 // copy for rollouts (own solver scratch state)
};
```

Per-dawn loop in `Agent::act` (hour 0): choose the opening model/style for days < opening days ->
`decode_intent` -> set the time valve -> `compile_day` -> herd reach / crop reach re-decodes and
re-compiles (keep the first larger plan that still compiles in full) -> `DayExecutor::start`. If no
schedule compiles, the day runs markets only. Every step: `History::observe`, `DayExecutor::act`.

### 4.5 Day compiler (`source/compiler.hpp`, namespace `dc10`)

```cpp
enum class CompileStatus { Ok, InvalidIntent, NoSchedule, Unfunded };
enum Fallback : int { KeepAll = 0, NoLand, NoNewEntities, NoCollection, SurvivalOnly };
struct CompileOptions { dp::SolveOptions solve; ... };  // every field: ARCHITECTURE.md section 5
struct DayPlan {
    CompileStatus status; int fallback; int return_percent; int funding; bool stress_funded; std::string reason;
    dp::DayInput input;                  // what was sent to the route solver
    Action actions[24];                  // worker actions + the solver's purchase and hire orders, per hour
    int receipts[24][9];                 // products reaching the shed by hour
    int night_carried, night_items[9], keep_overnight[9], hires, new_animals[3], hours;
    double compile_ms, shortfall;
};
DayPlan compile_day(const agent::AgentObservation& dawn, const History&, const DayIntent&,
                    const CompileOptions&, dp::Solver&);
class DayExecutor {                      // follows a plan hour by hour, decides sales from the actual market
    void start(const AgentObservation& dawn, const History&, const DayPlan&, const CompileOptions&);
    void act(const AgentObservation&, Action&);
};
void choose_sales(const AgentObservation&, const int incoming[24][9], int hours, double hold_discount,
                  const int reserve[9], int night_room, const double rival[24][9], const int hold_supply[9],
                  int sell[9], bool tie_now = false, bool melon_tie_now = false);
```

### 4.6 Opponent history (`source/history.hpp`)

```cpp
class History {
    void observe(const AgentObservation&, const Action& submitted);   // once per step, before acting
    void expected(int today, int days, double out[24][9]) const;      // opponent net sales per hour, last `days` days
    const std::array<int, 9>& opponent_stock() const;                 // observed harvests - inferred sales
    double sell_through(int today, int p, int days, double prior, double weight = 5) const;
    void hour_profile(int today, int p, double out[24]) const;
    int dawn_visible(int day, int p) const; int dawn_stock(int day, int p) const; int sold_on(int day, int p) const;
};
void visible_supply(const AgentObservation&, int out[9]);             // opponent's ripe crops + held animal products
```

Opponent trades are never observed directly: inferred sales = market inventory change + known
town/shop demand - our actual fills (our fills = the shed change the worker phase does not explain,
computed by replaying the worker phase in an engine copy). Using our requested quantities instead
turns our failed orders into fake opponent trades.

### 4.7 Route solver (`day_policy_local/source/policy.hpp`, namespace `kag::agents::day_policy_contract`)

```cpp
enum Event : uint8_t { Water = 1, Fertilize = 2, Harvest = 4, Clear = 8, Feed = 16, Care = 32, CollectFertilizer = 64 };
struct DayInput {
    Tile grid[100]; int shed[N_ITEMS], seeds[5]; int hours = 24;
    int buy_seeds[24][5], buy_animals[24][3], buy_wheat[24], buy_fertilizer[24];  // purchases per hour
    int land_hour = -1;                                  // at most one quadrant (NE, SW, SE order)
    uint8_t events[100];                                 // requested events per existing tile
    NewProduct establish[100]; int establish_count;      // new crops/animals: product + events; solver picks tiles
    int returns[24][9];                                  // at least N units of product returned by hour h (cumulative)
};
struct SolveOptions { SearchEffort effort; int max_hires = 13; int variants = 4; bool minimize_hires = true;
                      bool opportunistic_hire_reduction; int minimize_variants; int min_hires; int max_executions;
                      bool feasibility_first; ... };
struct SolveResult { SolveStatus status; Action schedule[24]; DayState state; int hires, receipts[24][9],
                     production[9], attempts, executions; double microseconds; };
class Solver { public: SolveResult solve(const DayInput&, const SolveOptions& = {}); };
```

The solver handles tiles, placement, worker routes, hires and market slots for purchases and hires;
the caller (the compiler) handles affordability, market decisions and shed capacity (SPEC). Local
additions to the repository version: `min_hires`, `max_executions`, `feasibility_first`, profiling.

### 4.8 Bridge (`source/lb_bridge.cpp`) and Python wrapper (`agent/main.py`)

```c
void* opus_new(const double* config, int n, int seat);   // config: episodeSteps, boardSize, startingMoney,
                                                          // maxMarketOrdersPerTurn, turnsPerDay, shedCapacity,
                                                          // weedSpawnChance, townShopUnlockInterval,
                                                          // townShopSellInterval, townCenterSellInterval, farmHandCostMult
int   opus_act(void* game, const double* fields, int n, int32_t* out, int cap);   // returns ints written
void  opus_delete(void* game);
```

`fields` = player, step, day, hour; per farm: money, unit count, quadrants, hires today, unit
positions, 100 tiles x 13 values; own shed (12 items), seeds (5), per-worker inventories; market
inventory and prices (9 each); unlocked shops; optional trailing remaining overage. `out` = unit
count, order count, then (op, item, count) triples for units and orders. The exact layout is
`_fields` / `_action` in `main.py` (same as `scripts/lb_play.py` and `fast_game_engine/export_trace.py`).

## 5. Where the implementation differs from the design

| Design says | Implementation (Sep 25) |
|---|---|
| Replan every hour from the observed state | Worker actions are fixed at dawn; only market orders (sales, and the plan's purchases/hires) are decided each hour; a runtime cash guard protects purchases |
| Economic value = expected long-term margin; validated continuation values | Fixed constants: held stock 0.95 x tomorrow's price, next-dawn cash reserve, 1 wheat per animal, trim order, $2 per returned unit; no learned value |
| Search for the best legal plan within budget | Fallback ladder with funding variants and return attempts; the first plan that passes is kept, except herd/crop reach (try larger plans) |
| Wheat/fertilizer trades for profit | Not implemented: inputs are bought for needs and surplus is sold |
| Stress forecast for safety, expected forecast for value | Both exist; the stress case assumes the opponent sells all visible output at hour 2 |
| Latency: average initial compile <= 0.5 s | ~0.7 s per dawn on average on a loaded machine (21 s per game), worst dawn ~4 s |
| The compiler never digs one-shot crops except by request | It digs one-shot crops that turn into weeds today when their tiles are needed (same end state; accepted by Pavel) |

These gaps are where most remaining weaknesses come from (WEAKNESSES.md sections 2, 3, 6, 7).
