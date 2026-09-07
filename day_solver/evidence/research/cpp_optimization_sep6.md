# C++ optimization results, 6 September 2026

The selected solver is `work/native_day_scheduler_v3`, wrapping the v17
portfolio. Gate 12 closes at 1,376/1,376 strict unseen schedules from 60 episodes,
including 1,022 hard days. There are no extraction or conversion failures.
Median 363.308 ms, mean 1.125905 s, p90 1.079736 s, maximum 50.271813 s;
963 finish within 500 ms and 1,226 within 1 s. Independent replay and the inventory
ledger agree on all 33,024 hours. Evaluation took 785.665 seconds with two cases
running concurrently. Sources, budgets, extractor and checker froze before
opening any selected replay.

## Changes retained in v17

- Reuse immutable exact-model data within a solve, including tile graphs and
  worker release data. Use bitsets for board reachability. Do not cache an
  original route or a schedule across public calls.
- Avoid constructing diagnostic exact-model variable names. Preserve model
  indices, constraints and construction order.
- Reuse neighborhood working buffers; restore only the two changed routes;
  reserve capacity and select only the needed top candidates.
- Remove redundant coarse ordering decisions already implied by route order
  and mutually exclusive pickup choices.
- For an item with no initial stock and no within-day producer, prevent pickup
  before its first purchase becomes available. Retain flexible local reuse of
  wheat and fertilizer; retain the cumulative purchase-stock guard.
- After the ordinary quick pass fails, try a short fixed-work repair before
  the complete fallback. This reduces sensitivity to timed repair cutoffs.

All accepted schedules still pass strict replay. Model equivalence does not
make wall-time-limited search deterministic: faster code can change which
proposal is returned before a cutoff.

## Development and component checks

All 323 real development inputs and nine synthetic controls pass. On the same
323 inputs, observed median/mean/p90 changed from 361/1,213/1,522 ms in v6 to
329/760/708 ms in v17. These were separate runs on a shared workstation.

A separate six-case comparison alternated frozen v6 and v17 for five repeats,
with 60 strict schedules and 1,440 independent hours. Five case medians improved
modestly. The sixth, a known timing-sensitive case, changed from 66.920 s to
0.432 s. It dominates the summed result; do not call this a universal 47-fold gain.

The compact kernel matched all 432 candidate comparisons, covering 1,734,062
candidate generations; sampled kernel time fell 9.8%. Cached exact construction
matched 72 models and 48 strict schedules. Removing exact variable names matched
another 72 normalized models and 48 strict schedules. Ordered coarse controls
passed 108 cases; pickup readiness passed five boundary checks and three strict
schedules. These are component controls, not unseen coverage claims.

The full regression suite and all 12 typed API controls pass. Integration tests
check strict replay, unchanged input, zero-budget UNKNOWN and malformed input
or budget rejection. The linked API executable defines the optimized kernel
symbols, so the new kernel is actually included.

## Prototypes kept separate

V18 first tried the constructor's worker groups for 0.3 s, then relaxed them on
failure. It passed 81/81 strict but was slower: median 300 ms, mean 1.400 s versus
v17's 253 ms/821 ms on its earlier 81-case run. 48 trials succeeded; 111 failed trials
cost 9.925 s. This is not a Crop Dusta invariant and is not selected.

The unnamed coarse model passed 108 normalized comparisons and 36 strict
schedules. Construction improved from 106.4 ms to 88.2 ms across the timing sample,
but whole-solve case medians did not improve. It is not integrated.

V19 skips only a literally identical constructor proposal after a definite
coarse INFEASIBLE result. It never caches UNKNOWN or exact-completion failure.
It passed 81/81 strict and the full suite, with seven cached rejections across
three cases. Its observed median 288 ms and mean 955 ms did not beat v17's earlier
81-case run. No promotion or new unseen claim is justified by that result.

## Evidence and limits

- [Fresh first-attempt results](../work/crop_dusta_gate12_reserved/final_summary.json)
- [Freeze](../work/crop_dusta_gate12_reserved/gate_plan.json)
- [323-case summary](../work/native_quick_portfolio_v17/development_summary.json)
- [Paired timing](../work/native_quick_portfolio_v17/paired_timing_v1/timing.json)
- [Public API](../work/native_day_scheduler_v3/README.md)

The 900 s allowance remains a soft total budget; fast trials use one CP thread
and fallback uses eight. Uniform subsecond latency is not achieved. v3 ignores
shed capacity and excludes random nighttime weeds. Days within episodes are
correlated. Cross-cohort speed comparisons are not paired estimates.
The latest metadata query returned the same 531 episodes for submission 55929317;
there are no new unused episodes in that listing after Gate 12. Future changed
solvers must not be described as unseen-tested by reusing this cohort.

Final summary SHA256:
7514c40f5796c280d5eecbfe796e2f0b1432d3970a94addd81fd2797afeaa583
