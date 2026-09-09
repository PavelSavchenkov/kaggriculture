# Review003 — 2026-09-09 01:56 UTC

Decision: continue the estimator task. Shift the next assessment from absolute workforce fit toward selection among matched alternatives. Do not select a model from the small partial comparisons yet.

Evidence:

- Historical first comparison used206 completed reference cases. Fitted geometry MAE was0.692/0.773 workers across archive/root groups; adapted old formula4.905/3.464. Historical groups are exposed and have broad shared ancestry. A scaled ridge model failed badly when extrapolating between them; investigate scale instability before considering that model.
- Fresh first comparison used85 completed cases. Five individual-team folds had at least10 examples each. Mean fold MAE: adapted old formula3.475, fitted geometry1.040, ridge0.840, extra trees0.905, random forest0.969. No untouched validation or test families have been read. These are diagnoses, not final accuracy claims.
- Added a tuned task-only baseline to distinguish geometry gains from simple retuning. Shared physical contracts belonging to multiple teams are now excluded from family-generalization comparisons; their evidence remains stored.
- Reference progress at01:56: 2,832/3,523 historical queries,1,270/5,866 fresh queries. All are fixed-budget V30 outputs with independent strict replay.
- New paired layout set has48 source parents across8 development teams,240 distinct candidates and2,985 frozen queries. Each pool has identical tile work, endpoint obligations and non-labor value; coordinates are permuted only within ownership quadrants. Changed deadlines may make a layout infeasible, so UNKNOWN remains censored.
- Source normalization verified536/1,578 historical pairs and309/685 fresh pairs. Ten historical source pairs were already invalid. Most normalization failures arise because engine hire spawning depends on shed occupancy, so earlier hires change starting positions. Keep only replayed normalized certificates. The lower bound never exceeded any valid source workforce in these2,253 pairs.
- Corrected two fresh extractor metadata failures; both now strictly pass with unchanged physical contracts. Record899 supported ordinary source days, but keep the frozen897-case input index and ongoing reference sweep unchanged. Amendment evidence is separate.

Priorities:

1. Export per-case predictions, fit coefficients and controlled model snapshots. Compare task-only, geometry and geometry-plus-residual models under family exclusion.
2. Measure normalized source witnesses against the short-solve upper bounds, then adjudicate consequential uncertain cases more deeply. Do not train on a missed-schedule artifact as if it were a minimum cost.
3. Evaluate the paired layout pools once reference evidence is sufficient. Keep unknown alternatives visible in decision metrics.
4. Implement and time the first compact C++ predictors before reading a new validation family.

Next review02:16:39 UTC. DeadlineSeptember10 00:56:39 UTC. No accepted predictor or submission yet.
