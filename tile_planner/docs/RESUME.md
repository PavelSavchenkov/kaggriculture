# Resume tile-planner work

The reusable code now lives in `tile_planner/`. Its C++ files are unchanged from the checked final September 10 component. CMake paths and documentation were adapted for the root package; the historical experiment remains untouched. `PROVENANCE.json` maps every copied file to its original hash. This extraction does not change any quality or repair gate decision.

Start with `RESULTS.md`, `GATE_DECISION.md`, `DESIGN.md`, `API.md` and `LEARNINGS.md`. `SEARCH_COVERAGE.md` maps rule-based, analytical, local/evolutionary, donor, quadrant, conditional, repair and cache work. Earlier entries in the chronological ledger may describe checks that were corrected later; the final results and gate decision take precedence.

## Data and checkpoints

`evidence/RESULTS.json` is the compact paired result index. `research/session_reports.tar.gz` preserves small reports, protocols, all research Python scripts and the 20-minute reviews without repeating large generated courses or shared libraries. `research/REPORTS.json` records each archive member's original relative path, byte count and SHA-256. This archive also contains the initial September 7/PDF assessment and the complete source-review metadata. Original source URLs, team IDs, seeds and splits remain in those records.

`research/INPUTS.json` indexes the 75 oracle/cache-check inputs and existing assignments, with hashes. These contain 61 distinct plan contents; they stay in the original experiment instead of being duplicated. `examples/rotation/` is the small independent input/output demonstration. `generate_cases` can regenerate synthetic programs from the preserved C++ factories.

The full original checkpoint is `experiments/v6/sep10_tiles_placement/`. It contains raw replays, trace conversions, physical problems, executable courses, finance contexts, packed banks, frozen binaries and all failed attempts. It is required for exact historical replay and full research resumption; it is not required for the root component's build or example. `research/EXISTING_DATA.json.gz` indexes retained research artifacts without copying their payloads. Its build-directory and copied-runtime exclusions are explicit in `research/CHECKPOINT.json`.

Do not delete that experiment unless its data has been preserved elsewhere. Committing only `tile_planner/` preserves the component, compact findings and reports, but does not copy the bulk historical replay/course data. No remote backup location is claimed.

## Commands

Run from the repository root with new output paths:

```bash
conda run -n kaggriculture python tile_planner/tools/check_integrity.py
conda run -n kaggriculture python tile_planner/tools/research.py verify
conda run -n kaggriculture python tile_planner/tools/research.py reports --output tile_planner/research/restored_reports
conda run -n kaggriculture python tile_planner/tools/research.py inputs --output tile_planner/research/restored_inputs
conda run -n kaggriculture cmake --build tile_planner/build --target generate_cases --parallel 2
tile_planner/build/generate_cases tile_planner/runs/new_synthetic_inputs
```

The research helper verifies hashes before copying inputs or extracting reports. `verify` checks indexed original evidence; use `--repo` for a different checkout. The restored reports retain historical paths in their contents. Old Python research scripts assume an experiment root and are archival recipes, not the new package's runtime tools. Use the current C++ programs to start a new self-contained experiment, or reproduce the old run in its original directory with its frozen tools and protocol. Do not overwrite a frozen output folder.

For current-package validation, build the checks and run:

```bash
conda run -n kaggriculture cmake --build tile_planner/build --target check_ordered_stock check_repeated_service check_preparation_repair check_life_compile_reuse check_life_day_cache --parallel 2
conda run -n kaggriculture python tile_planner/tools/check_package.py --output tile_planner/runs/package_check
```

Shared source and runtime hashes are in `REPOSITORY_DEPENDENCIES.json`. The historical dependency versions are needed when reproducing old measurements. If a shared estimator/compiler changes, recheck cache assumptions and representative full courses; a previous timing or correctness result does not certify a new backend.

## Next work and controls

The measured bottleneck is complete affected-day certification, not proposal generation. Preserve strong source/nearest/animals-first controls and share new routing witnesses back to the unchanged layout. Keep every unsupported input, timeout, market failure and resource deficit in the coverage counts. The explicit repeated-service extension is later than the frozen final method and must remain separately labeled.

Prioritize timed supply/decay commitments and certification of all affected later days before fitting more placement rules. A separate service-calendar study could inspect repeated fertilizer applications, but only a static candidate count was recorded; no removal gain is established. Demand rules must follow useful complete counterfactuals and use only information available when placement is chosen. Quadrants remain globally coupled through workers, shed trips, capacity and hiring thresholds.

The original 5% quality and 20% complete-search CPU criteria remain in `PROTOCOL.md`; both lack a broad pass. Exact reuse is retained under a narrower result. A new final test needs new unexamined strategy families: the original final inputs have now been studied, and several nominal teams share public tapes.
