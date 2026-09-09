# Fast day-solver estimator

Predict daily labor cost and promising worker counts before calling the day solver. The hot path is framework-free C++ and performs no scheduling. The original solver still constructs and verifies schedules.

Two components have passed scoped tests: ordinary cold proposal ordering, and a warm-compiler deferral policy on new seeds with the same source/rival/shop fixture. General-calendar point estimates, large-addition accuracy, and broader warm transfer remain experimental. This package does not claim minimum-workforce optimality or improved game scores.

## Build and use

Linux x86-64, a C++20 compiler, CMake 3.19+, and standard Linux build tools are required. This folder is a component of the repository: it uses the existing `../day_solver/`, `../fast_game_engine/`, and `../agents/` directories. Estimator weights and code are kept here. Python is used only for offline development and validation. Run commands from this directory:

```sh
conda run -n kaggriculture cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
conda run -n kaggriculture cmake --build build --target estimate_day estimator_api_smoke -j4
conda run -n kaggriculture ctest --test-dir build --output-on-failure
conda run -n kaggriculture ../day_solver/with_runtime.sh build/estimate_day examples/ordinary.json
```

The CLI accepts the public day-solver JSON contract, followed optionally by active hours (`23` or `24`) and an explicit hiring-menu file. Its JSON output distinguishes a necessary lower bound, a cost-equivalent worker estimate, a hire-cost estimate, difficulty flags, and analytical rejection. `certificate` is always false. Invalid input exits nonzero; rejected physical obligations return null point estimates.

For repeated search calls, use the C++ API rather than launching one CLI process per proposal:

```cpp
#include "fast_day_solver_estimator/estimator.hpp"

auto menu = fast_day_solver_estimator::earliest_hiring_menu(problem);
auto estimate = fast_day_solver_estimator::estimate_day(problem, menu);
if (!estimate.analytically_rejected) {
    // Rank proposals using estimate.cost and retain its difficulty flags.
    // The downstream solver must verify any schedule you accept.
}
```

Link the header-only CMake target `FastDayEstimator::estimator`. Pure inference does not link the scheduling backend. JSON loading and offline reference tools link the shared `DaySolver::scheduler` separately. See [API and contract](docs/API.md) for fixed hires, marginal estimates, query ordering, and failure handling.

For full-course validation and the optional warm compiler, build the fixture targets:

```sh
conda run -n kaggriculture cmake -S . -B build -DFAST_ESTIMATOR_BUILD_WARM=ON
conda run -n kaggriculture cmake --build build --target warm_season warm_compile_guided verify_warm_course fixture_generic fixture_pair -j4
conda run -n kaggriculture python tools/check_package.py --output evidence/my_package_check
```

The output directory must be new. The two fixture-check executables validate full self-play, PASS matches, action metadata, instance reset/concurrency, and generic-versus-typed action/reward parity; compare their stdout files exactly.

The shared backend libraries target Linux x86-64. Other platforms need their own compatible backend build; pure C++ inference has no solver runtime dependency.

## What to commit

Keep this folder's source, current model headers, documentation, reports, examples, and manifests. `build*/`, dependency copies under `vendor/`, and the optional research archive are ignored. Builds are reproducible and should remain local.

The shared dependencies must also be present in the repository checkout. Optional warm benchmarks use `agents/external/early_structure_cow/` and `agents/external/nanare_four_quadrant_course/`; include those agent additions if they have not already been committed. The research-restoration manifest also includes the existing parent fixture. No Git tracking status was inspected.

`REPOSITORY_DEPENDENCIES.json` records the exact shared files used by this delivery. Run `conda run -n kaggriculture python tools/repository_dependencies.py` to check them. Every removed vendor file was byte-identical to its repository counterpart. The folder needs no files from `experiments/` for builds, inference, or its included validation.

## Measured results

| Completed comparison | Original | Improved |
| --- | ---: | ---: |
| Complete compiler CPU, 56 paired new-seed cases | 163.44 s | 125.85 s; 23.00% less |
| Cold layout time to best reference bill, 46 novel pools | 13.21 s | 1.62 s; 87.76% less |
| Best reference bill within 30 CPU seconds, same pools | 43/46 | 46/46 |
| Worker-equivalent MAE, 558 novel source days | 3.624 | 0.497 |
| Marginal cash MAE, 166 novel removal pairs | 214.80 | 26.49 |

Adaptive C++ prediction averages 69.69 microseconds for ordinary inputs and 86.56 for other tested calendars, including features and bounds. Timers exclude JSON parsing and process startup. Cold time-to-best is retrospective; complete compiler CPU is a separate measurement. Full details, signed errors, confidence intervals, failed gates, and hard cases are in [results](docs/RESULTS.md).

## Contents and resumption

- `include/`, `models/`, `source/`: deployable C++ estimator, frozen weights, reference and integration code.
- `docs/API.md`, `docs/COMMANDS.md`, `docs/DEVELOPMENT.md`, `docs/LEARNINGS.md`: formulation, runnable pipeline commands, promotion rules, positive/negative findings, and next priorities.
- `evidence/`: compact primary reports, final checkpoint state, dependency hashes, and package validation.
- `REPOSITORY_DEPENDENCIES.json`: exact hashes for shared solver, engine and agent dependencies, without duplicate copies.
- `research/ARCHIVE.json`: checksum and inventory of the optional complete research checkpoint, distributed separately from source code.
- `tools/restore_checkpoint.py`: restore a supplied checkpoint plus verified shared repository dependencies into a fresh workspace. See [development](docs/DEVELOPMENT.md).

The original experiment is retained at `experiments/v6/fast_day_solver_estimator/`. Its `distribution_archive/checkpoint.tar.zst` preserves all training data, raw replays, schedules, frozen predictions, reference attempts and partial runs. This large archive is optional for use of the estimator, but required to reproduce the entire research history. No download location has been published; anyone needing that full checkpoint must receive it separately. No research jobs remain running after finalization. Interrupted tests are not accepted results; see `evidence/FINAL_CHECKPOINT.json`.

See [delivery validation](docs/VALIDATION.md) for the clean-copy build, replay, agent-interface and archive checks. `MANIFEST.json` records delivery hashes; verify them with `conda run -n kaggriculture python tools/check_integrity.py`.
