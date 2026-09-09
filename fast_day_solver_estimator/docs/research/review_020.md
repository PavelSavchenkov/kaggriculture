# Review 020 — 2026-09-09 07:36 UTC

Decision: keep the estimator target and wait for the complete existing comparisons before selecting a new model. Do not spend more effort optimizing the conservative warm screen before its full benchmark result. Prioritize terminal reference quality and calendar-aware learning next.

The frozen cost-training set has 936 labeled contracts, with median ten workers, 99th percentile fourteen, and maximum seventeen. Its tree predictions have little direct training support above that range, which matters when investigating costly underestimates. New expansion, synthetic and calendar evidence must be evaluated rather than assuming tree extrapolation will work.

The new cold-query representation uses 320 values: physical work, active horizon, requested workforce, exact prefix/radius action capacities, deadline ratios, and physical release descriptors. It excludes unused optional slots, full-menu bounds and whether selected hires were described as mandatory. Those details cannot alter an identical cold solver call. C++ and Python match exactly on 6,400 queries; unselected-slot, selected-birth-order and mandatory-designation invariants pass. This is an input implementation check, not a predictive improvement.

Partial full-compiler evidence shows why repeated controls are necessary. Carrot fails under both methods with no skipped trial; cow also fails under both, with one skip among 49 screened trials; melon completes under both with no skips. CPU differences on these partial runs cannot be attributed entirely to pruning because finite-budget parallel repair varies. The complete seven-course/two-repeat benchmark continues, including independent successful-course replay. No speed claim yet.

The terminal reference wrapper currently calls the 24-phase solver and removes phase 23 afterward. It can miss a valid terminal schedule because the solver was not told that required field work must finish by phase 22. Preserve the first reference sweep. Add a separately named wrapper that supplies the same last-real-phase work constraints as the original warm caller, then compare reference evidence and budgeted outcomes explicitly. Do not fit this wrapper limitation as physical labor demand.

At 07:33, reference counts are 25,614/34,840 for the untouched-family wave, 5,860/8,702 for larger/land additions, 5,580/8,211 for small owned additions, 1,465/3,903 for ordinary calendars and 1,181/1,476 for terminal calendars. Holdout_b remains unused, and current prospective parameters are unchanged.

Next priorities:

1. Complete the frozen unseen report and signed actual-expansion reports when each full sweep finishes.
2. Add and validate explicit terminal work deadlines in a separate reference wrapper. Keep old and new backend outcomes distinguishable.
3. Fit calendar-aware and extrapolating cost/query models with whole-family exclusion. Investigate known-baseline route/slack context only if remaining warm errors justify it; do not relabel a day solver as an estimator.

No estimator accepted. Next review 07:56:39 UTC; deadline September 10 00:56:39 UTC.
