# Estimator contract, version 0

## Inputs

A dawn work contract supplies current managed tile states and layout, ordered field tasks and exact per-harvest outputs, starting shed/seeds, input and land release times, cumulative withdrawal deadlines, exact terminal tile/stock requirements and the permitted hiring rule. It supplies no answer workforce, source route, source name, opponent identity, solver outcome or future market scenario.

The initial hiring rule retains non-hire slots and allows an explicitly supplied chronological list of optional hire slots; query count k activates its first k slots. Raw source hire counts are label metadata and must never leak into cold features. The allowed slot list must be generated independently of that answer count. Future rules can use the same estimator with a new calibrated rule descriptor.

For a marginal query, supply both the candidate and baseline work contracts. A separate warm mode may additionally supply the baseline's verified workforce/cost; it must not read the candidate's schedule or cost. Report cold and warm results separately.

The root v3 solver starts with one empty-handed farmer at (4,4); it has no arbitrary mid-day state or explicit task windows beyond its task order, biological timing and stock deadlines. Do not claim a broader supported interface. Mid-day repair, cash, capacity, actual sale orders and stochastic weeds require separate modules/full-engine checks.

## Outputs

Return estimated workforce and hire cost, uncertainty/intervals or threshold probabilities, and estimated marginal cost where a baseline exists. Valid analytical lower bounds are separate from predictions. Unsupported inputs fail explicitly. No predicted value proves feasibility or infeasibility.

Core prediction is CPU C++ with bounded memory and no solver, JSON, subprocess or schedule access in the scoring call. Python scripts may prepare data and train compact models offline through the prescribed conda environment. Models must export to deterministic C++ inference.

User clarification at02:29: optimize average iteration speed and useful search progress, not uniform simplicity or a hard latency ceiling. An adaptive predictor may spend much longer on rare difficult inputs. Report mean latency, tail latency, deferral frequency and total prediction-plus-compilation cost at matched plan quality. The initial100-us p95 tier remains a useful measurement target, not a global acceptance rule. A heavier method is justified by better decisions and faster complete search, even if it exceeds that tier on hard cases.

## Target and reference evidence

Conceptually the target is the cheapest feasible workforce under the supplied hiring rule. Exact minima are often unavailable. Preserve a proven lower bound and the cheapest strictly replayed upper-bound schedule, plus all bounded solve attempts. An UNKNOWN observation says only that the specified search did not certify that query.

Use two evaluations: prediction against independently built reference intervals, and actual quality/cost of plans reached by a frozen compiler under a fixed total budget. For the latter, bounded solver success is an operational outcome, never an infeasibility claim. Version the compiler and its budget with those labels.

True daily hire cost is the sum of Fibonacci prices 1,1,2,3,5,... . Marginal cost is candidate cost minus baseline cost. Predict daily thresholds before summing a season; a smooth per-operation charge cannot represent an extra expensive hire.
