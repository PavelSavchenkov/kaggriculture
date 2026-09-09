# Necessary checks in the original warm compiler

The caller proposes a concrete workforce and exact hire times before each warm repair or cold solve. This is a legitimate query argument, distinct from using a source schedule's answer as a feature for an unknown candidate.

`screen_query` builds that exact committed calendar and gives every other slot zero action capacity. It checks total/radius/deadline action capacity, carried-input supply, seed/land releases, and unreachable required outputs. A nonzero mask records which necessary condition failed. It does not invoke a solver or interpret timeouts. Bound values computed with all other slots disabled must not be described as global minimum workforce estimates.

The three witness audits check 4,553 records, of which 4,543 pass strict replay. The ten old metadata failures remain invalid; none of the records is screened out. This includes 98 ordinary warm-course witnesses and 45 terminal witnesses. Some records can share physical work, so these are audit counts, not 4,543 independent examples. Mean query-screen time is 74.477 us and p95 118.031 us, including physical feature extraction and calendar checks, excluding parsing and witness replay.

The integrated variant derives from the original early-input compiler. `runs/warm_screen_integration_v1/SOURCE.patch` records the complete source difference: add necessary-condition logging and skip rejected trials after unchanged construction. Query order, route reuse, fixed repair, cold fallback, full economic replay and first-certificate stopping remain unchanged. The copied original baseline stays intact.

`warm_screen_benchmark_v1` compares all seven imported course specifications, two repeats each, reversing each specification's method order on the second repeat. The benchmark is frozen before these calls. Every successful course gets an independent 719-transition replay. Complete child CPU includes the common launcher, contract construction, repair, fallback, economic checks and artifact output. Failure rate, hire bills and economic differences accompany speed. A faster failed course cannot count as a successful improvement.

The gate requires at least 20 percent mean CPU saving with a positive course-cluster confidence interval, no extra failed courses, and no worse mean verified hiring bill. Individual cost/cash regressions remain visible. These are exposed scenario-level compiler experiments; they are not unseen online-agent results or proof of a general outer-search improvement.
# Completed paired benchmark

`runs/warm_screen_benchmark_v1/REPORT.json` reports fourteen pairs across all seven imported course specifications, with two reversed-order repeats. The original and screened variants each fail six runs and jointly complete eight. Every successful output passes independent 719-transition verification.

Mean complete CPU is 154.905 seconds original versus 161.641 screened: a 4.35% increase. The course-bootstrap saved-CPU interval is [-16.474, 0.890] seconds. Only two of 568 screen calls reject a query; total screening CPU is 0.0673 seconds. The two skipped calls occur in failed course runs. Sparse pruning and variable four-thread repair explain why this is not evidence for a reliable benefit.

On common successes, screened bills are 58.25 higher on average, with a worst increase of 233; own cash moves by the opposite amounts. No certificates are lost or gained. The fixed development acceptance gate fails. Keep the necessary screen as a tested optional component, but do not describe it as an original-pipeline speedup or repeat this benchmark merely to chase a favorable average.

The report writer's first attempt failed on a NumPy boolean during JSON serialization. Its zero-byte output is retained as `REPORT.failed_empty.json`; serialization now precedes exclusive file creation and uses ordinary booleans. Benchmark inputs and measured runs were not changed.
