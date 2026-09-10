# Sales planner

Reusable C++ financial planning code and the September 9–10 session handoff. This is the **sales planner** discussed in the session. No tile planner or tested tile changes were produced.

The best retained search is [`delay_sales_within_day`](include/day_timing.hpp), with public [held-stock history](include/rival_stock.hpp) and an [earliest rival delivery bound](include/rival_delivery.hpp). It improves an existing order plan while preserving the supplied farm work. The live example is [`history_course_multiple`](agents/history_course_multiple/README.md); its default farm is the cow course. Keep the one-edit `history_course_delivery` control.

Measured results have different baselines:

| Comparison | Mean final cash-margin gain | Evidence |
| --- | ---: | --- |
| `room_keep` versus our original `early_structure_cow` agent, 6,144 pairs | +$24.22 | Direct agent improvement |
| Multiple-sale timing versus purchase repair on three supplied courses, 9,216 pairs | +$574.93 | Retained timing component |
| Multiple-sale timing versus one-edit timing on those same games | +$41.55 | Nine losses; worst −$44 |

Better composition selection and general purchase timing remain unproved. Read [RESULTS.md](RESULTS.md) for uncertainty, negative cases and promotion limits.

## Start here

- [DESIGN.md](DESIGN.md): problem, inputs, outputs and the narrower problem actually solved.
- [REUSE.md](REUSE.md): C++ interface, event order, period boundaries and caller duties.
- [PROTOCOL.md](PROTOCOL.md): objective, comparison rules and promotion criteria.
- [REPRODUCE.md](REPRODUCE.md): build, check, restore data and run comparisons.
- [RESUME.md](RESUME.md): next priorities and the status of every implementation.
- [LEARNINGS.md](LEARNINGS.md): successful and failed experiments, with measured outcomes.
- [FARM_RECOMMENDATIONS.md](FARM_RECOMMENDATIONS.md): concrete sales-friendly and sales-unfriendly plans, and untested schedule/tile ideas.
- [IDEAS.md](IDEAS.md) and [PROFILING.md](PROFILING.md): review decisions and speed measurements, including unfavorable results.

From the repository root:

```bash
conda run --no-capture-output -n kaggriculture python sales_planner/scripts/verify.py --live
```

This uses three bundled, exposed regression cases. It needs no downloads or experiment archive. Builds and test outputs are ignored by this folder's `.gitignore`.

## What is stored

`include/`, `source/`, `agents/` and `scripts/` preserve the session's small implementations, controls, paused prototypes and diagnostic tools. Runtime paths were relocated; the trading rules were preserved. `configs/league.json` references existing root agents. Existing engine, common API, opponent sources and farm courses are dependencies, not copied here. See [DEPENDENCIES.json](DEPENDENCIES.json) and [PROVENANCE.json](PROVENANCE.json).

The wrappers inherit the source lineage and reuse restrictions documented in those root agent packages. No Kaggle leaderboard rating was measured for these modified wrappers.

[data/MANIFEST.json](data/MANIFEST.json) identifies 297 exposed episodes, calendar versions, replay hashes and 25 still-reserved episodes. Three compressed regression calendars are included. Large replays, game-by-game results, binaries, notebook downloads and redundant source archives are excluded.

[evidence/](evidence/) contains compact protocols, measurements and diagnoses. In preserved research notes, `runs/...`, `research/...` and `build/...` identify artifacts in the original [`experiments/v6/sep09_sales_planner/`](../experiments/v6/sep09_sales_planner/) archive. Copied evidence lives under `evidence/runs/`; the [run index](evidence/RUN_INDEX.json) records what was kept. These historical paths are evidence references, not runtime dependencies. Run new experiments under `sales_planner/runs/` and preserve old reports.
