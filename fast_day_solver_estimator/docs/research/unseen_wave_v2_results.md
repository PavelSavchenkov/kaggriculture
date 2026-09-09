# Second frozen wave

The direct-cost model with nearby-workforce queries passes the predeclared cold-layout gate. This is a scoped result for ordinary days with the earliest permitted hire menu. The general marginal estimator and original warm pipeline are not accepted.

The model was frozen at 05:20:42 UTC, before downloading holdout_a. All predictions preceded reference calls. The unchanged V30 sweep completed 34,840 three-second queries on 974 physical contracts. There are 910 verified upper bounds, 50 output-deadline impossibility proofs, and other unresolved inputs. Source witnesses and timed compiler outcomes remain separate.

The overlap audit leaves 560 novel source contracts, 46 novel layout pools and 47 novel work-subset pools. The source set has 558 verified upper bounds and two unresolved contracts. Ten physical source contracts occur in more than one new family; overall metrics count each once and family metrics retain each membership.

## Cold layout search

| Method | Mean time to reference-best bill | p95 time | Best bill within 30 s | Mean verified bill at 30 s |
| --- | ---: | ---: | ---: | ---: |
| Original geometry + nearby queries | 13.206 s | 40.002 s | 43/46 | 392.29 on 45 certified pools |
| Direct-cost forest + nearby queries | 1.616 s | 4.644 s | 46/46 | 153.98 on all 46 pools |
| First-failure hybrid, nominal budget | 1.827 s | 7.675 s | 46/46 | 153.98 |
| First-failure hybrid, predicted CPU | 1.595 s | 5.558 s | 46/46 | 153.98 |

The primary reduction is 87.76%. The family-bootstrap interval for saved mean CPU is [5.454, 18.434] seconds, above zero. Mean scoring cost for the direct model is 81.04 microseconds per proposal in this replay. It is included, along with measured policy decisions and recorded backend CPU. All 24,145 choices in the novel C++ replay exactly match the frozen policy; the full panel passes another 25,020 choices.

The nominal-budget hybrid is 13.04% slower than the direct model and fails its optional gate. The CPU hybrid gains only 1.30%, with an interval crossing zero and no 30-second quality gain. Keep both experimental. Its apparent full-panel gain is much larger; the novelty exclusions matter.

The three material primary regressions take 3.464, 1.871 and 1.129 additional seconds. Their final 30-second bills are unchanged. Individual pools and family differences remain in the report.

Time to reference-best bill is retrospective: a caller cannot know when that bill has been reached. The fixed-budget results are the actionable quality measure. These results do not establish an 87.76% reduction in complete warm-course iteration time. Parsing and report output are excluded from scoring timers.

A separately labeled, post hoc stop-after-first-certificate diagnostic gives direct-model mean time 1.069 seconds and bill 176.07, versus geometry 3.258 seconds and bill 1,490.48. The saved-time interval [-0.796, 7.628] crosses zero; the saved-bill interval [855.70, 1,882.19] does not. Only 30/46 direct first certificates are reference-best. The nominal hybrid's mean first bill is 6,435.50, so its rapid first success can be expensive. These results are in `FIRST_CERTIFICATE.json` and do not change the predeclared gate.

## Source and work-changing forecasts

On 558 labeled novel source contracts, direct-cost workforce-equivalent MAE is 0.497, versus original geometry 3.624. Family-macro MAE is 0.451 versus 3.821. Direct-cost mean signed error is -0.079 workers and -63.83 cash; 2.51% of cash forecasts underestimate the upper reference bill by at least 500. Original geometry overstates by 1,321.94 cash on average. These are reference upper bounds, not minimum-cost truth.

For 166 labeled novel work-subset pairs, direct-cost marginal MAE is 26.49 versus original flat10 214.80: an 87.67% reduction. The paired family-bootstrap saved-error interval is [145.96, 227.96]. Signed bias moves from -212.00 to +7.24. Twenty-two pairs remain censored. Timing plus boost has MAE 22.26 and bias -4.23, also with no errors of magnitude 500 in this set.

These work-subset cases mainly test removal savings. They are not evidence that large additions are solved. The independent addition reports still show severe underestimates, especially on rare expensive pairs; see `docs/expansion_results_v1.md`.

## Original full warm pipeline

The separate necessary-screen integration fails its full benchmark: only two calls skipped out of 568, 4.35% higher mean CPU, the same six failures, and 58.25 higher mean hire bill on common successes. That experiment is `runs/warm_screen_benchmark_v1/REPORT.json`. Do not combine its timings with the cold replay.

## Evidence and adoption

- `runs/holdout_a_gate_v2/GATE.json`: fixed gates and verified frozen-file hashes.
- `runs/holdout_a_layout_novel_evaluation_v2/MEASURED_RESULTS.json`: primary timing, quality, tails, families and regressions.
- `runs/holdout_a_source_novel_evaluation_v2/RESULTS.json`: source accuracy and signed errors.
- `runs/holdout_a_work_novel_evaluation_v2/RESULTS.json`: marginal errors, paired intervals and original-estimator controls.
- Corresponding `full_evaluation_v2` directories retain unfiltered results.

Adopt `search_v2_cost_only_h24_cold` for this scoped cold comparison, while retaining the frozen model unchanged as a control. Do not adopt the optional query model. Keep working on additions, inherited calendars, terminal support and observable stopping rules. Holdout_b remains unopened for a later prospective test.
