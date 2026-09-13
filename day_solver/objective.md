# Day solver v3

## Objective

Build a worker scheduling pipeline that matches the original Crop Dusta agent
on a large held-out set of previously unseen public replay days. Given a fixed
24-hour day problem, return one strictly valid schedule satisfying its inputs.
Infeasibility proofs and general completeness proofs are not required.

This is a feasibility problem. Worker actions and movement have no separate cost,
but consume worker hours. Stop after finding one valid schedule; minimizing
actions, movement, or another schedule-quality score is not required.

## Input

- `worker_count`: exact total workers, including the farmer.
- `start`:
  - shed and seed counts;
  - only tiles that may receive farm actions, with coordinates and exact crop,
    animal, structure, weed, locked-land, age, stored yield/product, dry/water state,
    feed/care state, and fertilizer age/state. Omitted tiles may be crossed but
    not acted on.
- `end_tiles`: exact required state of every supplied tile after deterministic
  daily updates, before random nighttime weed spawning. Existing weeds and
  weeds caused by crop decay or missed watering remain part of the state.
  Randomly spawned weeds are excluded; the scheduler does not receive RNG state.
- `tile_work`: exact successful farm-action sequence required at each tile,
  including action arguments, quantities, and exact produced item quantities.
  This fixes harvesting, planting, watering, fertilizing, digging, building,
  placing animals, feeding, care,
  and fertilizer collection, but not the worker, hour, route, pickups, or
  deposits.
- `buy_schedule`: fixed successful hires and purchases, each with hour,
  within-hour order, item, and quantity. These execute after that hour's worker
  actions. It contains exactly `worker_count - 1` hires. Economic feasibility
  and prices are handled by the caller.
  A land purchase specifies its fixed quadrant effect: 1 northeast,
  2 southwest, or 3 southeast, acquired in that order. It unlocks the supplied
  tiles in that quadrant after worker actions; farm work there can start the
  following hour. Its output order is the game's argument-free `BUY_LAND`.
- `shed_availability[24][item]`: minimum cumulative quantity of each item that
  must have been made available by the end of that hour's worker actions.
- `end_shed` and `end_seeds`: exact required persistent state after the last
  scheduled purchase and the automatic end-of-day cargo transfer.

Rules are global, not input. Day index, blocked tiles, prices, sales, opponents,
jobs, routes, worker assignments, and an original schedule are not inputs.

## Shed semantics

- Initial shed contents count toward cumulative availability.
- Each increase in `shed_availability` reserves that many additional units for
  the external strategy layer. Reserved units are immediately removed and
  cannot satisfy a later increase.
- Optional `start.shed_capacity` enforces shed capacity, overflow and the exact
  retained day-end inventory. Omission retains the original unlimited-storage
  v3 contract for historical inputs.
- Workers may make any legal pickups and deposits, including transfers through
  the shed between workers, provided all fixed requirements remain satisfied.
- After hour 23, all remaining worker cargo is transferred to the shed
  automatically before `end_shed` is checked.

## Output

Reject malformed input before solving. Such a rejection is a
failed extraction or solve when evaluating a known-feasible replay day.

- If feasible: exactly 24 hours of actions for every active worker, including
  movement, farm actions, pickups, and deposits. The fixed
  `buy_schedule` is copied into its supplied order slots.
- If no schedule is found within the declared search budget: `unknown`.
  This is a failure in replay evaluation. Any other result without a strictly
  valid schedule also counts as a failure on a known-feasible replay day.

Every returned schedule must pass exact replay with no failed requested action
and must satisfy every availability checkpoint and exact end state.

## Validation

Freeze the solver, extractor, checker, budgets, and selection rules before
opening unused Crop Dusta episodes. Evaluate every complete eligible day,
including hard and easy days and losses. Preserve every first attempt and
record all extraction failures, exclusions, duplicates, timeouts, and invalid
schedules. Original schedules may establish input and validation witnesses,
but never enter a solver. Diagnose failures only after closing that cohort;
test fixes on another unused cohort.

Match the original day by satisfying its fixed worker count, tile work,
purchases, availability deadlines, and exact deterministic endpoints within
24 hours. No action or movement minimization is required. Judge coverage across
whole unused episodes as well as individual days. Develop C++ speed only after
the Python pipeline has sufficiently broad held-out success.
