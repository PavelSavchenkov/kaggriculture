# Running the pipeline

Run commands from the package directory. Use fresh output directories. This page covers direct package entry points; full historical training and reference panels use the restored checkpoint described in `DEVELOPMENT.md`.

## Fast prediction

`build/estimate_day problem.json [23|24] [menu.txt]` returns one JSON forecast. The contract and menu are specified in `API.md`. Use the header-only API for actual search iterations; the process-per-input CLI is for inspection.

`build/predict_planning manifest.txt predictions.csv repeats` evaluates a batch in one process, measuring prediction CPU with adaptive and forced-complete-curve modes. Each whitespace-separated manifest record is:

```text
id problem_path active_hours fixed_count optional_count fixed_hour fixed_slot ... optional_hour optional_slot ...
```

Counts determine how many pairs follow. Paths and IDs must contain no whitespace. Provide hiring options from the actual allowed market calendar, independently of source worker count. The repeats argument must be positive. JSON loading is outside the recorded inference timer; menu construction, features, bounds and inference are inside.

## Bounded reference and verification

```sh
conda run -n kaggriculture cmake --build build --target reference reference_terminal_deadlines predict_planning -j4
conda run -n kaggriculture ../day_solver/with_runtime.sh build/reference queries.txt evidence/new_reference 3
```

Each `queries.txt` line contains `query_id problem_path`. Every contract here has one exact worker count and its matching selected hire slots. To sweep worker counts, construct separate exact-count contracts using the existing context/calendar helpers; changing `worker_count` alone is insufficient. The last argument is the solver's requested per-call budget in seconds. Record actual CPU and wall separately.

The reference writes `results.jsonl` and a schedule for each verified success. It independently checks strict replay, required work and invariants before reporting FEASIBLE. UNKNOWN is a bounded failure to find a certificate. Use `reference_terminal_deadlines` for H23: it gives the solver real phase-22 deadlines before scheduling. Keep mathematical lower bounds separate from these upper certificates.

## Complete warm compiler comparison

```sh
conda run -n kaggriculture cmake -S . -B build -DFAST_ESTIMATOR_BUILD_WARM=ON
conda run -n kaggriculture cmake --build build --target warm_compile_early warm_compile_guided verify_warm_course -j4
conda run -n kaggriculture ../day_solver/with_runtime.sh build/warm_compile_guided evidence/new_guided 1470762556 examples/warm_specs/melon12_d12.txt 3
conda run -n kaggriculture ../day_solver/with_runtime.sh build/verify_warm_course evidence/new_guided 1470762556 evidence/new_guided_independent.json
```

The original control uses `warm_compile_early` with the same arguments and a different fresh output directory. A specification line is `cell first_day item`, with cell `10*y+x`, a southeast-quadrant tile, first day 8..22, and a crop or animal item enum from the shared engine. The seven original specifications are in `examples/warm_specs/`.

These executables are full-course benchmark fixtures for the published source and rival. They compile each proposed day's obligations, retain route reuse/repair/fallback, write day contracts/actions and compiler logs, and can fail to construct a complete course. A run is accepted only after the separate 719-transition verifier succeeds. Keep every failed run and its timing. The three-second argument is a component call budget, not a three-second whole-course limit.

For performance claims, use the checkpoint's paired drivers: they time complete child CPU, balance method order, preserve exit codes/artifacts, and compare matched verified production and hire bills. Direct commands above show invocation and are not by themselves a new performance experiment. Do not use a partial-course resume when measuring a fresh complete compiler pair. See the frozen gates in `DEVELOPMENT.md`.
