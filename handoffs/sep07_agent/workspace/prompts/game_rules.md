# One Turn

Each player can submit:

- 1 action for the farmer;
- 1 action for every hired worker already active;
- up to 10 market orders.

The game executes them in this order:

1. For each player: farmer first, then hired workers in `hands` list order. Workers have list positions, not IDs. Each action changes the state immediately.
2. Market orders run by list position: both players' first orders, state update; both second orders, state update; and so on through order 10. Quantity trades run one item at a time. At each item step, both price quotes are calculated before either trade changes inventory. Failed orders are not retried.
3. Town and shops consume products, crops decay, then the day-end update runs when needed.

Players see the new state next turn.

# Workers

A worker means the farmer or a hired worker. Each worker takes one action per turn, can share a tile with other workers, and can carry unlimited items.

- `NORTH`, `SOUTH`, `EAST`, `WEST`: move one tile. Moving off the map fails.
- Farm actions affect the tile where the worker stands: `PLANT`, `WATER`, `HARVEST`, `FERTILIZE`, `DIG`, `BUILD_COOP`, `BUILD_PASTURE`, `PLACE`, `FEED`, `CARE`, `COLLECT_FERTILIZER`.
- Shed actions require one of the four center tiles beside the shed: `PICKUP`, `DROP`, `PLACE`.
- `PASS`: do nothing.

Workers may move across locked land, but farm actions there fail. Shed actions still work from a locked shed-access tile.

`HIRE` is a market order. You may hire at most 10 workers per turn or 240 per 24-turn day, if affordable; there is no separate worker limit. Successful hires cost $1, $1, $2, $3, $5, … each day. The price sequence resets daily, and all hired workers disappear at day end. A new worker starts acting next turn.

A new worker appears on the least occupied shed-access tile. Ties use NW, NE, SW, SE order. Locked tiles count. If the farmer is still on NW, the first hire appears on NE.

# Shed

The shed sits between the four center tiles; it is not a grid tile. A worker can access it from any of those four tiles, even when that tile is locked.

The shed privately stores at most 100 total items: crops, animal products, fertilizer, bought products, and unplaced animals. Seeds use separate unlimited storage and take no shed space. Only items in the shed can be sold.

In `[n]`, `n` is optional and defaults to 1.

- `PICKUP item [n]`: move up to `n` items from the shed to that worker.
- `PLACE item [n]`: move up to `n` carried items into available shed space; the rest stay carried.
- `DROP`: move everything possible into the shed; destroy overflow.
- Day end: carried items enter the shed automatically; overflow is destroyed.

Worker actions happen before market orders, so items deposited this turn can be sold this turn. Harvested items belong to the worker who collected them. That worker needs a later action to deposit them.

# Crops

Crop age is the number of days since planting; planting day is age 0.

Water a new crop on age 0 or it becomes a weed that night. Afterward, one dry day is safe; a second consecutive dry day turns it into a weed that night. Watering resets the dry-day count. A crop can be watered once per day.

Seeds have the fixed prices in the tables. They remain in unlimited, shared seed storage until used and need no `PICKUP`.

`PLANT crop` uses 1 seed and plants where the worker stands. The land must be owned, and the tile must contain no crop, weed, structure, or animal. Other workers do not block planting. A failed plant action uses no seed.

The planting worker cannot also water that turn. A later worker may water the new crop in the same turn, or any worker may do so later that day. Seeds bought this turn become usable next turn.

If same-turn `PLANT` requests for one crop exceed its available seeds, every request for that crop fails. Requests aimed at invalid tiles still count.

Table terms:

- `Base`: price when market inventory is 10,000.
- `Stored-yield cap`: most yield that can wait on the crop; extra yield is lost.
- `HARVEST`: move all stored yield to the harvesting worker. Deposit it in the shed before selling.
- `First yield-loss age`: after that age's hour-0 actions, stored yield loses 1; it then loses 1 after h2, h4, and so on. At 0, the crop becomes a weed. Water and fertilizer cannot stop this.

## One-shot crops

A one-shot crop begins with 1 stored yield.

On each age listed under `Water adds yield at ages`, `WATER` immediately adds 1 stored yield, or 2 if fertilizer is already active. Watering at any other age only keeps the crop alive.

`First harvest` is the earliest age when `HARVEST` works. Harvesting collects all stored yield, removes the crop, and leaves the tile empty. No `DIG` is needed.

| Crop | Seed | Base | First harvest | Water adds yield at ages | Stored-yield cap | First yield-loss age |
|---|---:|---:|---:|---:|---:|---:|
| Wheat | $10 | $25 | 2 | 2–4 | 6 | 5 |
| Carrot | $20 | $35 | 2 | 2–3 | 4 | 4 |
| Melon | $80 | $250 | 10 | 6–12 | 6 | 13 |

Wheat example:

- Plant on age 0: stored yield is 1.
- Water on age 0: yield stays 1; this water only prevents weeds.
- Skip age 1: the crop survives.
- On age 2, harvest before watering for 1, or water first and harvest next turn for 2.
- Fertilize on age 2 before watering, then water on ages 2–4: yield becomes 1 → 3 → 5 → 6, stopping at the cap.
- If wheat holds 4 at age 5, harvesting at h0 still gives 4. Otherwise, decay after h0/h2/h4/h6 leaves 3/2/1/weed.

After `HARVEST`, the tile is immediately empty. A later worker can use any action allowed on an empty owned tile, such as `PLANT`, `BUILD_COOP`, or `BUILD_PASTURE`. A newly planted crop must still be watered before day end.

## Ongoing crops

An ongoing crop produces on each listed age. Each production adds 1 stored yield.

Production adds 2 instead if, on the previous day, the crop was watered and fertilizer was active. Because fertilizer lasts three days, it may have been applied 1, 2, or 3 days before production.

Example: tomato produces at age 8. Water it at age 7. Fertilizer applied at age 5, 6, or 7 is active and gives the age-8 production +2.

`HARVEST` collects all stored yield and resets it to 0, but the crop remains and can produce again.

| Crop | Seed | Base | Production ages | Stored-yield cap | First yield-loss age |
|---|---:|---:|---|---:|---:|
| Tomato | $50 | $60 | 8, 9, 10, 11 | 4 | 12 |
| Strawberry | $100 | $120 | 10, 12, 14, 16 | 4 | 17 |

To make its tile empty, use `DIG`; any unharvested yield is lost. Harvest first if you want that yield. After `DIG`, a later worker can use any action allowed on an empty owned tile. A newly planted crop must still be watered before day end.

# Animals

Build a coop or pasture on an empty owned tile with `BUILD_COOP` or `BUILD_PASTURE`; building costs only that worker action.

Buy an animal into the shed. A worker must `PICKUP` the animal, carry it to the correct empty structure, and use `PLACE`. Animals cannot be sold, and `DIG` cannot remove a placed animal; only escape removes it.

Animal age is the number of days since placement. `First production age` is the age when its first product becomes available; the game creates it during the previous night's update. `Interval` is the number of days between later productions; `Held cap` is the most product waiting on the animal.

| Animal | Cost | Structure | First production age | Interval | Held cap | Product |
|---|---:|---|---:|---:|---:|---|
| Goose | $300 | Coop | 4 | 1 day | 4 | Egg |
| Cow | $400 | Pasture | 8 | 2 days | 6 | Milk |
| Sheep | $500 | Pasture | 6 | 3 days | 6 | Wool |

`FEED` requires a worker on the animal and consumes 1 carried wheat. An animal can be fed once per day. After two consecutive unfed days, it escapes at day end; its structure remains.

Every scheduled production adds 1 product, up to the held cap.

`CARE` costs no item and works once per day. If `CARE` and `FEED` both happen that day, their order does not matter: the animal banks +1 for its next scheduled production after today.

At each night update, the order is:

1. If production is scheduled, add the base 1 plus previously banked bonuses—but bonuses apply only if the animal was fed that day. Then clear the old bonus bank.
2. If the animal was both cared for and fed that day, bank a new +1 for a later production.

Thus care earned today never increases production processed tonight. Bonuses accumulate until the next scheduled production. If the animal is not fed on that production day, the old bonuses are lost. The held cap still applies.

Example—a mature goose produces daily. `CARE` + `FEED` on day 10 banks +1 after that night's production. Feed it on day 11, and the day-11 night production adds 2 instead of 1; those units are visible on day 12. Care on day 11 starts the next bonus.

`HARVEST` moves all held product to the harvesting worker, resets held product to 0, and leaves the animal. Deposit the product in the shed before selling.

# Fertilizer

Fertilizer comes from two places:

- `BUY_PRODUCT FERTILIZER`: bought fertilizer enters the shed.
- Animals: every surviving animal makes 1 fertilizer available at day end, even without feed or care. An animal holds at most 1 available fertilizer; missed days do not accumulate.

A worker gets fertilizer with `PICKUP` at the shed or `COLLECT_FERTILIZER` while standing on an animal. Both put fertilizer in that worker's carried inventory.

To fertilize a crop, the worker stands on it and uses `FERTILIZE`. This consumes 1 carried fertilizer. It adds no yield immediately.

Fertilization is active on the application day and the next 2 days. Example: applying it on day 5 covers days 5, 6, and 7. Reapplying extends the end date but does not stack the bonus.

- One-shot crop: while fertilizer is active, an eligible `WATER` adds 2 yield instead of 1. `FERTILIZE` must execute before `WATER`.
- Ongoing crop: production adds 2 instead of 1 when the crop was watered and fertilizer was active on the previous day. `WATER` and `FERTILIZE` may execute in either order that day.
- Stored-yield caps still apply.

# Shops

Exactly one shop unlocks at the start of days 3, 6, 9, 12, 15, 18, 21, and 24: eight guaranteed shops in a 30-day game. Each unlock independently chooses one of the eight types with equal probability (1/8), with replacement. Types may repeat; every instance remains until game end.

Every 4 turns, each shop instance removes these products from the shared market inventory:

| Shop | Products removed |
|---|---|
| Bakery | 1 egg + 1 wheat |
| Pizza Shop | 1 milk + 1 tomato + 1 wheat |
| Brunch Spot | 1 egg + 1 wheat + 1 strawberry |
| Yarn Store | 2 wool |
| Ice Cream Shop | 1 strawberry + 1 milk + 1 wheat |
| Pet Cafe | 2 carrot |
| Smoothie Shop | 1 strawberry + 1 milk |
| Farmers Market | 1 wheat + 1 carrot + 1 tomato + 1 strawberry |

One shop never increases its own rate. Total demand grows because more shops appear, and duplicate shops consume separately. No shop consumes melon or fertilizer.

# Prices

Each product has one shared market-inventory number. With fixed game settings, that number fully determines its price. Shed and worker inventories do not count.

Every product starts at inventory 10,000, where its price equals `Base`. All products below have dynamic prices and can be sold; players can buy only wheat and fertilizer.

| Product | Base | Player can buy |
|---|---:|---|
| Wheat | $25 | Yes |
| Carrot | $35 | No |
| Tomato | $60 | No |
| Strawberry | $120 | No |
| Melon | $250 | No |
| Egg | $50 | No |
| Milk | $160 | No |
| Wool | $200 | No |
| Fertilizer | $100 | Yes |

Only these events change market inventory:

- Successful player `SELL`: +1 per sold item, except a sale at the $1 price floor adds 0.
- Successful player `BUY_PRODUCT`: −1 per bought wheat or fertilizer.
- Town center: −1 from every non-fertilizer product every 24 turns, during hour 0 after market orders—not at day end.
- Shops: remove the products in the Shops table every 4 turns.

Lower inventory raises price; higher inventory lowers it. Price cannot fall below $1. Each item in a quantity order receives a new quote after earlier items change inventory, so later items and later market orders use updated prices. Prices also update after town/shop consumption.

Seed and animal prices are separate and fixed. Buying them does not change product inventory. Animals cannot be sold; their eggs, milk, and wool can.

Bought wheat can feed animals. Buying also raises the shared wheat price for both players, but immediately reselling into an otherwise unchanged market yields no profit.
