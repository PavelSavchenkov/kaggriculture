# Empty-sale removal validation

Rejected under the preregistered per-game direct-parent margin gate in both fresh and native panels. Aggregate league gains and operational checks do not override that failure.

67,584 fresh games, 5,632 native/PASS games, 1,024 operational games, and 64 rebuilt full records. All frozen source hashes unchanged.

Direct parent: 906 wins, 38 ties, 80 losses; mean margin +239.62.

| Panel | Parent utility | Candidate utility | Gain 95% interval (percentage points) |
| --- | ---: | ---: | ---: |
| current | 93.956% | 94.725% | +0.709 to +0.835 |
| historical | 95.744% | 95.864% | +0.043 to +0.214 |

Paired parent regressions:

- fresh: 1 cases, minimum margin gain -6. Exact seeds and cash changes are in the central report.
- native: 1 cases, minimum margin gain -6. Exact seeds and cash changes are in the central report.

The first order change at270 has no economic effect in this case. The causal change is at598: removing an empty milk sale advances11 strawberries. Both players otherwise sell11 strawberries on the same slot. The floor-price rule only increments market inventory when the quote exceeds1. Sequential sales leave strawberry inventory one lower than simultaneous sales (10062 versus10063). Immediate rival cash falls2, but subsequent unchanged trades pay more to both players, ultimately +47 own/+53 rival. Input movement is not the cause of this failed case.

The floor-price mechanism is proved for native seed2004097 seat0 only; the other failed fresh cases are listed for later diagnosis.

Reproduce in order: prepare.py, run_fresh.py, analyze_fresh.py, run_native.py, analyze_native.py, run_checks.py, check_frozen.py, run_trace.py, analyze_trace.py, finalize.py. Output folders intentionally fail if they exist; preserve existing evidence and use a clean copied run with adjusted output paths for reruns. Read FRESH_PREREGISTERED.json before testing. The isolated frozen/FROZEN.json contains its exact rebuild command.

Preserve empty slots when a later shifted sale can reach the price floor. Test guards using own quantity, a matched rival-quantity assumption, and the rival shed-capacity bound. These are observation-based economic conditions, not seed or opponent exclusions. A tail-only non-input rule would not prevent the demonstrated strawberry failure.
