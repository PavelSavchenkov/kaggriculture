# Day compiler

## Responsibility

The day compiler turns the dawn state, observed history, and `DayIntent` into
every worker action and market order for the day. Every plan must follow the
game rules and satisfy all `DayIntent` requirements, except as
described in [When the DayIntent cannot be met](#when-the-dayintent-cannot-be-met).
The compiler must not stop at the first legal plan. Within its runtime budget,
it searches for the legal plan with the highest economic value.

**Most importantly, economic value means expected long-term margin against the
opponent. It does not mean cash at the end of the current day.** The compiler
estimates which within-day choices are most likely to produce the best margin
over the rest of the game.

At dawn, it creates an initial plan for the whole day. Every hour, it replans
from the observed state. Replanning uses actual action results, market fills,
prices, and observed opponent activity instead of assuming that the dawn
forecast happened exactly.

## What the compiler decides

The compiler chooses everything not fixed by `DayIntent` and optimizes these
choices together:

- which group members receive each requested count, and the tiles for new
  crops and animals;
- workers, hires, paths, and action times. On a one-shot crop, it fertilizes
  before watering and waters before harvesting, because `DayIntent` options do
  not record order;
- watering of ongoing crops: every retained or new ongoing crop survives
  tonight. On the day before a production, a crop with active fertilizer should
  almost always be watered, because watering doubles that production. Top
  players did this for 98% of such crops. Skip it only when the extra unit
  would be lost to the held-product cap or is worth less than the work;
- building coops and pastures, clearing weeds as needed, and how each requested
  clear is done;
- fertilizer collection from animals; and
- all purchases, sales, and returns to the shed, with their quantities and
  times.

## Market planning

Sales and purchases are a central part of the plan, not a final cleanup step.
The compiler must plan:

- wheat purchases and sales together with animal feeding and funding needs;
- fertilizer purchases and sales together with crop service and fertilizer
  collected from animals;
- sales of ready crop and animal products together with harvest, collection,
  shed returns, capacity, funding, opponent supply, and price recovery; and
- wheat and fertilizer trades made for profit or margin, beyond our own needs.
  An immediate buy-and-resell gains nothing, because there is no spread. Such a
  trade pays only if the price moves in between, through town and shop demand,
  opponent trades, or holding stock to a later day. Buying wheat also changes
  what the opponent pays for feed and earns from wheat sales. These trades must
  not take cash, shed space, or order slots that required work needs.

Sales may be early, late, partial, or staged. Partial and staged sales matter
most: every unit sold lowers the price, so selling a large stock at once pushes
the price down for the last units. Selling part now and the rest after town and
shop demand has pulled the price back up usually earns more. Selling early can
fund useful work or beat the opponent to a high price. Waiting can be better
when the price is expected to recover. The compiler must value these
possibilities and reconsider them every hour.

The expected opponent forecast ranks economic value. A separate conservative
stress forecast checks funding and execution safety. The stress forecast must
not be used as the expected economic future. Both forecasts are updated from
observations every hour.

## When the DayIntent cannot be met

- The compiler reports the reason: invalid `DayIntent`, proven infeasible,
  unaffordable under the stress forecast, or search budget exhausted. A failed
  search does not prove that the `DayIntent` is infeasible.
- Part of the search budget is reserved, so a failed attempt at the requested
  `DayIntent` still leaves budget for a fallback plan.
- The fallback plan, and any repair when actual fills or prices make a
  requirement impossible during the day, keeps requirements in this order:
  survival of retained crops and animals first, then harvest and collection of
  existing product, then new crops, animals, and land. Every dropped requirement
  is reported.
- Every executed action, including fallback and repair actions, comes from the
  compiler and is tracked as completed or remaining work. A failed plan never
  turns into an idle day.

## Things to watch

These rules caused repeated planning bugs:

- `HIRE` is a market order. Hires share each hour's 10 order slots with
  purchases and sales.
- Both players' orders run by list position. An opponent order earlier in the
  list changes the price of ours, so order position changes cash.
- Workers act before market orders. Overflow from a `DROP` is destroyed before
  a sale in the same turn can make room.
- Carried items enter the shed at night, and overflow is destroyed.
- Crops and animals hold only a capped amount of product. Production above the
  cap is lost, so water, fertilizer, or care that only adds units above the cap
  has no value.
- A crop past its first yield-loss age loses one stored unit after hours 0, 2,
  4, and so on. A later harvest collects less, and a crop that reaches zero
  becomes a weed.
- Items bought in one turn are usable only from the next turn. An hour-0 pickup
  needs stock that was already in the shed at dawn.
- A sale at the $1 floor does not change market inventory.
- Hire prices restart each day. A replan continues from the number already
  hired.
- Plan from actual fills, deposits, and completed work, not from requested
  quantities.
- Opponent trades are not observed directly. Estimate them from the change in
  market inventory minus our actual fills and known town and shop consumption.
  Subtracting our requested quantities instead turns our failed orders into
  fake opponent trades.
- Day 29 has only 23 turns and no night update. Stock not sold by the last
  market is worth nothing.

## Validating economic value

How the compiler values the end-of-day state and its effect on the opponent is
an implementation choice. Examples are sale versus hold, input stock kept for
the next dawn, and product held on crops and animals. The validation is fixed:

- On controlled alternatives that satisfy the same `DayIntent`, the plan ranked
  higher must realize higher longer-term margin. The compiler must not
  consistently choose a worse legal plan because its estimate misses downstream
  effects.
- Realized value is measured by continuing both alternatives with the same
  policy on later days, against opponents that react, over enough independent
  seeds and both seats to give a confidence interval. Continuations against a
  fixed opponent replay are diagnostics only.
- Predicted and realized value are compared for each part of the end-of-day
  state, so a systematic error can be located.
- Margin is the main metric. Own cash and opponent cash are also reported, to
  show whether a margin change comes from our income or from the opponent's.
  An estimate that treats opponent sales as fixed quantities makes lowering
  prices look free; its gains must hold in the realized results.

## Development gates

A compiler version is ready only when all of these areas pass:

- **Legality and coverage:** it completes a large and varied set of replay days,
  including hard days, without illegal actions or missed `DayIntent`
  requirements. Missed requirements are counted after the whole day runs
  against actual opponent behaviour, not at compile time. Compile failures,
  in-day repairs, and missed requirements are reported separately.
- **Market quality:** when tested separately, its wheat market, fertilizer
  market, and ready-product market behaviour is economically similar to or
  better than typical top-player behaviour. The more precise gate is that it
  has no conceptual misses compared with top-LB replays: every important,
  recurring market behaviour seen there must be represented and evaluated.
  Timing, quantities, funding, and retained stock all matter.
- **Joint quality:** all compiler choices work well together. Passing isolated
  component tests is not sufficient.
- **Biological and logistics quality:** for a fixed `DayIntent`, the compiler
  avoids losses caused by its own choices: shed discard, stranded cargo,
  economically late returns, and yield lost to decay before a required harvest.
  Required harvest and collection work must be followed through to valuable
  storage and market handling. Clipping, escape, and abandonment follow from
  `DayIntent` counts, so they are network decisions.
- **Economic value:** value estimates pass the checks in
  [Validating economic value](#validating-economic-value).
- **Closed-loop quality:** hourly replanning responds correctly when actions,
  fills, prices, or opponent behaviour differ from the dawn forecast.
- **Full-game quality:** full games with the current policy against current
  opponents are not worse. Replay-based gains are not enough, because compiler
  choices change the states the policy sees and the policy can react badly to
  small changes.
- **Latency:** initial full-day compilation averages at most 0.5 seconds,
  hourly replanning remains bounded and fast, and no day causes an explosive
  latency spike.

A new compiler must be stored as a separate version. Changes to the
`DayIntent` interface require a new compiler version that implements and tests
the new contract.
