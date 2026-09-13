# Release 1.2.0 — September 13, 2026

This release adds consumption-aware sale-producer selection and improves the
portfolio order for budgets from one through four seconds. It first constructs
routes with RegretFast, then may retry with one fewer worker and restore the last
hire as idle at its original slot. Every returned schedule passes strict replay
against the original contract, including outputs, inventory and hire orders.

Longer budgets retain the previous portfolio order. Applying the new reservation
to all budgets regressed a known 12-second cold control; that rejected candidate
and its result are preserved in the release evidence. Stock-repair expansion,
worker-hint changes and fixed-worker completion trials are not included.

## Complete paired four-second comparisons

Counts below use original replay workers as the baseline. Each workforce receives
its own time budget. Failed calls remain in the means, and successful overruns
do not count as within-budget successes. Returned schedules also passed the full
game engine with identical outputs, accepted trade slots and retained inventory.

| Cohort | Workers | Previous successes | Current successes | Previous mean | Current mean |
|---|---|---:|---:|---:|---:|
| 145 exposed leader days | Original | 124 | 129 | 1,086ms | 807ms |
| Same days | +1 | 143 | 142 | 699ms | 528ms |
| Same days | +2 | 140 | 140 | 754ms | 485ms |
| Same days | +3 | 140 | 142 | 720ms | 456ms |
| 1,943 historical Crop Dusta days | Original | 1,922 | 1,923 | 299ms | 259ms |
| 58 newly opened leader-seat days | Original | 58 | 58 | 531ms | 307ms |
| Same unseen days | +1 | 58 | 58 | 465ms | 292ms |
| Same unseen days | +2 | 58 | 58 | 464ms | 282ms |
| Same unseen days | +3 | 58 | 58 | 472ms | 312ms |

The leader comparison has twelve gains and six losses across580 contracts;
original-worker calls have five gains and no losses. Crop has eight gains and
seven losses. Its median falls from173 to131ms. This is an aggregate improvement,
not a claim of dominance on every day or workforce.

The unseen test froze candidates before opening replay108286683. It includes all
29 ordinary days of both THIRD FARM CLUB and ymg_aq. It is one new game with two
leader seats, not58 independent games. The short-budget release order is the
same as the candidate evaluated in these four-second comparisons.

## Fast policies

All four public policies remain available. `regret-fast` retains its90% initial
completion share, with the sale-producer correction applied before job grouping.
It avoids assigning a sale return to a harvest consumed later on the same tile
when a suitable independent producer exists. Actual delivery constraints remain
unchanged. The diagnosed five-worker M&M contract now solves in about45ms.

The complete paired145-day500ms test gives previous/current successes of
62/62 at−1 worker,92/92 at original,113/114 at+1,107/108 at+2 and109/110 at+3.
Current mean runtimes are271/265/246/261/265ms respectively. Four M&M gains and
one timing-sensitive ymg loss are retained in the evidence. Separate repeats do
not replace these counts. The fixed-worker trial lost unseen fast coverage and
was excluded; its results are not attributed to the released fast solver.

## Validation and remaining limits

All seven regression groups pass in the ordinary build, rebuilt core and
standalone copy. Four public CLI policies, six full-engine schedules (144 hours),
the overflow control, and zero/invalid options also pass. The checks cover
public CLI policies, zero/invalid options, shed capacity, within-day transfers,
the new cold search controls, and independent full-engine replay. Exact commands
and results are in `evidence/release_1_2_0/checks.json`.

Search remains incomplete and time limits are soft. `UNKNOWN` does not prove
infeasibility. Some known feasible replay days remain unresolved, and results
can vary near a cutoff. Passing these checks does not prove the absence of all
undiscovered bugs. Sales and cash must still be validated by the caller's full
game integration; the replay benchmarks ignore only extra-wage cash shortfalls.

`solve.sh` runs `runtime/day_solver_cli`. CMake builds matching source without
replacing that sealed executable. `manifest.json` and `SHA256SUMS` identify the
release. Benchmark rows, paired losses, rejected long-budget check, validation
logs and source hashes are included in `evidence/release_1_2_0/`.

Before publication, upstream added a joint-action helper to the bundled engine
header. A clean rebuild produced byte-identical solver and audit executables.
The first new suite passed5/7 groups while Git's automatic packing saturated
the CPU; both timed groups passed after that maintenance was paused. The first
results and the recheck are preserved in `evidence/release_1_2_0/upstream_header/`.
The source and engine-header hashes include the upstream addition.
