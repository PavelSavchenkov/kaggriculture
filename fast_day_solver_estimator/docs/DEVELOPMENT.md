# Resuming optimization

## Objective

Improve verified plan quality at a fixed total estimation-plus-compilation CPU budget, or reduce that CPU at matched quality. Cheap worker/cost estimation is the component being optimized; the day solver is the reference and downstream verifier. Do not relabel solver improvements or stronger offline schedules as estimator gains.

The preferred ordinary prediction budget is below 100 microseconds on average including features. It is acceptable to spend more on difficult cases if complete iteration throughput improves. Report mean, p50, p95, p99 and the fraction sent to the heavier path. A more accurate probability model is useful only if it makes better calls or saves meaningful complete CPU.

## Restore the complete checkpoint

```sh
conda run -n kaggriculture python tools/restore_checkpoint.py /absolute/path/to/new_workspace --archive /path/to/checkpoint.tar.zst
```

The archive is a separate research artifact, not part of the lean source folder. The retained local copy is `experiments/v6/fast_day_solver_estimator/distribution_archive/checkpoint.tar.zst`; obtain that file separately on another machine. Its checksum is in `research/ARCHIVE.json`. Running the estimator and included validation does not require it. Full retraining and continuation of the historical panels require its data, models and schedules.

The destination must not exist. Restoration verifies the archive checksum and exact shared dependency hashes, extracts the experiment under `experiments/v6/fast_day_solver_estimator/`, and copies the selected day solver, engine, and agent files from the repository checkout. It leaves historical evidence unchanged. Build afresh; old build directories and machine-specific CMake caches are deliberately excluded.

From the restored experiment:

```sh
conda run -n kaggriculture cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
conda run -n kaggriculture cmake --build build --target reference reference_terminal_deadlines predict_planning predict_context replay_search_cpp -j4
```

The historical scripts expect this experiment-depth layout. `development/scripts/` in the root package is a browsing copy, not a separate relocated workspace. Run scripts from the restored experiment with its complete data and relative dependencies. Do not rerun historical imports from other experiments: their imported source/data are already included.

Historical JSON, commands and worker manifests can contain the original absolute prefix ending in `sep09_labor_estimator`. Those strings are provenance, not current file locations. `RENAMED.json` records the move. Build new runtime manifests with current paths and exact input-file hashes; never rewrite a frozen prediction or reference record in place to make its hash appear unchanged.

## What to resume first

1. Inspect `FINAL_CHECKPOINT.json`. The ordinary third-wave sweep stopped at 30,127/44,023 calls, H23 at 2,711/2,943, H24 completed 2,541/2,541. The stage benchmark stopped at 103/112 child runs and shop transfer at 40/252. Longer-retry holdout calls had not started. These are user-interrupted tests, not failed gates.
2. Complete the missing exact reference query IDs in new continuation directories, preserving the original backend, budgets, candidate weights, inputs and all completed observations. The existing `run_reference.py` creates a fresh whole round and cannot append safely to an interrupted one. Use a new missing-only manifest; interrupted in-flight calls lack valid full timing and must be rerun as newly identified attempts. Verify every resulting schedule.
3. For full compiler benchmarks, retain every finished `RUN.json`, including native failures. Run only unstarted/incomplete method-case invocations from the original specification and within-pair ordering. Do not resume a course midway or mix partial CPU with fresh full-course CPU. Preserve cancellation records, new binary hashes, and changed-machine limitations. The stage continuation script demonstrates preservation of previously completed records; adapt its input root deliberately for a new location.
4. Run the complete-sweep report readers only after completeness checks pass. `report_holdout_b_ordinary_v3.py` and `report_holdout_b_calendars_v3.py` assemble the panels; warm readers evaluate complete paired cases. Correct report readers with preserved before/after hashes and a reason. Do not change scores, thresholds or membership after observing outcomes and still call the test frozen.
5. Assess expensive additions and difficulty flags before fitting another model. The direct forest has a hard training-support limit. Timing-residual models help moderate expansions, while simple range checks and known-baseline anchoring do not solve severe errors.
6. Audit the experimental seed-suffix bound against all existing valid witnesses, explicit calendars and random prefix chains, then measure how often it strengthens existing bounds and changes actual calls. Its 240 exact constructed cases alone do not justify promotion.
7. If fitting again, treat previously viewed/collected families as development. Obtain new whole-family replays after freezing the next model and protocol. Keep a later reserve unused until final selection.

The two published fixtures are `agents/external/early_structure_cow` and `agents/external/nanare_four_quadrant_course`. The latter is one fixed 719-action public course, not an adaptive opponent. A stronger transfer test should vary source agents as well as seeds, shops and calendars.

## Training and export pipeline

The checkpoint includes raw replays, extraction and generator scripts, physical hashes, family splits, manifests, all solver attempts, strict schedule witnesses, assembled datasets, training artifacts, and frozen C++ exports.

- `model_study.py` and `models.py`: whole-family exclusion for formulas, residual models and cost forests; final `models.joblib` records the exact feature schema. The accepted cost fit is `runs/cost_model_study_v3/`.
- `context_query_study.py`, `context_query_models.py`, `upgrade_context_dataset.py`: physical plus explicit selected-calendar features and three-second call outcomes. Source workforce and unused hiring options are excluded.
- `export_search_cpp.py`, `export_context_cpp.py`, associated `check_*` scripts: framework-free export and numerical/decision parity. Do not widen tolerances to hide an exporter defect.
- `freeze_*`, `prepare_*`, prediction and reference scripts: pre-outcome membership, code, weights, input and prediction hashes. Query ranges come from allowed inputs through 40 workers, not source crew caps.
- `frozen_pair_analysis.py`: use `--target-field reference_workers` for best physical upper evidence or `short_sweep_workers` for the original operational sweep. State the choice explicitly. The default is the short-sweep field.
- `adjudicate_dataset.py`: adds stronger physical evidence while keeping original `query_outcomes` unchanged.
- C++ replay and complete warm compilers: measure actual query choices and full runtime, not only the last solver call's timer.

The package records the development Python versions in `evidence/ENVIRONMENT.json`. Offline work uses Python, NumPy, SciPy, scikit-learn, joblib and threadpoolctl; replay acquisition may also require the repository's Kaggle tooling. No credentials are bundled. Execute Python and package-management commands through the `kaggriculture` conda environment.

## Promotion rules

Freeze the primary comparison, membership, compute budget, thresholds, novelty exclusions, grouping, and gates before outcomes. Keep the exact original estimator and the accepted frozen estimator as controls.

| Scope | Required evidence |
| --- | --- |
| Ordinary cold ordering | At least 20% lower mean time-to-reference-best; positive family-bootstrap saved-CPU interval; no worse 30-second certificate rate or mean verified bill; exact C++ choices and scoring cost included. Report observable stopping separately. |
| Marginal additions | At least 20% lower family-macro cash MAE than original flat10; positive family-bootstrap improvement interval; no increase in underestimates of at least 500. Small and large additions pass separately. |
| Explicit calendars | At least 20% lower family-macro worker MAE than geometry with the same necessary floor; positive grouped interval; no increase in underestimates of at least two workers. H23 and H24 reported separately. |
| Difficulty flags | At least 80% of labeled severe underestimates captured while flagging at most 30% of all inputs, including unknowns. Report absolute counts and uncertainty; point errors and marginal-pair errors are separate targets. |
| Complete warm compiler | At least 20% mean complete CPU saving with positive seed/world-cluster interval, no lost certificates, no higher mean common-success bill, equal production, independent 719-transition verification. Retain individual bill regressions. |
| Optional longer refinement | Known-certificate scope only; at least 20% CPU saving with positive family interval and no higher final bill in any tested state against predictive no-stop and nearest-all controls. Include feature/model/selection overhead. |

The new-shop protocol additionally requires at least 24 eligible paired cases across six worlds. All budget/quality tradeoffs must be explicit. These thresholds are the session's chosen gates, not universal statistical guarantees. A gate pass applies only to its tested scope.

## Required artifacts for each candidate

Preserve source/model/compiler hashes; exact input contracts and full hiring menus; family/course ancestry and physical/context duplicate keys; freeze timestamps; raw candidate predictions before labels; attempted query IDs, requested counts, budgets, actual CPU/wall, status and backend hash; schedules and independent replay reports for every certificate; unresolved/excluded cases with reasons; original and improved upper-bound histories; signed errors and large-error slices; grouped intervals and worst regressions; complete paired compiler records when claiming iteration gains; C++ numerical, input-isolation and decision parity; and the exact report scripts and environment.

Review overall progress every twenty minutes during a resumed research session. Prefer a useful cheap control over extra model complexity. Do not silently turn missing evidence into success, discard inconvenient hard cases, or reuse an exposed family as a fresh final test.
