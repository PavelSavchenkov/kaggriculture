# Exact day reconstruction

The C++ consumer links persistent day_solver V30 directly. Root and bundled
engine headers were verified byte-identical. Exceptions stay inside this
offline integration; no local-agent API throws or invokes a subprocess.

Build through conda:

```
conda run -n kaggriculture cmake -S experiments/v6/sep07_compositions_v0/scheduler -B experiments/v6/sep07_compositions_v0/build/day_scheduler -DCMAKE_BUILD_TYPE=Release
conda run -n kaggriculture cmake --build experiments/v6/sep07_compositions_v0/build/day_scheduler -j2
conda run -n kaggriculture day_solver/with_runtime.sh experiments/v6/sep07_compositions_v0/build/day_scheduler/rebuild_days 55 experiments/v6/sep07_compositions_v0/results/day_rebuild_55_v2 2
```

Use a new result directory for a new run. Arguments are replay-library program,
output directory and soft seconds per day. The current driver uses discovery
seed 1000, seat 0 and live public_router to obtain a source course. It extracts
successful ordered tile work, exact input orders, fixed hires/land, cumulative
sales deadlines, and exact next-morning physical states. All 29 full days are
considered; final day 29 has only 23 game actions and is outside this adapter.

The original worker schedule must satisfy the constructed day contract before
the solver runs. The solver receives no original routes. Returned routes are
checked again in the full game with the original accepted market quantities
and recorded rival course. This is a conditional reconstruction check, not a
new adaptive full-game agent or a promotion result. Original actions, problem
JSON, returned numeric schedules, errors and a CSV remain in the run directory.

Within one hour, workers act before every market order. The physical contract
therefore cancels same-product buys and sales against each other and schedules
only net warehouse changes. The complete accepted market sequence remains
separate and is restored for exact price/cash evaluation. This permits opening
wheat round trips without inventing worker-phase stock. Using requested rather
than accepted quantities changed three rebuilt days; the exact quantities fix
all three. Source-overflow days are explicitly skipped because V30 ignores
capacity, not declared infeasible. A solver timeout remains UNKNOWN.

First complete corrected run: 26 of 29 source contracts valid, three skipped
for source overflow; 21 of 26 solved within the two-second soft budget, all 21
match full-game cash, land, inventories, production and next-day tiles. Median
solved time 0.256 seconds. Five cases are UNKNOWN; one soft-budget run lasted
3.37 seconds. These are exposed development days, not held-out coverage.

Next: allow dated composition/service and sale-deadline edits to build these
contracts, retain multiple worker plans, and check capital/capacity and live
opponent responses before selecting a complete policy.
