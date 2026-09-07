# Kaggriculture day solver

Reusable C++ scheduler for one fixed 24-hour day. Give it the required farm work,
purchases, workforce, inventory deadlines and end state; it returns a strictly
replayed worker schedule or `UNKNOWN` when its search budget runs out.

This package contains **V30**, the fastest version with completed broad
development validation, and **V20** as the retained held-out reference. It is
self-contained: copy this directory to use it. The experiment folder and the
sibling `fast_game_engine/` folder are not runtime or build dependencies.

## Try it without building

From this directory, on a compatible Linux x86-64 machine:

```bash
./verify.sh
./solve.sh examples/tomato/input.json work/tomato
cat work/tomato/report.json
```

The output directory must be new or empty. On success, `schedule.json` contains
24 hours of worker actions and fixed purchase/hire orders. The third argument
optionally sets the soft budget in seconds; the fourth sets fallback threads:

```bash
./solve.sh examples/tomato/input.json work/tomato-5s 5 1
```

Defaults are 900 seconds and eight fallback threads. Most tested days finish
much earlier. Smaller budgets can reduce coverage. Exit codes: **0** schedule,
**1** unknown, **2** invalid input/options or adapter error. No original schedule
or route is accepted as input. `solve_reference.sh` runs V20 with the same interface.

## Use it inside C++

```cmake
add_subdirectory(path/to/day_solver day_solver_build)
target_link_libraries(my_pipeline PRIVATE DaySolver::scheduler)
```

```cpp
#include <day_solver/scheduler.hpp>
#include <day_solver/io.hpp>

auto problem = day_solver::load_problem_json("day.json");
auto result = day_scheduler::solve(problem);  // native calls; no subprocess
if (result.schedule) {
    const std::array<kag::Action, 24>& actions = *result.schedule;
    // Consume the complete schedule in your pipeline.
}
```

For direct typed construction, fill `day_solver::DayProblem`, call
`day_scheduler::prepare_problem(problem)`, then solve. Call `prepare_problem`
again after changing purchases, end inventories or end tiles. The complete
[C++ example](examples/cpp/main.cpp) constructs a tomato day without JSON.

Build and run the example and tests from this directory:

```bash
conda run -n kaggriculture cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DDAY_SOLVER_BUILD_EXAMPLES=ON -DDAY_SOLVER_BUILD_TESTS=ON
conda run -n kaggriculture cmake --build build -j2
conda run -n kaggriculture ctest --test-dir build --output-on-failure
./with_runtime.sh ./build/day_solver_example
```

`with_runtime.sh` pins the bundled shared libraries for a C++ executable. The CLI
wrappers do this automatically. Outside this repository, ordinary `cmake` and
`python3` commands also work; conda is the repository's command convention.

## What is validated

| Solver and cohort | Strict schedules | Median / mean solve time |
| --- | ---: | ---: |
| Python reference, unseen Gate9 | 1,323 / 1,323 | 2.462 / 4.938 s |
| V20, unseen Gate13 | 1,399 / 1,399 | 330 / 545 ms |
| V25, unseen Gate14 | 1,338 / 1,340 | 339 ms / 2.580 s, including timeouts |
| **V30**, 323 real development days | **323 / 323** | **243 / 423 ms** |
| **V30**, synthetic controls | **9 / 9** | Not a representative speed sample |
| **V30**, 242 previously slow days | **242 / 242** | **1.135 / 2.340 s** |

These are different cohorts and versions, not paired speedups. V30's inputs
were already exposed during development; its fresh unseen coverage is not
established by V20's Gate13 result. Two known feasible days remain unresolved:
`106126272_p0_d16` and `106139776_p1_d16`. Both timed out under V25, V20 and the
frozen Python reference. We do not claim every Crop Dusta day is solved.

The solver is **not an ML model**. It has no training weights or inference-time
replay database. We developed heuristics and native kernels, tuned search
budgets on development inputs, and froze versions before evaluating unused
Crop Dusta episodes. Every claimed benchmark schedule passed strict replay and
an independent hourly inventory ledger.

## Documentation

- [Input and output specification](docs/input_output.md), with a
  [JSON Schema](schemas/day_problem_v3.schema.json) and complete example.
- [Integration guide](docs/integration.md): C++ fields, budgets, runtime libraries,
  source rebuild, execution in the full game and troubleshooting.
- [How it works](docs/algorithm.md): routes, resource repair, CP-SAT, strict completion.
- [Development and tuning](docs/development.md): what was learned from replays,
  held-out protocol, accepted optimizations and rejected ideas.
- [Benchmarks](docs/benchmarks.md): evidence, hardware, coverage and commands to rerun.
- [Exact contract](objective.md), [release manifest](manifest.json), and
  [third-party dependencies](licenses/README.md).

The implementation is limited to the fixed 10×10 board and **1–40 workers**.
It ignores shed capacity and excludes random nighttime weeds from required end
tiles. Sales, prices, cash and opponents belong to the caller. These differences
matter when combining a schedule with the complete game engine.
