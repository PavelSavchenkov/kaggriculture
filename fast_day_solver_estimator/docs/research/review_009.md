# Review009 — 2026-09-09 03:56 UTC

Decision: continue. The first candidate is a useful workforce-calibration improvement but does not yet justify acceptance as the best search estimator. Finish terminal reference plumbing, then prioritize routing features and cost-aware prediction rather than more pair-regression variants on the same labels.

Evidence:

- Frozen v1 source validation is fully reported. The exact-overlap-free stratum contains526 physical contracts,515 with a verified upper bound and11 unresolved. Macro-by-family workforce MAE falls0.9013 to0.3606 versus tuned task-only, a59.99% reduction with family-bootstrap95% interval[51.53%,67.78%]. Severe underestimates of at least two workers fall9.32% to0.58%. The original geometry arithmetic's adapted workforce MAE is3.9931. These targets remain verified upper bounds, not proven minima.
- The first-wave acceptance record is `runs/validation_v1_report/RESULTS.json`. The stronger-geometric-baseline layout-cost gate fails, so v1 is not accepted. Source-day accuracy must not conceal this selection failure. The validation wave can now become development data for later candidates; holdout_a and holdout_b remain unopened.
- Measured mean scoring time is58.32 us on new source inputs,82.07 us on new layout inputs and44.81 us on the independently constructed corpus. Corresponding p95 values are81.0,107.65 and149.27 us. Report the100-us tier misses, but the user's average-search-speed clarification makes that tier advisory; the layout-cost failure is independently sufficient to reject v1.
- Work-subset references completed240 contracts:195 certificates,24 analytical unreachable cases,21 other unresolved. This gives changed-operation-count pairs for the original flat coefficients, which layout-only tests could not exercise.
- Historical deeper reference pass completed747 queries. The diagnostic layout pass is near completion. Their upper-bound improvements must remain separate from estimator gains and from the fixed3-second operational results.
- All32 development terminal replays pass full-state parity and strict projected public-contract replay, giving31 distinct contracts. Independent boundary controls pass: required work at phase23 is rejected after truncation, work by22 is accepted, and an unnecessary last transfer can be removed without treating remaining cargo as a real terminal deposit. Explicit23-phase features/bounds and normalized-reference queries are being verified.
- The original ordinary reference binary/source are preserved under `models/reference_ordinary_v0`; terminal-wrapper changes do not alter the frozen ordinary results. Terminal outcomes use a distinct binary that removes the virtual phase and replays before issuing a certificate.

Next20 minutes:

1. Complete horizon feature parity and terminal source-bound checks, then launch the small terminal reference sweep.
2. Add cost-target/leaf-cost models. Fibonacci conversion of a mean workforce is not the same as expected hire cost; compare calibration and decision effects rather than assuming the latter will win.
3. Implement the planned C++ improved route ordering/splitting feature and measure its marginal value and latency.
4. Merge completed deeper evidence without overwriting short-sweep outcomes, and analyze changed-work signed errors against original coefficients0/10/25.

Keep a later full-pipeline test on the work list: current CPU replay assumes physical day contracts already exist and does not include original proposal/forecast construction or the warm compiler. A faster isolated cold-backend proxy alone is insufficient to claim faster full SOTA agent optimization.

Next review04:16:39 UTC. DeadlineSeptember10 00:56:39 UTC. No accepted final estimator yet.
