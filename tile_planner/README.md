# Tile planner

Assign legal cells to dated crop and animal lifetimes, compile their daily work with the existing day solver, and check the complete season and resource calendar. This is an offline C++ component for composition search, not a submitted agent.

Keep exact lifetime/day caches, packed strictly replayed schedules and complete-incumbent retention. General placement search remains experimental: six supported frozen final inputs tied equal-budget fixed-layout refinement, and two separate distinct-family cases lost. General demand/species rules and cash repair did not pass promotion. On eight supplied real incumbents, exact loading changes reduced mean compilation plus independent verification from 132 to 107 ms; all paired actions and outcomes matched across 120 runs. Source-certificate construction is excluded.

## Build and run

Run from the repository root. The package uses the existing `day_solver/`, `fast_day_solver_estimator/`, `fast_game_engine/` and `agents/` directories. It needs no experiment files for its build, example or package checks.

```bash
conda run -n kaggriculture cmake -S tile_planner -B tile_planner/build -DCMAKE_BUILD_TYPE=Release
conda run -n kaggriculture cmake --build tile_planner/build --parallel 2
tile_planner/build/compile_cold tile_planner/examples/rotation/INPUT.plan tile_planner/runs/rotation tile_planner/examples/rotation/assignment.txt 0 0 4001 0.005 --opponent public_router --shop_seed 5003 --bank tile_planner/examples/rotation/BANK.bin --warm_seconds 0
tile_planner/build/export_calendar tile_planner/runs/rotation 4001 0.005 0 tile_planner/runs/rotation/calendar
conda run -n kaggriculture python tile_planner/tools/optimize_placement.py tile_planner/runs/rotation tile_planner/runs/rotation_fixed --seconds 5 --mode fixed --objective cash
```

Output directories must be new. The example completes thirty days with hire bill 14, cash 14,246 and zero solver queries. The independent checker covers all 719 transitions. Use `--mode search` to enable experimental placement proposals; compare against `fixed` with the same total budget. Failed or unfinished candidates cannot replace a verified complete incumbent. A near-zero budget can return the supplied incumbent with `input_reverified:false`.

The default build creates seven tools. Other preserved research and check targets in `source/` are built explicitly. C++ callers can link `TilePlanner::planner`; see [API](docs/API.md). The whole-season CLI starts at day zero; the daily API accepts an actual dawn state. A general mid-game suffix CLI and within-day crop decay are not implemented.

## Documentation and research

- [Formulation](docs/DESIGN.md), [API](docs/API.md) and [handoff](docs/HANDOFF.md): inputs, outputs, objective, component boundaries and limits.
- [Results](docs/RESULTS.md), [promotion criteria](docs/PROTOCOL.md) and [gate decisions](docs/GATE_DECISION.md): what passed and what did not.
- [Search coverage](docs/SEARCH_COVERAGE.md), [positive/negative ledger](docs/LEARNINGS.md), [profiling](docs/PROFILING.md) and [ideas](docs/IDEAS.md): all method families explored and unresolved questions.
- [Resuming research](docs/RESUME.md): compact reports/scripts, input index and existing bulk evidence.
- [Package validation](evidence/VALIDATION.json), [provenance](PROVENANCE.json) and [shared dependency versions](REPOSITORY_DEPENDENCIES.json).

Keep source, tools, docs, example, compact reports and manifests when committing this folder. Generated builds/runs are ignored. The existing 7 GB experiment is referenced rather than copied; the full research history needs that existing directory. No Git tracking status was inspected or changed.
