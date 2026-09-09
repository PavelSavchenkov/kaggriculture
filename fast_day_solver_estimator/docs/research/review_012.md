# Review 012 — 2026-09-09 04:56 UTC

Decision: continue, reprioritizing cheap supply checks and compiler-call prediction. The heavier route-order feature has not earned its cost. Keep minimum-workforce estimation separate from predicting what a bounded compiler can certify.

Evidence:

- Frozen v1 performs substantially worse on the independently generated synthetic challenge. Among 170 certified contracts, timing_extra has family-macro workforce MAE 1.241 and overall MAE 1.415, with overall signed error -1.124 against the verified reference upper bounds. The ridge secondary is better here: macro MAE 0.853. Large gaps occur on dense, distant and late-purchase workloads. These are errors against certificates, not proven minimum-workforce errors. Preserve the initial frozen result; no retuning has been applied to it.
- Extra routing features cost approximately 0.5–0.65 ms and do not consistently improve layout or marginal decisions. Set them aside from the default candidate. The routing-specific work-removal regressors also fail to beat the cheaper timing models.
- The new carried-input necessary condition costs mean 1.065 us / p95 1.543 us. It passes 4,445 valid source/compiler witnesses and 42 boundary controls without contradiction. It proves 39 additional synthetic cases and ten additional development layouts unreachable. Five other synthetic lower bounds improve, all on unresolved contracts. Certified-case point accuracy does not improve from this screen alone.
- Marginal slices now preserve baseline cost, task-change size and direction. Only five certified work-removal pairs from two parents have baseline bill at least 500. In that tail, original flat10 MAE is 195.8, timing_boost 123.59 and timing_extra 167.93. Do not generalize the much larger overall percentage gain to arbitrary expensive expansions.
- A held-family model for a proposed three-second compiler call's success and CPU is implemented. It reads the physical features and requested workforce k, never the source answer or future outcome. Its first dynamic query replay reaches the best reference layout cost in about 3.12 CPU seconds on average. After strengthening simple controls to skip workforce counts already excluded by analytical bounds, direct-cost ranking with nearby-worker ordering takes only 2.895 seconds and beats the learned query policy on this mean. The learned policy has slightly better 30-second cost (113.31 versus 115.17). Do not claim a mean speed gain over the strongest control. These are development proxies with provisional prediction charges, not accepted full-pipeline results.
- The original pipeline is a collection of proposal/forecast/course-compiler runs, not one completed general optimizer. Its later compiler tries source-visit reuse, restricted repair and then root scheduling, retaining source hiring calendars. Our current cold, normalized-calendar query replay does not reproduce that warm course compiler. An actual integration comparison remains necessary before making a full original-pipeline speed claim.
- A 402-query, 30-second synthetic adjudication pass is running on 148 non-screened contracts. Existing short-sweep outcomes remain fixed. Longer fresh adjudication was 1,002/1,130 queries complete at the last count.

Next 20 minutes:

1. Identify which pools drive query-policy gains or regressions. Test simple-first ordering with model use after an observed compiler failure. Bootstrap at the parent-family level; classification AUC alone is not an acceptance metric.
2. Export a small deterministic C++ query predictor and measure complete scoring cost before freezing a new policy. Compare nominal-budget and predicted-CPU ordering; the first log-CPU regression is a geometric-mean heuristic.
3. Incorporate tighter reference evidence when complete, without overwriting operational outcomes. Decide whether to freeze the next candidate or gather more development diagnostics before opening holdout_a.
4. For the next untouched test, generate the allowed query range from physical inputs, independent of source workforce. Existing reference ceilings are metadata for offline evidence and must not become a deployment input.

Holdout_a and holdout_b remain unopened. No estimator is accepted. Next review 05:16:39 UTC; deadline September 10 00:56:39 UTC.
