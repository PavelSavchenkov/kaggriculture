# Improvement opportunity: plan shed inventory across the night

Status (Sep 25): identified with evidence, not implemented. Two naive probes failed for a known
mechanical reason (below). Owner: day compiler first; a network target may follow.
Context: `REPORT.md` (weak points), `BOUNDARY.md` section 2.2 (why this is not a network
decision yet). Agent: `experiments/v10/sep24_BC_opus` (best candidate `models/cand_v12_vadim6`).

## The gap in one sentence

The day compiler plans each day as if the shed must be almost empty by morning: its sale DP ends at
the night, stock left overnight is worth a fixed 0.95 x tomorrow's price, and returns are sized so
tonight's deposit fits. Top teams instead run an inventory cycle across the night: a nearly full
shed at dawn, sold down in the morning, refilled by the day's harvests.

## Evidence (days 10-27 unless noted; top = 4,134 top-10 perspectives, Sep 18-23)

| Measure | Top teams | Ours (Vadim candidate, Local-LB) |
|---|---:|---:|
| Shed units at dawn, day 12 / 16 / 20 | 65 / 79 / 80 | 12 / 40 / 68 |
| Units carried into the night deposit, per night | 66 | 57 |
| Units sold at hours 0-5, per day | 27 | 11 |
| Units sold at hours 18-23, per game | 577 | 1,038 |
| Days selling all available strawberries / eggs / wool | 2% / 15% / 13% | 42% / 45% / 38% |
| Dawn stock strawberry / egg / fertilizer | 15 / 7 / 8.6 | 8 / 3.6 / 2.6 |
| Wages per game | $5.4k | $7.1k |

- Our same-day returns (deposit by hour 20 so goods sell tonight) add 1.4 hires per mid-game day
  (+$165/day, 5 compiler-logged games): the route for the requested work needs 10.25 hires, with
  returns 11.65.
- The sale DP's corner solutions come from the terminal rule: with a steep price curve and a fixed
  0.95 hold factor, "sell all" or "hold all" usually wins; top teams sell a steady fraction.

## What the probes showed

| Probe (Local-LB, paired) | Result | Why |
|---|---|---|
| X5: keep same-day returns only if their sale gain beats the extra wages | wages -$1.3k, own money +$2.0k, opponent money +$3.8k, margin -$1.8k [-6.6k, +1.2k] | our evening sales beat the Local-LB agents' morning sales; without them the opponent sells first |
| X8: cap daily sales at the top teams' sell fraction per product | -$23.6k, wins 14% | stock accumulates, shed full by day 18, 27-50 units destroyed per night |
| X11: hold top-team overnight stock (strawberry 15, egg 7, fertilizer 8) | -$13.2k, wins 47% | shed at 82-100 at dawn, 15-24 units destroyed; returns and room logic assume an empty shed |

Conclusions: (1) holding cannot be bolted onto the one-day compiler; (2) evening selling also wins
a race against opponents that sell in the morning, so any redesign must keep "who sells first"
in the objective; (3) the value of top-team-like holding is not measured yet.

## Proposed design (compiler)

1. Extend the capacity plan over the night boundary: tonight's deposit, tomorrow's dawn stock and
   tomorrow's first market hours form one plan. A dawn sell-down (hours 0-5, before workers deposit
   the day's harvests) becomes a way to make room, so the evening need not empty the shed.
2. Give held stock a cross-day value instead of 0.95 x tomorrow: tomorrow's own production (known
   biology), the opponent's expected supply (visible biology, see REPORT section 3) and demand
   ticks, or a learned terminal value per product (`VALUE_MODEL.md`).
3. Choose returns with their real cost: extra hires at the Fibonacci wage versus the value of
   selling tonight (including beating the opponent's morning sales), inside route construction
   rather than as hard deadlines added afterwards.
4. Only after 1-3: test a network overnight-stock target per product (exact replay labels:
   end-of-day shed stock) against the compiler's own choice.

## How to test

- Gate: paired Local-LB panel (`run_exp.sh`, `paired.py`) plus the C++ and zoo panels; report own
  money, opponent money and margin separately (X5 showed they can move in opposite directions).
- Hard checks: shed discards per game stay < 1 (baseline 0.4); unsold stock at the end $0.
- Diagnostics: dawn shed level, sales by hour, sell fraction distribution per product and wages,
  against the top-team table above (`tools/ledger` + `q25.py`, `q28.py`).

## Pitfalls already hit

- A sell fraction or a fixed stock target without a capacity plan fills the shed (X8, X11).
- Dropping evening sales to save wages loses the race (X5).
- A stock valued with the trailing opponent forecast misses production waves (melons, wool, milk).
