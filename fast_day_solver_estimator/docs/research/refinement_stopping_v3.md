# Observable stopping after a certificate

Status: development only. The ordinary cold cost component remains accepted separately. These stopping rules have not passed a fresh prospective gate or the complete original warm pipeline.

`include/refinement_policy.hpp` keeps the accepted around order until the first verified schedule. It then evaluates remaining cheaper calls using the new context model. The expected-saving heuristic is predicted three-second success probability times the possible hire saving, divided by three seconds. A supplied threshold can stop further refinement. Alternatives preserve around order, order calls by this utility, stop after the first certificate, or allow fixed numbers/times of extra calls. The policy never consults an unqueried result or the best reference answer to decide when to stop. Failure remains UNKNOWN.

The recorded backend calls are unchanged. Complete termination includes unsuccessful improvement attempts, not just time to reach the best reference retrospectively. Fixed-time controls finish a call that crosses their refinement cap; the actual overshoot is included. Separate budget reports credit only certificates completed within the budget.

On 48 exposed development layout pools with held-family forecasts, utility threshold 1 uses 5.179 seconds and adds 0.042 to the mean bill, compared with 35.121 seconds for exhaustive around search. The worst extra bill is 2. One additional around call uses 5.208 seconds and adds 7.896; a ten-second refinement cap uses 13.205 seconds and adds 3.708. These are Python replays with provisional prediction charges.

On the already exposed second wave, full C++ replay matches all 690 policy tests and 3,489 query choices exactly. Cost scoring, context extraction, lazy query inference and decisions cost about one millisecond per five-proposal pool. File parsing and report output are excluded. Results on 46 pools:

| Rule | Complete CPU seconds | Mean added bill | Worst added bill |
| --- | ---: | ---: | ---: |
| Exhaustive around | 40.830 | 0 | 0 |
| First certificate | 1.070 | 22.087 | 144 |
| One additional around call | 4.224 | 3.130 | 89 |
| Two additional around calls | 8.561 | 1.935 | 89 |
| Ten-second refinement cap | 13.126 | 0 | 0 |
| Utility threshold 0.1 | 28.296 | 0 | 0 |
| Utility threshold 1 | 7.504 | 1.370 | 55 |
| Utility threshold 10 | 2.268 | 13.152 | 144 |

All retain a certificate in every pool and preserve first-certificate query choices. These rows show a compute/cost tradeoff, not one universally best setting. A simple time cap is a strong control, especially when exact reference quality is preferred. The new model and thresholds were developed after this wave was exposed; no row is a fresh-test claim.

Evidence: `runs/stopping_query_controls_v2`, `runs/holdout_a_stopping_controls_v3`, and `runs/holdout_a_refinement_cpp_v3`. The latter contains per-pool signed regrets, actual C++ timers, all query choices and input/source/binary hashes.
