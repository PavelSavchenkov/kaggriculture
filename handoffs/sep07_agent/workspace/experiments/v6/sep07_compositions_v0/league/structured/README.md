# test_structured_economic_policy

Reusable C++ port of the public notebook `Kaggriculture: Structured Economic
Policy`. The economic policy and requested action order are unchanged.

## Provenance

- Notebook: `experiments/v3/aug31_four_random_shops_vs_league/holdout/public_sealed/structured_economic_policy/kaggriculture-structured-economic-policy.ipynb`
- Notebook SHA-256: `2968c446a3c978a0680f99f67b9f0fcb64e1343b9b703d29a41cdc88514ac981`
- Extracted policy-cell SHA-256: `b590bd88e63d28930f223e7b192d606e6224f87efeff30afb2553ff5566426f1`
- Notebook-declared engine: `kaggle-environments==1.32.6`
- Parity-trajectory engine: `kaggle-environments==1.32.7`, matching the local
  game engine.

The notebook was parsed as JSON and its one `%%agentfile` payload was extracted
after an AST-only audit. Notebook cells were not executed. The audited payload
imports only `collections` and `math`.

## Behavior

The policy:

- reserves spatial roles for cattle, sheep, melons, strawberries, wheat, and
  carrots;
- creates urgency/value-ranked field jobs and greedily matches workers to jobs;
- protects feed, crop watering, harvest, fertilizer, and terminal unloading;
- sells by projected proceeds before allocating livestock, wheat, land, seed,
  and labor capital;
- raises the labor cap from 12 to 13 hands on day 20;
- contains a stateful opponent-signature overlay that buys wheat at day 11 hour
  2 and resells excess wheat at hour 19.

The C++ implementation uses fixed arrays, numeric item/operation identifiers,
bounded BFS queues, prefix sorting, and the engine's cached price curve. It does
not allocate heap containers or compare item strings in the decision path.

## Verification

Strict compilation passed with `-Wall -Wextra -Wpedantic -Werror`, exceptions
and RTTI disabled.

Original-Python/C++ parity is 2,882/2,882 exact complete ordered actions:

- seeds 217 and 218, both seats, all 719 decisions per seat: 2,876 actions;
- six stateful fixtures covering signature activation at steps 24, 192, and
  264, the 100-unit wheat request at step 266, and the resale at step 283.

The reached trajectories cover every unit operation requested by this policy
and every market order family it can issue. The original exception fallback was
not reached.

The replay check took 1.676 seconds for the original Python and 0.57 seconds for
C++ on the same 2,882 observations. Both include JSON parsing; the C++ timing
also includes action serialization. This is a fixture-runner comparison, not an
end-to-end league benchmark.

Evidence is in
`experiments/v3/aug31_four_random_shops_vs_league/work/test_structured/`, with
the summary in `parity_report.json` and the strict receipt in
`strict_build.json`.

The parity claim does not cover malformed observations, custom market
parameters, non-10x10 boards, or arbitrary off-trajectory states. Four-shop
strength and execution profiles are recorded in the v3 league package; they do
not extend the source-parity claim.
