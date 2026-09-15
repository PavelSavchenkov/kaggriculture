# Placement testing

## Replay evidence

`placement_patterns.cpp` replays the original games in the native engine and
records successful establishments. `PLACEMENT_REPLAY_PATTERNS.md` summarizes
opening, early, middle and late placement. Replays supply behavioral clues;
the policy contains no player identity, original route, tile tape or future plan.

## Independent dawn tests

The extractor restores each recorded dawn independently. It retains successful
events, input purchases, and cumulative explicit shed returns with their original
deadlines. It discards worker assignments, movements and new-product sites.
Seed/animal purchases move to hour 0; wheat/land purchase hours are retained.
The solver sees the event-set API, not an ordered replay route. The evaluator
replays every reported success independently and checks all events, receipts,
production, resulting state and hire limits.
New jobs retain their successful creation order in the trace. Placement uses
stable input order for otherwise tied choices, so permutations can change the
chosen layout; input order is not a required execution order.

Days using fertilizer purchases/pickups, unsupported new-product events,
standalone housing, repeated events/generations, multiple land purchases or
invalid contract inputs are excluded. Day 29 has only 23 actions and is excluded.
Exclusion CSVs contain every player-day and its first exclusion reason. Successful
original hires exclude the farmer; original failed hire requests are not counted.
The solver does not constrain the new schedule to the original hire count unless
the benchmark explicitly applies that cap.

## Placement carried from the start

`progression.cpp` carries a shadow grid from the beginning of each game. Each
product generation receives a permanent site chosen by the policy. The harness
maps later jobs to that identity and applies native overnight updates. Newly
appearing weeds remain part of the exact grid. It uses the same job compiler as
the solver, including same-day clear/replant dependencies and planned land.
On excluded prefix days, a new generation cleared again before night has no
surviving identity. Only the final generation is transplanted; the excluded day
does not become a successful worker test.

The original-placement control must match every dawn tile attribute. Changed-grid
cases retain the original eligible denominator; a missing identity or unsupported
prefix mapping counts against coverage, rather than disappearing from the test.

This is a geometry test, not a feasible full-game rollout. The prefix preserves
recorded service outcomes, including excluded days or days the worker policy
cannot solve. It does not propagate solver cash, shed contents or production.
The worker solver may move an unstarted crop during a repair; the prefix harness
uses the initial deterministic placement. Final integration must propagate the
solver's returned state and recompute the next plan from that actual state.

## Return tests and failure categories

- **Strict:** retain every original cumulative return deadline. This tests real
  early-return support, but a changed layout can make an inherited deadline
  physically impossible even with unlimited workers.
- **Hour 23:** retain final return quantities, moving all deadlines to the last
  playable hour. This isolates daily work and transport capacity. It is not an
  early-return result and must not replace the strict table.
- `return_bounds.cpp` gives each producer its own ideal worker, ignores service
  and cargo conflicts, and computes the earliest possible return. A violated
  non-wheat bound proves that those field quantities cannot reach the shed in
  time on that layout. Passing the bound does not prove feasibility. Wheat is
  excluded because purchased wheat can also be returned.
- Other failures remain search/workload failures or unresolved constraints;
  do not automatically classify them as impossible or as bugs.

## Selection and measurement

Placement candidates are selected on the 914 development days. The 639/645
comparison games were already exposed during worker-policy development; their
new layouts are confirmation tests, not unseen opponents. Late means days 20–28.

Use sequential, pinned solver processes. Include failed calls and hire
minimization in median/mean timing. Report original and changed layouts separately.
The two worker profiles run independently; chaining them adds their time.
Ordinary native release and PGO are different builds and must be labeled.

Minimum hires are not proved. Replay-day execution coverage provides evidence
about this component's usefulness; it does not establish full-agent win rate or
a gold-medal result without a decision engine and complete-game evaluation.
