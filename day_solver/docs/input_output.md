# Input and output specification

The native parser and validator are authoritative. The
[JSON Schema](../schemas/day_problem_v3.schema.json) checks structure and simple
ranges; replay additionally checks the game, sequencing, inventories and exact
endpoints. Top-level fields listed below are required in JSON. Unknown fields are
rejected, including routes, owner hints, cash, sales and original schedules.

## Public v3 input

| Field | Type | Meaning |
| --- | --- | --- |
| `format_version` | integer, exactly 3 | Fixed schema version |
| `worker_count` | integer, 1–40 | Exact total workers including the farmer and all hires |
| `start` | object | `managed_tiles`, `shed[12]`, `seeds[5]`, optional `shed_capacity` |
| `end_tiles` | array | Exactly one `{tile, state}` for every managed tile |
| `tile_work` | array | `{tile, actions}` for each tile requiring work; omit idle tiles |
| `buy_schedule` | array | Fixed purchases and hires, each with hour and order slot |
| `shed_availability` | 24 arrays of 12 integers | Cumulative external withdrawals by the end of each worker phase |
| `end_shed` | 12 integers | Exact shed after hour 23 purchases and automatic cargo transfer |
| `end_seeds` | 5 integers | Exact shared seed inventory at day end |

All inventory inputs are integers in `[0, 2147483647]`. Native cumulative
arithmetic uses signed 64-bit counts. Hours run from 0 through 23. The farmer
starts at `(4,4)` with empty cargo. There are no hired workers at hour 0; they
appear through the fixed hire orders. This is a day-start solver, not an
arbitrary mid-day-state scheduler.

Optional `start.shed_capacity` is an integer from 0 through 32766. Use 100 for
the standard game. Omission preserves unlimited storage for historical inputs.
With explicit capacity, `end_shed` is the exact retained inventory after losses,
not production plus purchases minus consumption and sales. PLACE retains
excess cargo; DROP and automatic night transfer destroy overflow in game order.
The solver validates every returned schedule against this contract. Capacity
handling can still return UNKNOWN within its soft search budget.

Item indices are fixed:

| ID | Item | In seed array? |
| ---: | --- | --- |
| 0 | Wheat | Yes |
| 1 | Carrot | Yes |
| 2 | Tomato | Yes |
| 3 | Strawberry | Yes |
| 4 | Melon | Yes |
| 5 | Egg | No |
| 6 | Milk | No |
| 7 | Wool | No |
| 8 | Fertilizer | No |
| 9 | Goose | No |
| 10 | Cow | No |
| 11 | Sheep | No |

### Tiles and states

`start.managed_tiles` is an array of `{ "x": 4, "y": 4, "state": {...} }`.
Coordinates are unique integers from 0 to 9. X grows east; Y grows south.
Every `tile` reference is an index into this array, not `y * 10 + x`.
Omitted squares may be crossed but may not receive farm actions. Include an
idle tile if its deterministic end state must be checked.

Each start and end `state` has exactly these fields:

| Field | Meaning |
| --- | --- |
| `kind` | `empty`, `weed`, `locked`, `crop`, `pasture`, or `coop` |
| `crop` | Crop ID 0–4, or -1 when absent |
| `animal` | Animal ID 9–11, or -1 when absent |
| `age_days` | Nonnegative relative crop/animal age |
| `stored_units` | Waiting crop yield or animal product; bounded by that entity's cap |
| `consecutive_dry_days` | Consecutive dry days for crops; consecutive unfed days for animals |
| `pending_care_bonus` | Nonnegative banked animal-care bonus |
| `fertilizer_days_remaining` | Positive means active today; application gives 3; night decrements it |
| `watered_today` | Boolean |
| `fed_today` | Boolean |
| `cared_today` | Boolean |
| `fertilizer_available` | Boolean: an animal holds collectible fertilizer |

Relative counters must fit nonnegative signed 32-bit integers. Empty, weed and
locked states have crop/animal -1, all counters zero and all flags false. Crop
states have no animal-only state. A goose requires a coop; cows and sheep
require pastures. Empty structures have no animal state. An end state describes
the next morning after deterministic daily updates, before random weed spawning.
Daily flags therefore normally reset, ages advance and stored output may change.
Deterministic decay and weeds caused by missed watering remain required.

### Ordered tile work

Every action has `{op, arg, quantity, output_item, output_quantity}`. `quantity`
is always 1: each entry is one worker action. Harvest yield is instead recorded
in `output_quantity`. Actions are ordered within their tile; different tiles
may interleave. Later workers may execute a successor on the same tile within
the same hour, if the sequential game rules allow it.

| `op` | `arg` | `output_item`, `output_quantity` |
| --- | --- | --- |
| `plant` | Crop ID | -1, 0 |
| `place` | Animal ID | -1, 0 |
| `harvest` | Harvested product ID | Same ID, exact positive quantity |
| `collect_fertilizer` | -1 | 8, 1 |
| `water`, `fertilize`, `dig`, `build_pasture`, `build_coop`, `feed`, `care` | -1 | -1, 0 |

`place` here means placing an animal on its structure. Shed deposits, pickups,
movement and passes are chosen by the solver and must not appear in tile work.
Duplicate tile-work entries or empty `actions` arrays are invalid. Do not put
the full day yield in every harvest entry; each entry fixes its own exact output.

### Purchases and hires

An entry is `{ "hour": 0, "order_index": 0, "op": "buy_seed", "item": 2,
"quantity": 1 }`. Hours are 0–23, order indices are 0–9, and each `(hour, slot)`
is unique. The array is interpreted by hour and slot, not by incidental array order.

| `op` | `item` | `quantity` |
| --- | --- | --- |
| `hire` | -1 | Exactly 1 |
| `buy_seed` | Crop ID 0–4 | Positive integer |
| `buy_product` | Wheat 0 or fertilizer 8 | Positive integer |
| `buy_animal` | Goose 9, cow 10 or sheep 11 | Positive integer |
| `buy_land` | Quadrant 1 northeast, 2 southwest, 3 southeast | Exactly 1 |

Exactly `worker_count - 1` hire entries are required, even for workers hired in
hour 23 who never act that day. Workers act before that hour's market orders.
Seeds, goods and land bought at hour H become usable for work at H+1. A hire
first acts at H+1. Spawn location is the least occupied shed-access square,
with ties `(4,4)`, `(5,4)`, `(4,5)`, `(5,5)`; previous workers' current positions
matter. Land purchases follow northeast, southwest, southeast order; the
specified quadrant is input metadata, while the emitted order is `BUY_LAND`.

There are no prices, cash balances or sale orders in this input. The caller is
responsible for purchase affordability and market interaction.

### Availability is a withdrawal requirement

Let D[h,item] be `shed_availability[h][item]`, and D[-1,item] = 0. Each item's
column must be nondecreasing. After all worker actions at hour h, reserve and
remove **D[h,item] - D[h-1,item]** from the shed. Those units cannot be reused
for feed, fertilizer or a later deadline. Initial shed stock can satisfy these
withdrawals; it does not create an extra credit after it has been removed.

Example: start with two wheat; require D = 2 from hour 3, then D = 5 from hour
10 onward. The caller removes two wheat at hour 3 and three more at hour 10.
The schedule must produce/deposit or buy the additional three in time. A purchase
at hour 10 cannot satisfy its hour-10 worker-phase withdrawal.

Seeds are shared storage with no pickup action. Wheat, animals and fertilizer
must be carried before a worker uses them. Earlier production on a route may
supply later consumption; earlier deposits may supply another worker. All
identities are fungible. End shed excludes the reserved withdrawals and includes
automatic transfer of all remaining worker cargo after hour 23.

## Output

The CLI writes `input.json`, `report.json` and, only on success, `schedule.json`.
An accepted report has `status: "SCHEDULE"`, elapsed `seconds`, `input_sha256`,
`winning_stage`, diagnostic `attempts`, an `exact_report`, and a schedule filename.
`UNKNOWN` has no schedule. Diagnostic inner `INFEASIBLE` statuses refer to
restricted candidate models and do not prove the public day is infeasible.
The report explicitly makes no infeasibility proof claim.

The schedule is an array of exactly 24 objects, in hour order:

```json
{"units": [["PASS"], ["PICKUP", "WHEAT", 2]],
 "orders": [["BUY_SEED", "TOMATO", 1]]}
```

`units[0]` is the farmer; subsequent entries follow hire order. There must be
exactly one action for every worker active at that hour. `orders` preserves the
fixed order slots, using `NONE` in unused interior slots when needed. Item names
in output are uppercase strings, unlike numeric item IDs in the input.

Unit commands are `PASS`, `NORTH`, `SOUTH`, `EAST`, `WEST`, `DROP`, `WATER`,
`HARVEST`, `FERTILIZE`, `DIG`, `BUILD_COOP`, `BUILD_PASTURE`, `FEED`,
`COLLECT_FERTILIZER`, `CARE`; `PLANT` adds its crop name; `PLACE` and `PICKUP`
add an item name and may include a quantity. `PLACE` may be an animal placement
or a shed deposit depending on location/item. Omitted quantity means one.
Market commands are `HIRE`, `BUY_LAND`, and item/quantity purchases.

The typed output is `optional<array<kag::Action,24>>`. Use only `n_units` entries
of `units` and `n_orders` entries of `orders`; trailing fixed-array storage is
not part of the output. Commands have already been finalized. Movement and
passes can be arbitrary when several feasible solutions exist; their count is
not an optimization promise. See the complete
[tomato input](../examples/tomato/input.json) and [sample output](../examples/tomato/schedule.json).
