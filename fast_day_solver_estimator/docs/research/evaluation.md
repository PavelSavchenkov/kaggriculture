# Evaluation design, version 0

Freeze case membership, label-building budgets and numeric acceptance criteria after the input inventory and before inspecting validation/test outcomes.

## Baselines and candidates

Reproduce both existing estimator formulas, including their actual assumptions. Compare tuned versions on development data so improvement is not credited merely to retuning one scalar. Add cheap spatial/task/deadline heuristics and simple fitted models before considering larger models. Inference includes feature-extraction cost.

The day solver remains unchanged. Offline label generation retains prior solutions and records all attempts; improvements in that label search are not the estimator's gain.

## Data and leakage controls

Historical archive cases and root solver benchmarks are exposed development evidence. Split additional data by complete source family/course; derived days, mutations and near-duplicates stay in that split. Obtain unused whole-family data and, if accessible, freshly downloaded replays for a final frozen evaluation. Do not call archive cases globally unseen.

Prepare both ordinary source days and candidate/baseline edits: work additions/removals, altered layouts, crop/animal mixes, input timing, delivery deadlines, land expansion and staggered calendars. Generated edits must have explicit consistent endpoints and recorded verification. Failed or unsupported extraction remains visible.

Do not include raw answer worker_count, source hire count, case IDs, candidate schedules, solve results or timings in predictor inputs. Optional hire-slot menus must not encode the observed workforce. Audit duplicate hashes and source ancestry across splits. Warm baseline-cost features are evaluated separately from cold predictions.

## Measures

Primary usefulness test: at fixed prediction plus compilation CPU budget, improve the verified value of candidate plans reached, or reduce compile cost at matched promising-plan recall. Use frozen candidate pools and a fixed background value term so only labor estimates differ.

Supporting measures: marginal-cost ranking and regret, costly underestimates near worker thresholds, recall of best certified alternatives, interval coverage/sharpness, workforce error on proven/tight-reference cases, and total per-query/batch latency. Report CPU/wall time and p50/p95. Initial inference target is under 100 microseconds per ordinary day including features; establish meaningful budget tiers by measurement.

If the true optimum is bounded but unknown, report interval-compatible error/regret and sensitivity to improved upper bounds. Do not present source-workforce MAE as optimal-workforce accuracy. Timeouts are censored evidence. Training/test label budgets must be comparable and stated.

## Coverage

All five crops and three animals; one-shot/ongoing crops; new construction/planting/rotations; dense and distant layouts; input pickups and releases; collected fertilizer; harvest-to-feed transfer; withdrawal deadlines; day-end deposits; idle and heavy days; high Fibonacci-cost thresholds; early land; tight orders; final-game hour-22 limit; stale/invalid input controls. Distinguish actual engine capacity/funding checks from physical solver checks.

Use synthetic independently constructed farms for cold coverage and real unfamiliar families for final evidence. Aggregate equally by family and bootstrap at family/course level. Preserve a final unused reserve until the candidate and protocol are frozen.
