# Continuation forecast calibration

No horizon broadly improves the relaxed day-program controller. One-day decisions reproduce it exactly on all128 controls; longer forecasts change decisions with mixed gains and losses. Keep the reusable physical validator, but do not replace the compiler or best agent with these value selectors.

Episode-level calibration only: predicted values sum conditional decisions made at different reached states, so their sum is not an unbiased single-policy forecast. Actual outcomes compare complete policies with the original compiler on matched seeds/seats.

| Horizon | Active /128 | Mean predicted sum | Mean actual cash gain vs original | Positive forecast, negative actual | Mean simulated steps | Maximum act ms* |
|---|---:|---:|---:|---:|---:|---:|
| 1 day | 81 | 626.21 | 690.59 | 17 | 87.2 | 2.68 |
| 3 days | 56 | 876.32 | 564.81 | 8 | 228.9 | 7.17 |
| remaining season | 58 | 1089.67 | 665.33 | 7 | 1062.9 | 34.11 |

*Measured under concurrent audit load; not a controlled throughput comparison.

Improve day contracts and scenario models before increasing horizon or candidate count. Diagnose missing current-state service tasks and use observed rival supply and uncertain future shops when comparing longer continuations.
