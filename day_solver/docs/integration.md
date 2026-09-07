# Integration guide

## Choose an interface

Use `solve.sh INPUT OUTPUT [SECONDS] [THREADS]` for an offline process pipeline.
It launches the unchanged V30 executable with all validated search settings.
Use `DaySolver::scheduler` for repeated in-process calls from C++20. It compiles
a small public wrapper and links the exact V30 archives. C++ solving performs
no JSON serialization, file access, Python execution or subprocess calls.

The package works without the original experiment or repository. Binaries were
built with GCC 13.3 for Linux x86-64 on Ubuntu 24.04 and checked on an Intel
Core i7-14700K. The bundled libraries include OR-Tools 9.15.6755, PyVRP 0.14.0,
its required numeric/search code, and the runtime dependency closure. The
system loader and glibc are not bundled. This is not a Windows, macOS or ARM
binary distribution; a generic cross-platform rebuild has not been validated.

## Build and link

```cmake
add_subdirectory(/absolute/path/day_solver day_solver_build)
target_link_libraries(my_pipeline PRIVATE DaySolver::scheduler)
```

Include `<day_solver/scheduler.hpp>` for the solver, and optionally
`<day_solver/io.hpp>` for JSON parsing, validation and replay utilities.
The target supplies matching `kag` engine types and the public schema headers.
Do not mix this API with older copies of `day_solver_api.hpp`, or compile an
incompatible `kag::Action` definition in another translation unit.

The example is also an independent consumer CMake project:

```bash
conda run -n kaggriculture cmake -S examples/cpp -B build-consumer
conda run -n kaggriculture cmake --build build-consumer -j2
./with_runtime.sh ./build-consumer/example
```

Run an integrating executable through `with_runtime.sh`, or set
`LD_LIBRARY_PATH` to this package's absolute `runtime/lib` path. The wrapper
sets it exactly to pin the validated dependencies. If your application needs
other private shared libraries, include those explicitly in your own launch
environment and check that the solver still resolves to these copies.
Do not combine incompatible OR-Tools/protobuf/Abseil versions in one process.
The solver uses exceptions; do not compile its source with `-fno-exceptions`.
Catch exceptions before returning through an agent API that forbids throws.

## Construct and edit problems through C++

`day_solver::DayProblem` predates the final public schema. Use these fields:

| JSON | C++ field |
| --- | --- |
| `format_version` | `format_version`, default 3 |
| `worker_count` | `worker_count` |
| `start` | `start.managed_tiles`, `start.shed`, `start.seeds` |
| `end_tiles` | `required_end_tiles`; set `tile` and `exact_state` |
| `tile_work` | `tile_work` |
| `buy_schedule` | `market_plan`; `market_op` uses `kag::M_*` |
| `shed_availability` | `shed_availability` |
| `end_shed`, `end_seeds` | Same names |

Build from a default `DayProblem`. Use `kag::OP_*` operation constants and item
constants such as `kag::TOMATO`, `kag::WHEAT`, `kag::FERTILIZER`.
Then call `day_scheduler::prepare_problem(problem)`. This validates the problem,
derives end-tile kind/item, and rebuilds the legacy exact-inventory and purchase
views from the public inputs. It performs no solve and uses no hidden state.
Call it again after changing those requirements in an outer planning loop.

The JSON parser already prepares its result. Do not supply your own
`required_outcomes`, `allowed_acquisitions`, `sale_targets` or `limits` to extend
the contract. `prepare_problem` replaces the first two; `sale_targets` must be
empty. Leave `start.cash`, `start.shed_capacity`, `limits` and event `cash_delta`
at their defaults; they are not extra v3 scheduling controls. In particular,
budgets come from `day_scheduler::Options`, not `DayProblem::limits`.

```cpp
day_scheduler::Options options;
options.seconds = 5.0;
options.fallback_workers = 1;
day_scheduler::prepare_problem(candidate);
auto result = day_scheduler::solve(candidate, options);
if (!result.schedule) {
    // This candidate was not scheduled within this budget.
    // Keep another already valid plan, or change the candidate/budget.
}
```

`solve` leaves its input unchanged. The schedule owns its actions after the call
returns. Elapsed time is reported separately. An empty optional is UNKNOWN;
malformed input or invalid options throw `std::runtime_error`. Allocation
failures may also propagate. A zero budget returns UNKNOWN on valid input.

## Budgeting and concurrency

Defaults reproduce the validated portfolio: 900 s soft total budget, eight
fallback solver threads, one CP-SAT thread per quick trial. Constructors, model
building and replay consume wall time. The budget is soft and is not a hard
real-time cancellation guarantee. Hundreds of milliseconds is typical for
development cases, not a promised worst case.

The historical evaluation ran two day processes concurrently, with up to eight
solver threads in each fallback. This avoids paying for a fresh interpreter
inside the native core while using spare CPU capacity. Start with that setting
for comparable offline evaluation. For many simultaneous callers, reduce
fallback threads to avoid oversubscription. No GPU is required at inference.

Each solve creates local search/model state; exact-model caching is within that
call. Cross-call memoization is not implemented. If your planner memoizes, key
the complete v3 problem, budget/options and solver version. Never reuse UNKNOWN
as an infeasibility proof. Time-limited search and parallel CP-SAT need not return
the same route or timing on repeated calls. The routine is exercised serially
and through multiple processes; heavily concurrent in-process use is not a
separate coverage claim of this release.

## Combine with the full game engine

The sibling `fast_game_engine/` provides the complete transition engine. Its
`sim.hpp` and `pyrandom.hpp` match this package's copied engine headers at release.
The day scheduler uses its own strict day replay with the explicit v3 contract:

- Shed capacity and overflow are ignored.
- Availability withdrawals represent the outer strategy's reserved goods.
  Output orders contain purchases and hires, not sales.
- Prices, cash, opponent orders and economic feasibility are external.
- Exact end tiles exclude only random nighttime weeds.

Therefore a strictly valid day schedule is not by itself a ready-to-submit full
game policy. The caller must execute its intended sales in the remaining market
slots, preserve purchase ordering, ensure affordability and actual shed capacity,
and validate the combined plan against the full game when those constraints
matter. Do not subtract availability again after executing the matching sales.

## Rebuild and inspect

Default CMake builds only the wrapper. The exact portfolio/model archives remain
unchanged. To rebuild their project sources:

```bash
conda run -n kaggriculture cmake -S . -B build-source -DCMAKE_BUILD_TYPE=Release \
    -DDAY_SOLVER_REBUILD_CORE=ON -DDAY_SOLVER_BUILD_TESTS=ON
conda run -n kaggriculture cmake --build build-source -j2
conda run -n kaggriculture ctest --test-dir build-source --output-on-failure
```

This compiles the portfolio, models, replay and ranked-neighborhood source list
in `cmake/core_sources.cmake`. It still links the shipped PyVRP archive, numeric
helper shared libraries, and OR-Tools dependency closure. It is not a source
build of every third-party dependency or a portable binary release. A rebuilt
core can choose different branches under time limits; validate changes on the
exposed corpus and a new held-out cohort before replacing the release binary.
`solve.sh` always invokes the original packaged executable, even after a source
build. To exercise a rebuilt core, run its C++ consumers/tests.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| Missing shared library or protobuf symbol | Use `with_runtime.sh`; inspect `ldd`; avoid mixing SDK versions |
| CLI rejects an output directory | Choose a new or empty directory |
| Input rejected | All required JSON keys, tile indices, exact end tiles, hire count, item IDs and order slots |
| Stale terminal constraints after a planner edit | Call `prepare_problem` after the edit |
| UNKNOWN on a known feasible day | Inspect attempts; try the preserved default budget; do not label infeasible |
| Slower than historical figures | Check CPU contention, fallback thread count, budget, and whether the input belongs to the slow set |
| Full-game replay differs | Check capacity, actual sales/cash, market ordering and excluded random weed semantics |

Generated outputs and builds belong under `work/` or `build*`. Preserve the
original binaries, checksums and first-attempt evidence when making a candidate.
