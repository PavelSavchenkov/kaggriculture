# Initial design

This document will track the implemented design, including limits.

Module boundaries, representation and fidelity may change with evidence. The
user explicitly authorized such changes. Preserve the intended strong-agent
improvement loop and original ideas as hypotheses; do not confuse a coverage
checklist with a requirement to keep every initial implementation choice.

Full productive service is the default heuristic: water for survival/yield,
collect available fertilizer, feed and care when their output can be used.
This is a strong prior, not a rigid constraint and not a claim of an observed
95% frequency. Bounded local optimization can override dated service in fast
estimated play or in full compiled play. Compare each exception with the same
composition under the default, then check estimated and exact gains. Ordinary
service should not add many outer decisions merely to handle rare exceptions.

| Module | Owns | Output and consumers | Initial budget/limits |
| --- | --- | --- | --- |
| Outer search | Dated compositions and branch alternatives | Candidate to estimator/compiler | Cold and warm starts; budget measured after baseline |
| Biology | Dated crop/animal yields and service | Inputs and jobs to economics/compiler | Exact rules; ideal service is explicitly conditional |
| Placement | Tile assignment and replacement | Distances and jobs to labor/compiler | Several cheap greedy orders, then local swaps |
| Economics | Cash, shared market, inputs, trade timing | Value and rejection witnesses | Daily estimate first; refine intraday arrivals |
| Workforce/routes | Hires, unit assignments, routes | Actions and realization gaps | Bounded greedy first; persistent day solver optional |
| Policy | Observed branches and verified repairs | Observation-only action | Deterministic work budget and safe fallback |
| Exact evaluator | Full game, diagnostics, promotion | Results and counterexamples | Paired seats and common seeds |

An edit invalidates biology, inputs, cash, jobs, placement, hires, routes, market
flows, and subsequent state as applicable. Initial implementation rebuilds whole
candidates; caches require explicit equivalent inputs and version keys.

Use C++ typed data and the persistent engine on CPU. There is currently no
demonstrated need for GPU work or a learned surrogate. Add either only if a
measured bottleneck and useful workload justify it. The user questioned the
earlier GPU assumption; do not spend time building GPU machinery speculatively.
