# Formulation and API

## What is being estimated

Given an unscheduled day contract, estimate the smallest feasible workforce and its hiring bill cheaply enough to guide strategy search. Most available targets are the smallest workforces with verified schedules found under specified compute budgets, not proven optima. Keep those operational targets separate from necessary lower bounds and stronger physical upper bounds.

The input is essentially the day solver's public v3 problem with worker count left to the estimator: dawn tile states, ordered required tile operations, seeds and shed inventory, purchases and their order slots, cumulative withdrawals, exact end-state/inventory obligations, and active horizon. Provide a permitted hiring menu independently of any source schedule. Current scope starts with the farmer empty-handed at the shed. Arbitrary mid-day starts are unsupported.

The JSON schema retains `worker_count` for compatibility; use a dummy legal count with matching hire metadata when constructing pure estimation inputs. The estimator ignores that answer field. It never infers the optional hiring menu from how many hands the source happened to use. Genuine fixed hiring commitments must instead be supplied explicitly through the menu API.

H24 has real phases 0..23. Terminal H23 has real work only in phases 0..22; virtual phase 23 supplies no work. Purchases at hour h first become usable at h+1. Seeds are global; carried inputs require pickup. Workers can preposition before global seed/land releases. Later workers may perform a following operation on the same tile in the same phase. Carrying is unlimited in the physical model. Cash, shed capacity, rivals, and full-course botanical/economic outcomes require downstream game verification.

## C++ entry points

`fast_day_solver_estimator::estimate_day(problem)` uses the ordinary earliest optional hiring menu. `estimate_day(problem, menu)` accepts an explicit `labor::PlanningMenu`. Construct a fixed/optional menu with:

```cpp
std::vector<std::pair<int, int>> fixed;    // (hour, market slot)
std::vector<std::pair<int, int>> optional;
auto menu = labor::fixed_planning_menu(problem, 24, fixed, optional);
auto prediction = fast_day_solver_estimator::estimate_day(problem, menu);
```

Fixed hires are selected before optional hires when constructing a proposed workforce. They are not globally sorted together to define worker IDs. The menu has at most 39 hands; counts include the farmer, so valid total workforces are 1..40. The sentinel 41 means unsupported/excess workforce and must never be priced as 40.

The result exposes:

| Field | Meaning |
| --- | --- |
| `analytically_rejected` | A necessary bound or resource condition rejects the available menu; point fields are NaN. |
| `lower` | Necessary workforce lower bound, not a feasible schedule. |
| `workers`, `cost` | Fractional cost-equivalent workforce and interpolated Fibonacci hire bill. |
| `uses_direct_cost` | Ordinary H24/earliest/no-fixed path uses the frozen cost forest. |
| `probe_workers`, `probe_probability` | Three-second cold-solver success forecast at the rounded-up ordinary point. |
| `weak_probe` | Available point probe is below 0.2. Unavailable probes are not evidence of confidence. |
| `low_peak` | The maximum forecast over allowed queries is below 0.5. |
| `complete_curve`, `probabilities`, `forecasted` | Whether all allowed counts were evaluated and which entries are available. |

Ordinary inputs return early when the point-query probability is at least 0.5. Otherwise all allowed counts are evaluated. Other calendars always use a curve-derived point estimate. Running-max normalization of solver probabilities is a heuristic; its quantiles are not calibrated confidence bounds on minimum workforce. General calendar accuracy remains unaccepted.

`marginal_cost(baseline_estimate, candidate_estimate)` returns candidate cost minus baseline cost, or no value if either was analytically rejected. Negative means predicted saving. Retain both difficulty flags. Major additions can be badly underestimated; a cost difference alone is insufficient to accept a plan.

## Choosing solver queries

The accepted ordinary cold policy is in `query_policy.hpp`: score each same-value proposal with `CostModel::direct`, then use `QueryPolicy(..., QueryOrder::around)`. Supply an allowed-workforce bit mask and deterministic tie ranks. `next()` returns the next proposal/count; `observe()` accepts only an actual verified success or a completed bounded failure. Read the header and `source/replay_search_cpp.cpp` for the complete caller pattern. Do not use knowledge of the best reference bill as an online stopping condition.

For a specific proposed count, `extract_context(problem, menu)` and `context_query_features(context, k)` supply the frozen `context_model::boost` success model. Enforce the menu's minimum, size, and necessary bounds before scoring. This predicts a bounded compiler outcome, not mathematical feasibility. The pure predictor cannot access source schedules, candidate outcomes, source workforce, or latent game state. A caller may use its own completed calls in separately documented retry policies.

The optional `long_retry_policy.hpp` is experimental and applies only after a known certificate already exists. The warm `.02` deferral policy retains source-route reuse and retries deferred calls if other choices fail. It passed one fixed-fixture seed-transfer test but failed an earlier exposed quality test. The `.10` after-repair stage variant is unaccepted.

## CLI menu format

Run `estimate_day problem.json 23 menu.txt` for H23 with an explicit menu. The whitespace-separated menu starts with fixed-count and optional-count, then fixed `(hour slot)` pairs, then optional pairs. Zero optional hires is allowed. The CLI's convenience default constructs the earliest 39-slot menu; an explicit menu can provide fewer slots.

The new `seed_suffix_bounds.hpp` prototype is not part of the frozen default estimator. It has exact constructed controls but still needs broader witness, timing, and decision validation.
