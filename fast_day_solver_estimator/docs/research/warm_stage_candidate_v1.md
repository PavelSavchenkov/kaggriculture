# Cold fallback deferral after repair fails

The original per-day policy spends 1,140.26 recorded CPU seconds across 111 reached dawns in the exposed complete-call catalog. Its 166 failed executed calls account for 968.45 seconds, including 771.32 in cold fallback. This identifies the expensive stage to predict. Catalog collection itself costs more because it evaluates all seven extra-hire counts; that total is not an iteration runtime.

The existing .02 whole-call deferral replay saves 24.80% of per-day CPU with unchanged recorded bills. A hindsight oracle that jumps to the cheapest known successful call saves 76.40%; this is a headroom diagnostic, not an implementable policy. Selecting the fastest known call regardless of its bill raises mean daily hire cost by 230.34 and is not an acceptable matched-bill comparison.

The separate stage candidate keeps original visit reuse and fixed repair. Only after repair fails does it compute the existing calendar-aware probability and defer cold fallback when that probability is below .10. It proceeds through other extra-hire counts in original order. If all fail, it retries deferred cold calls in order without repeating repair. The complete serialized day input must match the saved deferred input exactly. No learned flag permanently rejects a call or claims physical infeasibility.

This is a call policy with access to its own observed repair failure. The pure labor estimator still receives only unscheduled obligations and explicitly proposed calendars/workforce. No future cold result or candidate route enters the decision; models and weights are unchanged.

In the development catalog, 238 calls reached cold fallback and seventy returned verified full-game endpoints. None below probability .25 succeeded. The selected .10 threshold defers 101 calls and saves 42.48% CPU in per-dawn replay, with no recorded bill increase or lost certificate. A .20 threshold saves 48.59%. Keep those numbers exploratory: the chosen .10 is deliberately below the observed successful range, and altered schedules can change later days. A complete independent benchmark is required.

Two development correctness controls are complete. The wheat course passes all 719 independently replayed transitions. The carrot course remains unresolved and exercises three deferred retries without an input mismatch. Sixty distinct probability predictions match the standalone C++ context model exactly; the controls have 35 deferrals in total. Their runtimes are not used as a comparative speed result.

Candidate and test protocol were frozen at 11:43:38 UTC, before extracting the new seed group. All 56 pairs are eligible: eight SHA256-selected seeds times the same seven exposed course specifications, with no exclusions. Seeds are 769316092, 1043827778, 2758895230, 2366201495, 4037497062, 2849440660, 3397005418 and 2851305734. They are disjoint from the ongoing .02-guidance seed panel and the original seed. Source/rival and shop sequence remain fixed, so this tests environment-seed transfer rather than unseen agents or shops.

Each pair receives original and stage compilers in a deterministic balanced order. Keep complete child CPU, all failures, certificate count, common-success hire bills, production equality, and seed/course bootstrap intervals. Acceptance requires at least 20% mean CPU saving, a positive seed-cluster saved-time interval, no lost certificates and no increased mean common-success bill. This candidate is unaccepted while its benchmark runs.

The shared warm report's `prediction_calls` counts guidance records, including cached predictions on cold retries. A fresh model evaluation occurs only on records with `retry=0`; cached retry records have zero prediction time. Use `SEED_REPORT.json` for the primary new-seed quality and timing gate.

Evidence: `runs/warm_catalog_policy_diagnostic_v2`, `runs/warm_stage_policy_diagnostic_v1`, `runs/warm_stage_source_v1/SOURCE.patch`, `runs/warm_stage_controls_v1/PROTOCOL.json`, `snapshots/warm_stage_v1/FREEZE.json`, and `runs/warm_stage_seed_benchmark_v3`. The earlier warm candidates and third-wave planning predictions remain frozen; their hash checks still pass.
