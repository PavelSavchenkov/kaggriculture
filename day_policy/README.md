# Day policy

Deterministic C++ worker scheduling and tile placement for a supplied whole-day
plan. The default is **Balanced-4 + Staged placement**, with hire minimization
and a hard cap of 13 hires plus the farmer. Use cap 11 when it is mandatory.
**Full-8** searches more broadly at higher cost; it is a separate call, not an
automatic fallback. Both support within-day shed returns with deadlines.

Everything needed to build and reproduce the tests is inside this folder.
The only prerequisites are a C++20 compiler, CMake, a Linux toolchain and Python
3 with its standard library. The small engine/API snapshots in `vendor/` avoid
dependencies on other repository or experiment files. No Git operation is needed.

## Contents

| Path | Purpose |
|---|---|
| `SPEC.md`, `source/policy.hpp` | Input/output contract and C++ types |
| `source/` | Worker search, placement, native verifier and executor |
| `docs/ALGORITHM.md` | Search profiles, placement rules and integration limits |
| `docs/TESTING.md` | Cohort construction, exclusions and failure interpretation |
| `docs/REPLAY_PATTERNS.md` | Placement evidence from top-player games |
| `measurements/REPORT.md`, `SUMMARY.json` | Total/per-player coverage, hires and timings |
| `measurements/CAPS.md`, `CAPS.csv`, `CAPS.json` | Caps 10/11/13 filtered to original hires <= the tested cap, by cohort and original agent |
| `measurements/rows.zip` | Compressed per-call measurement evidence |
| `data/` | Compressed replay traces, selected cohorts and format documentation |
| `tests/`, `tools/` | Regressions, complete-game smoke tests and replay benchmarks |
| `vendor/` | Pinned engine/API headers and provenance hashes |
| `MANIFEST.json`, `PACKAGE_CHECK.json` | File integrity/size inventory and delivery checks |

## Use

```cpp
#include "policy.hpp"
#include <memory>
using namespace kag::agents::day_policy_contract;

auto solver = std::make_unique<Solver>(); // One instance per thread.
DayInput input;                          // Fill exact dawn grid and declared jobs.
SolveOptions options;                   // Balanced-4, Staged, cap 13.
options.max_hires = 11;
auto result = solver->solve(input, options);
if (result.status == SolveStatus::Success) {
    // Execute result.schedule[0..23]; propagate result.state.
}
// Broader independent call: options.effort=SearchEffort::Full; options.variants=8;
```

Initialize all 100 cells, including locked land and overnight weeds. Timestamps
are relative to dawn, as documented in `policy.hpp`. Success means all requested
events and cumulative worker deposits passed native replay. `InvalidInput` and
`NoScheduleFound` expose no executable partial plan; the latter is not an
infeasibility proof. Minimum hires are not proved.

The caller handles purchase funding, sales, shed capacity and night settlement.
Adding sales must preserve scheduled purchases/hires and fit remaining market
slots; the policy reserves no sale slots or financing. Result state is after
hour 23 actions/decay, before automatic night deposits and random changes.
Propagate this actual state, including repaired crop sites. The shortened final
game day needs separate integration. `agent.json` wraps the executor and a
smoke-test planner; it is not a competitive purchase/sale decision engine.

## Build and reproduce

Commands below run from the repository root. Generated files go to
`work/day_policy`; nothing is generated inside the delivery folder.

```sh
conda run -n kaggriculture cmake -S day_policy -B work/day_policy/build -DCMAKE_BUILD_TYPE=Release
conda run -n kaggriculture cmake --build work/day_policy/build -j 4
conda run -n kaggriculture python -B day_policy/tools/check_package.py
conda run -n kaggriculture ctest --test-dir work/day_policy/build --output-on-failure
conda run -n kaggriculture python -B day_policy/tools/test.py prepare --work work/day_policy/check --build work/day_policy/build
conda run -n kaggriculture python -B day_policy/tools/test.py benchmark --work work/day_policy/check --build work/day_policy/build --labels 645 --suite quick --run quick --check-reference
conda run -n kaggriculture python -B day_policy/tools/test.py smoke --work work/day_policy/check --build work/day_policy/build --run smoke
```

`prepare` checks every replay state, regenerates original/Legacy/Staged day
cases, verifies original-grid progression and compares mapping/return bounds.
The quick suite runs five configurations per cohort, with two repeats by default.
Use `--suite final` without `--labels` for all 28 selected configurations, or
`--suite all` to add the 12 Legacy comparisons. `--check-reference` compares
every non-time CSV field, including schedule hash and hire count. Omit it when
evaluating an intentional behavior change. Give each run a new `--run` name.

For timings, choose an available core with `--cpu N` and run benchmarks
sequentially without concurrent builds. Times will vary by machine and load.
New `SUMMARY.json` files contain total/per-player coverage and timings, plus
original-hire-filtered totals at each cap. All successful schedules receive
a second native verification outside the reported solver time.

Regenerate the saved report from its archived CSV evidence:

```sh
conda run -n kaggriculture python -B day_policy/tools/test.py unpack-reference --work work/day_policy/check
conda run -n kaggriculture python -B day_policy/tools/report.py --results work/day_policy/check/reference --output work/day_policy/report
conda run -n kaggriculture python -B day_policy/tools/report_caps.py --output work/day_policy/caps_report
```

Use `--suite caps` for the complete cap-10/11/13 matrix: original dawns with
Balanced-4/Full-8, and our carried placements with Balanced-4 under strict and
hour-23 deadlines. `CAPS.md` applies `original_hires <= tested cap` before
aggregating coverage and timings. `CAPS.csv` includes every original agent in
each cohort and in the two comparison cohorts combined.

Release uses O3, native CPU tuning and LTO, without PGO or fast-math. Set
`-DDAY_POLICY_NATIVE=OFF` for a portable CPU build. For sanitizers, use a separate
build directory with `-DCMAKE_BUILD_TYPE=Debug -DDAY_POLICY_SANITIZE=ON`, then
run CTest, smoke and replay checks against it. To embed just the library, use
`-DBUILD_TESTING=OFF` and link the `day_policy::policy` CMake target.

## What the measurements support

The two comparison cohorts contain 1,284 compatible days from the recorded
top 30 players. With exact original dawns, Balanced-4 solves 1,223 at cap 11
and 1,277 at cap 13. With our placements carried from game start and the
original strict return deadlines, it solves 1,144 and 1,200 respectively.
See the report for Full-8, late days, per-player results and timing distributions.

Changed-layout tests preserve recorded prefix service outcomes, including
excluded/unsolved prefix days. They test geometry and execution capacity, not
a fully feasible economic rollout. Some inherited deadlines become physically
impossible after relocation; hour-23 diagnostics relax those deadlines and do
not establish early-return coverage. The comparison games were already exposed
during worker-policy development. These results do not establish full-agent
win rate or gold-medal strength.
