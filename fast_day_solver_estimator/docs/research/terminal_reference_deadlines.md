# Explicit terminal reference deadlines

The first terminal reference calls the unchanged 24-phase V30 solver, clears all actions/orders in phase 23, and independently replays the result. A certificate is valid for the actual 23-phase day, but the solver can return a schedule using required field work in phase 23. Removing that work then produces UNKNOWN. That is a limitation of the budgeted wrapper, not an impossibility proof.

`terminal_deadlines.hpp` adds in-memory cumulative work requirements through phase 22 before the solver call. It uses the same operation/metric mapping as the copied original warm compiler: plant, water, harvest output quantity, fertilize, dig, pasture/coop construction, animal placement, feed, care, and fertilizer collection. All necessary field work must occur in the real phases. The virtual phase remains only to represent public day-end totals; the returned schedule is still cleared there and independently replayed against the original public input.

These extra outcome bounds are a derived in-memory view, not part of v3 JSON. The wrapper rebuilds them after loading each problem. The original `reference` and `reference_terminal` sources/binaries remain unchanged. `reference_terminal_deadlines` is separately named and its source/header/binary closure is saved in `snapshots/terminal_deadlines_v1` before the comparison calls.

All 45 existing strict terminal witnesses remain valid with the new bounds. Independent controls reject a required water action at phase 23, accept it at phase 22, and obtain a valid simple constrained solver result.

The completed first calendar sweep has ten verified upper bounds among 40 contracts from eight parents. The necessary-bound and route-capacity controls underestimate those loose upper labels by 7.8 and 6.37 workers on average, respectively. These controls were saved before labeling; neither is a trained terminal predictor. Treat that result as sensitive to wrapper quality until the new same-query/same-budget comparison completes.

Two separately recorded rounds use the new wrapper at three seconds: the original 31 terminal reference contracts and the 40 explicit-calendar contracts. Original initial outcomes stay unchanged. For physical potential, stronger new certificates may tighten the old upper-bound interval; for operational query modeling, keep the wrapper/version and actual outcomes distinct.
# Replay-counter audit

The unchanged native replay adapter increments the eleven selected work metrics for each successfully executed action. Harvest counts its required output quantity. In particular, repeated valid fertilization emits an event each time; the deadline does not incorrectly count unique tiles. The first completed 347-query comparison on the older 31 terminal contracts added no new or cheaper upper bounds. The calendar comparison is still running.
