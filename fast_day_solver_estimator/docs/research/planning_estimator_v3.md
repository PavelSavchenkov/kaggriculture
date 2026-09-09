# Experimental C++ planning estimator

`include/planning_estimator.hpp` exposes `labor::estimate_planning(problem, menu, force_curve=false)`. It is a pure estimator: it does not invoke the day solver, inspect candidate schedules, or use source workforce as a feature. The caller supplies the unscheduled physical work, active horizon, and explicit fixed and optional hire slots through `fixed_planning_menu`. Fixed hires are real commitments, not hidden answer metadata.

The result contains a fractional workforce estimate, interpolated hiring cost, necessary lower bound, an analytical rejection flag, query-success forecasts, and difficulty flags. An analytically rejected input returns NaN cost/workforce; the caller must inspect the rejection flag. Unsupported or malformed inputs fail validation. Low predicted success is never reported as a proof of infeasibility.

For an ordinary 24-phase day with the earliest optional hire menu and no fixed hires, the point predictor remains the previously accepted direct-cost forest, clamped to the context's necessary floor. A probability check at its rounded-up workforce tests whether this proposed solver call looks plausible. If probability is at least 0.5, the estimator returns immediately. Otherwise it evaluates all allowed worker counts. `weak_probe` means this point probability is below 0.2; `low_peak` means every allowed query has probability below 0.5. The easy return is sufficient to establish that the peak is not low without finding its exact value.

Other calendars use the full query curve. Take its running maximum over increasing workforce, normalize by the peak and use the resulting mean workforce as the point estimate. This is a heuristic transformation of bounded-solver success, not a calibrated distribution of minimum labor. The result also exposes curve quantiles and mean hire cost for diagnostics. These alternatives have not replaced ordinary direct-cost ranking: they made an exposed layout search slower despite reasonable worker-error averages.

Supported contracts start at dawn with the farmer empty-handed at the specified shed start. Horizons are H24 and the terminal H23. Worker birth times, market-slot conflicts, fixed hires, and zero-action late hires are explicit. Terminal work must finish by phase 22; virtual phase 23 never supplies work. Cash, shed capacity, rival behavior, weeds during a rollout and arbitrary mid-day state remain outside the physical prediction target. Full-game use still requires the unchanged downstream compiler and replay.

The new-family input-only check covers 1,379 unique contexts, 55,160 query predictions and 8,274 adaptive/forced planning outputs. All numerical comparisons pass the existing mixed tolerance of 1e-7, with maximum planning discrepancy 8.0e-9. Balanced C++ timing on these inputs:

| Path | Mean | p95 | p99 |
| --- | ---: | ---: | ---: |
| Adaptive ordinary | 69.69 us | 135.15 us | 160.89 us |
| Forced full curve, ordinary | 129.23 us | 160.79 us | 168.60 us |
| Other explicit calendars | 86.56 us | 132.65 us | 139.62 us |

Timers include menu/features/bounds, cost inference, probability checks and curve computation. They exclude JSON parsing and file output, which are offline harness operations. These are prediction timings, not complete optimization-iteration speedups.

The frozen candidate is `snapshots/planning_wave_v3`. Inputs and overlap audit are in `runs/holdout_b_inputs_v3`; forecasts and verification are in `runs/holdout_b_planning_predictions_v3` and `runs/holdout_b_context_check_v3`. The reference sweep is running under `docs/unseen_wave_v3.md`. General accuracy, hard-case capture and calendar transfer are unaccepted until those results are available.
