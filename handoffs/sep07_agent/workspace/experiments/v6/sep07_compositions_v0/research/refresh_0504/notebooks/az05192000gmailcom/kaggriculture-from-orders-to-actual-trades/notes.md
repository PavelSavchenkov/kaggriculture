# Kaggriculture | From Orders to Actual Trades

**Reconstruct what actually traded and where the money went.**

This notebook replays Kaggriculture's market settlement one unit at a time, accounting
for partial fills, changing prices, shed capacity, and available cash. It turns a replay
into a revenue-and-cost ledger, then checks that ledger against both players' recorded
cash balances on every turn.

Why reconstruct the trades? A bot can request a sale of 100 units with only three in
its shed. A purchase can stop when cash runs out. And within a single order, successive
units can trade at different prices. The ledger follows those fills through settlement.

You get:

- Sales revenue, units sold, and realized average prices for each product.
- Spending on seeds, livestock, feed, fertilizer, labor, and land.
- Daily cash-flow charts and a breakdown of spending by category.
- Turn-by-turn balance checks that flag unexplained changes in cash.

During play, cash changes through `SELL`, `BUY_SEED`, `BUY_PRODUCT`, `BUY_ANIMAL`,
`HIRE`, and `BUY_LAND`. Farmer and hand actions run first, so the reconstruction also
applies their changes before processing the market queue.

The demo runs a simple farmer against the built-in starter. See sections 5 and 6 for
the accounting results, or section 8 to analyse your own replay. Full private
observations for both players are required.


## 1. The price curve

For a given set of market parameters, a product's sell quote depends on its current
market inventory. Under the default settings, every product starts at `I0 = 10,000`.
Sales above the $1 floor add stock; town consumption and purchases remove it. Prices
fall as inventory rises and increase as it falls, subject to rounding and the floor.

We use `market_price` from the installed engine. Replay analysis merges any
`marketParams` overrides with that version's defaults. The table below shows the
**default parameters**, independently of the demo episode.

`T` is a reference production quantity for a 5x5 field over 24 days, assuming optimal
watering and no fertilizer. The engine discounts animal quantities for feed overhead.
The `I0+T` column is the sell quote **at that inventory**, rather than the average
price received for selling `T` units.

Before the floor is reached, the T-th unit sold from `I0` is quoted at `I0+T-1`.
Floor-price sales add no inventory, so an actual sequence of sales may never reach
`I0+T`.

At `I0+T`, wheat and eggs retain about 80% of their base price; melon, strawberry,
milk, and wool are at the $1 floor. Section 7 compares the default curves in more detail.

## 2. A demo season

The demo farmer gives the accounting examples a mix of expenses. It buys seeds,
raises a goose, purchases another quadrant, and hires a hand on mornings 2 through 26
when it has enough cash. Its movement and work priorities are deliberately simple.

This example illustrates the ledger; its score is not a benchmark for competitive play.

## 3. Check prices in the demo episode

Each observation records market inventory and sell quotes. We recompute every quote
using the installed engine's price function and the episode's resolved parameters,
then compare the result with the recorded value.

This checks the price inputs before we reconstruct the trades. On an episode generated
locally by the installed engine it cannot fail; it earns its keep on a downloaded replay,
where a different engine version or overridden market parameters would show up here.
Snapshot quotes describe the market at that moment; later units in an order may trade at
different prices.


The next table describes the market conditions in this demo: each product's lowest,
highest, and final recorded quote, plus the fraction of observations above its base
price. It includes products neither player sold. These quotes provide context;
realized revenue comes from the fills reconstructed in sections 4 and 5.


In this demo, most products were quoted above base for much of the season. The farms
supplied relatively little while the town continued to consume stock; purchases also
reduced wheat inventory. Fertilizer ended below base.

Production, purchases, and shop openings shape each episode's price path. The next
step is to follow both players' orders through that market and calculate what they
actually earned and spent.


## 4. Replaying the market queue

The engine processes corresponding positions in both players' order lists together.
Within each position, it quotes one unit for each eligible order from the same market
snapshot, attempts both fills, and then quotes the next units from the updated inventory.

Three details matter:

- A sale uses the quote before adding the unit. A product purchase uses the quote at
  inventory minus one. A completed buy-and-resell sequence has zero net cash flow
  when no other trades or town consumption intervene.
- Sales at the $1 floor do not add market inventory.
- When an order can no longer fill, its remaining quantity is abandoned. Later orders
  keep their original positions in the queue.

`HIRE` and `BUY_LAND` run once at their queue position, before that position's unit loop.
Shed quantities, capacity, and available cash determine which trades can fill.

First, check a fertilizer purchase followed by a resale with no intervening market
activity. Then check that an empty shed blocks a sale and a full shed blocks a purchase.

## 5. Reconstruct the cash flows

The key input is the shed **when the market starts**, not just the shed in the previous
observation. Unit actions run first. We copy each player's previous farm and private
state, then apply the farmer's action followed by the hands' actions using the installed
engine. This includes changes within a turn, such as a hand placing an animal in a
structure the farmer has just built. The original replay is not modified.

We then replay both players' recorded market orders, recording quantities and cash
only when units fill. This distinguishes a large request that partly filled from a
large sale, and a rejected purchase from an expense.

Finally, we compare the reconstructed ending balances with the recorded balances on
each turn. The report includes signed error, total absolute error, the largest
absolute error on one turn, and the number of player-turns with a mismatch. Positive
and negative errors can cancel in the signed total, so the absolute errors matter too.


For this demo, every player's reconstructed **net cash change per turn** matches the
recorded change. That is a useful consistency check, not proof that every individual
trade is correct: offsetting errors within a turn can still escape this check.

Interpreting the categories also requires care. For example, the `feed` bucket records
wheat purchases, including any wheat later resold rather than fed to an animal.

The table lists sales revenue first, followed by spending categories.

`revenue gap vs base` equals units sold times base price, minus realized revenue.
A positive value means sales earned less than the base-price reference; a negative
value means they earned more. Both players' trades and town consumption affect this
gap, so it does not isolate the effect of your own selling.

## 6. Revenue and spending over the season

The daily chart shows when player 0 received sales revenue and paid expenses. The next
chart groups spending by category and gives total revenue in the title for context.

## 7. Prices as supply accumulates

These charts use the installed engine's **default parameters**, independently of `A`.
For each product, we start at `I0` and calculate the price of successive units sold.

The example assumes one seller, no town consumption, and unlimited shed capacity.
It illustrates the price curve rather than an executable sales schedule. It also
excludes production costs, so it does not measure profit or identify an optimal volume.

## 8. Run it on your own episodes

Use a replay with recorded actions, public farm state, and full private observations
for both players. Missing private fields raise an error because shed contents cannot
be reconstructed reliably without them.

Choose one of these inputs after running the function definitions above:

```python
# A downloaded replay attached as a dataset.
import json
with open("/kaggle/input/<your-dataset>/<episode>.json") as fh:
    A = analyse(json.load(fh))

# Alternatively, generate an episode locally.
env = make("kaggriculture", configuration={"seed": 7})
env.run(["/kaggle/working/my_agent.py", "starter"])
A = analyse(env.toJSON())

print(A["report"])
pnl(A["ledger"], player=0, params=A["params"])
```

After replacing `A`, rerun the P&L cell and the cash and spending charts in sections
5 and 6. Each chart obtains its episode data from `A`; the player labels are seat
numbers. Section 3 remains a summary of the original demo, and sections 1 and 7 show
default price curves.

**Check `A["report"]` before interpreting the tables.** Zero mismatched player-turns
means the reconstructed net cash changes match the recorded ones. It does not certify
every individual trade. Nonzero `absolute` or `worst_turn` values indicate a discrepancy
to investigate, including possible engine-version differences or a replay-model error.

**Version and data requirements.** The episode must carry both players' full private
state and the tile grid at every step; unit actions are replayed against the farm, so a
replay trimmed to save space raises an error rather than guessing. If your stored copy
drops the grid, regenerate the episode from its seed (`info.seed` in a downloaded replay;
`configuration.seed` is usually null), the same configuration, the recorded actions, and a
matching engine version, then check the regenerated observations against the fields you
kept. This notebook is
tested with `kaggle-environments==1.32.7`. It reads the episode's market, capacity, hiring, board,
and timing settings, but uses the installed engine's unit-action helper, price function,
and cost constants. Replays from other engine versions may behave differently. The
unit-action helper is an internal API, so upgrading the package requires revalidation.

## What the tables leave out

The ledger records cash, which determines the final score. It does not value unsold
inventory or measure the return an alternative plan could have earned. Seed, land,
and animal purchases appear as cash outflows when paid; these tables do not allocate
their costs across individual products or days of use.

Idle land and missed production therefore need separate investigation. Start with the
reconciliation report, inspect spending and realized prices, and use the replay to
understand the actions behind those numbers.