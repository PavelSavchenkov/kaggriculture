# Release 1.1.0 — September 13, 2026

The default is the retained main portfolio. It includes explicit shed capacity,
timed stock completion, input-readiness repair, route caching and bounded repair
of pickup/deposit quantities on rejected proposals. Every returned schedule
passes strict day replay. `UNKNOWN` means the search did not return a schedule;
it does not prove infeasibility.

The sealed CLI is `runtime/day_solver_cli`; `solve.sh` pins its bundled runtime.
The source and binary hashes are in `manifest.json` and `SHA256SUMS`. Rebuilding
with CMake does not replace the sealed executable. Run a candidate explicitly
through `with_runtime.sh build/day_solver_cli`.

The previous V30 and V20 executables and original seal remain preserved. The
unfinished conditional-polish prototype is removed. Experimental main fallback
is kept outside this production package. Optional private research constructors
remain disabled in all public policies.

## Main coverage

These are complete frozen evaluations of the retained algorithm, before release
packaging. They are different cohorts and should not be pooled as one benchmark.
Inputs use canonical tile order and original replay worker counts, with exact
outputs, accepted trade slots and retained inventory checked in the full game.

| Cohort | Limit | Original | +1 | +2 | +3 |
|---|---:|---:|---:|---:|---:|
| 1,943 ordinary Crop Dusta archive days | 4s | 1,922 | — | — | — |
| 145 ordinary leader development days | 4s | 123 | 143 | 139 | 140 |
| Newly opened Unknown Mother-Goose game, 29 days | 4s | 29 | 29 | 29 | 29 |

Crop mean runtime is 307ms across all calls. Leader development means are
1.096/0.723/0.757/0.712s. New-game means are 390/392/393/386ms. The fixed-stock
repair adds one Crop success and two +1 leader successes with no paired losses;
one of the leader gains also succeeds in baseline on a selected repeat, so it
cannot be attributed solely to the repair. Twenty-one Crop archive calls and
22 original-worker leader calls remain unresolved at four seconds.

## Fast policies

All fast policies use one thread and preserve the same exact contract.

| CLI / C++ policy | Behavior |
|---|---|
| `regret` / `Search::Regret` | Original regret insertion with local improvement |
| `regret-deferred` / `Search::RegretDeferred` | Try promising raw routes first; 75% initial completion share |
| `regret-fast` / `Search::RegretFast` | Same algorithm with a 90% initial completion share |

At 500ms on the matched 145-day development family, 75% gives 89/109/102
successes at original/+1/+2, mean 265/250/262ms. The 90% variant gives
91/113/108, mean 264/247/259ms: twelve paired gains, no losses. On the newly
opened 29-day game at original through +3, these are 24/26/24/24 and
24/26/25/25: two gains, no losses.

The complete cross-budget check explains why both options remain. At 200ms,
75% gives 49/58/57 and 90% gives 53/58/57; six gains and two losses. At two
seconds they give 115/133/135 and 115/132/135; one loss because the larger first
attempt leaves insufficient time for a successful polished completion.
There is no claim that one heuristic dominates every day or time budget.

A separate matched 145-day 500ms workforce test gives the 90% policy 62/145
one-worker reductions and 92/145 original-worker successes. All 62 reductions
are shared; 30 more days match only, and 53 remain unresolved at both counts.
Different runs can vary near their cutoffs. Every worker-count query has its
own budget; these results do not describe one combined workforce search.

## Validation and limits

Final ordinary, full-core and relocated builds each pass all six regression
groups (33.6s per suite). All four public CLI policies and the overflow control
pass strict replay plus120independent hourly checks. The standalone copy also
builds and runs the C++ example. Release evidence records these checks and
runtime dependency resolution. The first full-core test run had one 12-second cold-control
timeout; selected matched runs of both builds solved it in about 10.4 seconds.
The first failure is preserved alongside final checks, not replaced by them.

The solver's budget is soft. Checks exclude successful overruns from
within-budget quality counts. Search remains incomplete and timing-sensitive;
successful tests cannot prove that software has no undiscovered bugs.
The v3 interface is for ordinary 24-hour days. Capacity must be supplied to
enforce it; omission keeps the historical unlimited contract. Sales and cash
remain caller responsibilities, represented through the documented inventory
withdrawals and purchase slots. Validate their integration in the full game.

Evidence and source hashes are self-contained in `evidence/release_1_1_0/`.
