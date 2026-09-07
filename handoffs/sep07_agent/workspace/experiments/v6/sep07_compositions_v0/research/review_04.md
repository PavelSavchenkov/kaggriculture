# Review 4 — 2026-09-07 02:07 UTC

Original wording was reread during this interval; its full scope remains
active. This turn produced code and verified evidence. Next review 02:27 UTC.
Deadline September 8, 00:47 UTC is unchanged. No blocking condition or GPU need.

## Progress and limits

Optional C++ profiling now records individual lifetimes and daily service masks,
accepted/failed actions, actual product/seed purchases, net sales, hourly and
per-turn trades, hires/cost, land/cost, weed digs and shed pressure. Purchases
use exact portable-inventory mass balance, including consumption and discards.
Accepted unit effects use the engine sanitizer. A separate simulator copy
preserves the pre-refresh farm at day end. Profiling 32 games took 1.3 seconds;
action hashes and every previous result field match the ordinary run. Requested
minus successful unit totals match the independent fault diagnostic.

The first financial evaluator consumes warehouse arrivals, withdrawals and
fixed costs. It handles shop demand, marginal prices, input purchases, sales,
capacity loss and explicit funding/order-slot gaps. Four full 719-turn fixtures
match engine cash, market stocks, sales, residue and discards, including joint
sales and floor pricing. It remains conditional on supplied production and
schedule assumptions. Rival same-turn order interleaving is approximate;
negative cash is a funding witness, not feasible credit. Placement/labor
integration and estimated-versus-executed ranking remain unimplemented.

## Global invariant comparison

Local sample: public_router versus teammate, 16 common seeds, both seats.
Global reference: 72 recent normal-game top-12 games. Different opponents and
shops make these differences hypotheses, not causal findings.

- Crop occupancy: local 1,513.9 days, global 1,517.7; watering 72.7%/72.2%.
  Productive-day water+fertilizer 25.5%/34.9%. Value additional fertilizer by
  affected product and price rather than raising the rate indiscriminately.
- Local production: wheat 536.3, carrot 77.3, tomato 0, strawberry 262, melon 72,
  eggs 59.3, milk 240.8, wool 144.4, fertilizer 367.7. Global crop harvest:
  504.1, 99.3, 31.3, 237.2, 76.1. Keep tomato/carrot composition alternatives;
  do not impose the average mix on every shop scenario.
- Cow-days local/global: 217.5/186.1; sheep 134.9/159.8; goose 45/35.8.
  Local feed+care shares 83.5%, 87.9%, 83.3% versus global 76.8%, 79.5%, 80.3%.
  Fertilizer collection 95.0%, 91.5%, 84.7% versus 92.7%, 89.5%, 84.9%.
  Productive service is already strong; adding service everywhere is unlikely
  to explain the remaining strategic gaps.
- Local cow counts: 2 at day 0, 4 at day 5, 9 at day 8. Sheep: 2, 2, 4 at
  those dates, mean 5.75 by day 11. Goose mean 2.25 by day 11. Terminal cow
  and sheep means fall to 5.625/4.188. Preserve these milestones and inspect
  late service economics before labeling animal exits as execution faults.
- Gross buys local/global: wheat 141.8/326.3, fertilizer 68.9/57.4. Net wheat
  sales 191.8/184.3: smaller turnover is not weaker wheat production. Local
  net fertilizer 270.1 versus 239.4; milk 240.8 versus 198.4; wool 144.4 versus
  158.1. Inputs, use and residue separate production from net sales.
- Hires local/global: 282.8/286.3; hire cost $5,595.7/$6,122.8. Extra land
  2.0/2.014 quadrants. These are useful workforce and layout calibration targets.
- Discards 2.0/7.81. Local successful weed digs 24.22 and failed non-PASS unit
  commands 56.25/game. Global weed/fault counterparts remain incomplete.
  Mean purchase hour: wheat 8.12, fertilizer 1.49. Full buy/sell timing vectors
  are saved with actual quantities, not requested order volume.

## Next decisions

Finish recorded warehouse-flow and fixed-cost export, then calibrate financial
forecasts separately from biological/scheduling assumptions. Combine dated
biology, greedy placement, service travel and workforce into the first complete
composition estimator. Begin cold compilation promptly for realization feedback.
Preserve intraday slot reuse and early cash as unresolved details. Strong
references remain unchanged; no new agent was promoted in this interval.
