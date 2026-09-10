# Preserve the state needed to value a later period

`ContinuationResult` now returns shared market inventory, revealed shops, shop count and the end turn, alongside its existing accounts, resource buffers and error counts. Previously those shared fields were lost at the end of a call. A caller can now resume the financial scenario or price its later effects without inventing an ending market.

The returned phase is after the supplied period, before the next turn's shop arrivals and work. To resume, reveal that boundary's supplied scenario shops and apply our next pre-market resource events once. Rival events remain the evaluator's responsibility. Prefix commitment errors must be retained when combining reports.

The release check passes on all 297 extracted episodes: both seats, original and compacted orders, and 94 split points per plan. All 111,672 split/resume comparisons match the one-pass evaluation's accounts, resource buffers, deficits, discards, commitment errors, market inventory, shops and end turn. Another 55,836 prefix cash/market comparisons match the recorded source states under original orders. All 1,188 whole evaluations are feasible under their supplied financial calendars. The full check, including case loading and source certification, takes 8.70 seconds.

Address and undefined-behavior sanitizers pass on two complete episodes, both seats and the same split/compaction coverage. This validates the result contract; it does not certify future biology, routes, opponent compatibility or improved strategy quality.

## Cost and preserved control

The original header is retained in `control_include/continuation.hpp`, with its hash checked against the old build metadata. Both old and new benchmark binaries use the same existing evaluator client and supplied calendars. No sale-policy logic changed.

Five alternating longer pairs, each 13,600 complete-calendar calls per binary, have a median new/old elapsed ratio of 1.012: about 1.2% higher. Individual pair ratios range from 0.996 to 1.157. Shorter measurements also contain large outliers. Thus the median change is small, but these noisy timings do not establish an exact overhead or a speed improvement. Loading, extraction and live-policy work are excluded from the timed region. All raw measurements are retained, including unfavorable pairs.

Evidence: `PROTOCOL.json`, `RUNNER.json`, `SUMMARY.json`, `SANITIZER_STATUS.json`, `CONTROL_SOURCE.json`, `PERFORMANCE_RUNS.json`, `QUIET_PERFORMANCE_RUNS.json` and `LONG_PERFORMANCE_RUNS.json`. `scripts/measure_continuation_cost.py` reproduces alternating measurements; use a new output label to preserve earlier runs.
