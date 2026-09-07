# Review 1 — 2026-09-07 01:07 UTC

Reviewed the original objective again. The working goal remains active with the
original 2026-09-08 00:47 UTC deadline. All four user clarifications are recorded:
CPU default unless GPU has a demonstrated use; borrow useful external/replay
components with lineage; full productive service as a default with local
exceptions; change architecture and fidelity whenever evidence justifies it.

## Completed evidence

- Fresh official leaderboard snapshot: 2026-09-07 00:48:32. Top 12 selected by
  normal full-game leaderboard performance, independently of local execution.
- Six recent episodes per selected team: 56 unique replays, 72 player-games.
  Reconstructed every selected player's cash transition exactly. JSON's sorted
  inventory keys caused a $22 overflow discrepancy; recovering insertion order
  from prior actions fixed it. The failed run and mismatch witness are retained.
- Extracted 16,328 crop lifecycles, 1,188 animal lifecycles and 57,541 market
  orders with actual successful quantities and values. Exported per-game/team
  invariants, detailed service, placement, transitions and timing summaries.
- C++ local adapter for teammate `shoprouter-rl-v2`; copied original production
  tapes and translated its market head. Source notices and hashes preserved.
  Python-reference action parity is still pending, so do not call the port
  proven identical to the teammate source yet.
- Generic arena and a debug specialized pair runner built. Full PASS and
  self-play: 16 games each, both seats. Unit-count/metadata validation passed;
  the debug run also checks engine entity masks. All 16 PASS rows—including
  both action hashes, cash, output, faults and discards—match between generic
  four-thread release and single-thread debug pair. Independent agent state is
  per instance; decoded source tapes are immutable shared data.
- Source inspection: the adapter reconstructs state from the official local
  observation API, leaving opponent-private inventory and seed absent. A paired
  hidden-state/non-anticipativity adversarial test is still pending.

## Global invariant audit

Scope: all 72 current top-player games, not only winners. Means are descriptive
targets; these games have different shops/opponents from local tests.

- Animal-days: 13,397 cows, 11,507 sheep, 2,575 geese across the cohort.
  Feed+care shares over all lifecycle days: 76.8%, 79.5%, 80.3%; fertilizer
  collection shares: 92.7%, 89.5%, 84.9%. Terminal, newly placed and intentionally
  skipped service days are included. The user correctly clarified that these
  exceptions do not invalidate full productive service as the default.
- Sheep disappear before terminal in 30.8% of observed sheep lifecycles; cows
  in 10.5%, geese in 1.6%. Inspect prices, care banks and remaining production
  before inferring an economic rule or intentional escape from an observation.
- Crop occupancy: 1,517.7 crop-days/game. Harvest/game: wheat 504.1, carrot 99.3,
  tomato 31.3, strawberry 237.2, melon 76.1. Common completed ages: wheat 4,
  carrot 3, melon 10, tomato 12, strawberry 17. Ongoing crop removal is separate
  from its harvest dates.
- Crops watered on 72.2% of observed crop-days; water+active fertilizer on 34.9%
  of yield-relevant crop-days. These metrics include age-zero, dying and
  terminal crops. Fertilization should be valued by incremental output and
  market value, not assumed universally profitable.
- Gross product buys/game: wheat 326.3 and fertilizer 57.4. Net sales/game:
  wheat 184.3, carrot 98.0, tomato 31.0, strawberry 236.5, melon 76.0, egg 51.6,
  milk 198.4, wool 158.1, fertilizer 239.4. Gross wheat sales substantially
  exceed net wheat production sold. Do not reward buy/sell turnover as output.
- Labor: 286.3 successful hires/game, cost $6,122.8. Land: 2.014 extra quadrant
  purchases/game. Dated animal milestones and each team's labor/land values are
  in `invariant_games.csv` and `invariant_teams.csv`.
- Timing: exact successful trade hours, amounts and shop demand are recorded;
  the report separates buys and sales. Their relation to production-ready and
  deposit deadlines remains an estimator/compiler task. Animal placement and
  crop-to-animal tile transitions are available for layout rules.
- Shed: 97.2% of games reach at least 90 items, but only 0.91% of observed
  turns have 90+. Mean discarded units 7.81/game. Capacity pressure is brief
  and should be modeled around arrivals, deposits and transactions.
- Weeds and unsuccessful unit actions: lifecycle outcome rows and successful
  unit-event rows are available. A full per-team failed-action and weed-cost
  attribution is not yet implemented; do not substitute requested-action
  counts for actual service. This remains an explicit diagnostic gap.

## Local baseline and causal scope

The adopted teammate adapter averages $162,562.5 versus PASS across 16 games,
with PASS J about $152,449. Self-play averages $72,772.9 with zero mean margin.
Both are smoke cohorts, not promotion evidence. Mean output in self-play:
wheat 577.2, carrot 9, tomato 0, strawberry 258, melon 72, egg 0, milk 261,
wool 195, fertilizer 380. The field-wide global targets suggest carrot/tomato/
goose and adaptive composition coverage deserve investigation; absolute cash
from different opponent scenarios is not directly comparable.

On seeds 1000–1127 and both seats, with independently sampled shop streams:

| Opponent | Games | Teammate wins | Mean cash margin | Lower-CVaR10 margin |
| --- | ---: | ---: | ---: | ---: |
| indar | 256 | 256 | $22,137.9 | $9,130.7 |
| kaito | 256 | 256 | $36,730.4 | $23,159.7 |
| boatlee | 256 | 256 | $24,008.1 | $15,707.7 |

No strategy improvement has been claimed or promoted. These are baseline
screening results. Main scenario uses exact official transitions with shop
types drawn from an independent stream and exposed only on official reveal
days, preventing policy-dependent weed RNG consumption from changing the
common shop scenario. Native official-RNG audit mode is also implemented.

## Decision and next priorities

1. The older C++ ports are too weak to be the only improvement signal. Keep
   them for regressions; prioritize beating the teammate adapter and current
   top-player reconstructed strategies. Verify source parity of the teammate.
2. Extract a typed composition library and reusable complete day schedules from
   current replays. Preserve exact provenance. Use these warm starts immediately
   instead of waiting for a complete cold compiler before testing ideas.
3. Implement the fast default-service biology/economics estimator, compare it
   with recorded and exact compiled output, and expose marginal nothing/cow/
   sheep comparisons. Keep placement, capital, capacity and intraday timing
   errors separate from economic value.
4. Implement exact compilation/repair and a real improvement loop after the
   first estimator calibration. Continue cold construction/family replacement
   work so borrowed schedules do not define the reachable strategy frontier.
5. No GPU work: current 256-game C++ batches take 0.084–0.986 seconds across
   four threads. A GPU path has no demonstrated purpose.

Next full review: 01:27 UTC. Reread original wording and check every coverage row.
