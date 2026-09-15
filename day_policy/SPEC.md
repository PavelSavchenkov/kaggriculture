# Day policy API

The policy combines a day solver and tile-placement logic. Both are deterministic
heuristics and must be fast enough for repeated day-plan evaluation. Each call
solves one entire day, starting at hour 0.

By default, minimize hired workers while satisfying the plan. Strongly target 11
or fewer hires, with a hard cap of 13. These counts exclude the farmer. The
`day_policy_80p()` throughput profile instead fixes 11 hires and skips workforce
minimization.

## Input

- Exact dawn grid state, including weeds that appeared overnight.
- Shed contents and seeds.
- Seed and animal purchase quantities, bought at hour 0.
- Wheat purchase quantities and hours.
- Optional land purchase hour; at most one purchase, unlocking the next quadrant
  in order NE, SW, SE. NW is initially owned.
- Shed returns: `(product, CNT, T)` means at least CNT units in total returned by
  hour T, not counting start-of-day stock in the shed. Returns do not imply sales.
  Deposit only requested products, never exceeding the final requested quantity
  per product. No return requests means no explicit deposits.
- Jobs:
  - **Existing product:** `(tile, set of events)`.
  - **New product:** `(product type, set of events)`. Establishment is implicit;
    the policy chooses the tile and derives planting, housing and transport actions.

## Job events

| Job target | Allowed events |
|---|---|
| Existing crop | `water`, `fertilize`, `harvest`, `clear` |
| Existing animal | `feed`, `care`, `harvest`, `collect_fertilizer` |
| Existing weed or unoccupied housing | `clear` |
| New crop | `water`, `fertilize` |
| New animal | `feed`, `care` |

An empty event set requests no service. Harvest quantities are execution outputs.

## Inferring job ordering

Beyond game rules and return deadlines, use this ordering assumption:

- For one-shot crops, fertilize before watering when both events are requested
  and this order increases yield.

Other events have no fixed relative order. Choose their timing and worker
assignment to avoid crop decay and reduce travel and waiting.

## Execution

- Allow multiple workers/visits per tile, partial deposits, repeated returns and
  work after returning.
- Respect market-order slots for purchases and hires.
- A proof of minimum hires is not required.

## Limitations

- No fertilizer purchases or fertilizer pickups from the shed. Use declared
  animal collections.
- Hire only at hours 0 or 1.

The caller handles affordability, market decisions and shed capacity, including
automatic night deposits. Cash, market state and shed capacity are not inputs.

## Output

- **Success:** a complete schedule satisfying all requested events and returns,
  actual hires, production, timed receipts (delivered quantities and game hours),
  resulting state and solver computation time.
- **Failure:** no complete schedule found within the policy's limits.
