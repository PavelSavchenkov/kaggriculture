# Ranked ideas

Initial review, 00:10 UTC:

1. Build a complete-course identity/placement contract and full-game checker. Reuse persistent root extractors and solver APIs; do not build another day solver.
2. Pull current strong-agent replays and inspect placement/service patterns across games. Freeze family reserves first.
3. Establish equal-budget fixed-layout search and simple placement controls on several complete farms.
4. Search consistent lifetime placements using daily labor estimates; exact-compile a small diverse frontier.
5. Test grouping by service days, input sharing, deadlines and reuse; compare against nearest-first.
6. Cache unchanged day contracts and incrementally score changed days; measure complete CPU and RSS.
7. Integrate useful alternatives into actual composition selection and live-game checks.

Deferred until evidence: learned placement proposals, large neighborhood schemes, explicit SIMD/PGO. Do not tune one weak wheat branch or only the existing artificial day-layout corpus.

## Required search families and broader candidates

User clarification: cover rule-based placement, closed-form-based placement and heuristic search, and investigate substantially more than those examples. Compare them on identical contracts and budgets. Classify what is analytical versus what is itself a search; a Hungarian assignment is not a closed-form optimum of real routing.

Rule baselines and patterns:

- Original/source placement, identity-preserving local changes, nearest owned tile and quadrant-first placement.
- Give nearest tiles to the lives with most dated service, carried inputs, output trips or tight deadlines.
- Reserve near-shed tiles for early investments; avoid consuming future crop-rotation slots with permanent animals.
- Cluster equal service calendars, complementary animal/crop input flows and simultaneous harvests.
- Separate overloaded service clusters when sharing one daily worker becomes impossible.
- Reuse permanent animal structures and complete compatible crop-cycle tile chains.
- Borrow whole motifs from distinct high-performing farms; retain provenance and distinguish fixed role from observed conditional choice.
- Use service corridors, short shed-return loops, spatial sectors and balanced daily workload.

Analytical candidates:

- Rearrangement-optimal assignment for a separable weighted shed-distance proxy; evaluate its actual downstream limits.
- Closed-form daily work/travel lower bounds and release/deadline slack pressure.
- Price each day's added work with the local Fibonacci hiring threshold, rather than one global operation cost.
- Pairwise co-service and input-sharing distance formulas; optimize their assignment explicitly when not separable.
- Exact linear assignment on analytical costs, with locked existing entities, ownership times and lifetime overlap constraints.
- Marginal affected-day cost updates and analytical dominance/symmetry reductions.
- Compare workload-weighted distance, deadline-weighted distance, return-trip bounds and estimator-based objectives.

Heuristic search candidates:

- Multi-start greedy construction; relocate, swap and small cycle moves.
- Best/first improvement, deterministic tie alternatives, simulated annealing and bounded tabu.
- Beam search over placement groups; preserve diverse crop/animal/route motifs.
- Large-neighborhood destroy/repair by a difficult day, service cluster, quadrant or lifetime interval.
- Alternate placement and exact workforce improvement, retaining the best complete incumbent.
- Repair first compilation failures by moving only implicated future lives; never move established entities.
- Search linked tile chains versus individual lifetimes after crop removal.
- Recombine compatible groups from good complete layouts; reject biological/occupancy incompatibility before scoring.
- Allocate expensive solver calls by expected gain, uncertainty and remaining gap, with an exploration quota.

Speed and pipeline candidates:

- Incremental cached day features, canonical exact contract keys and reuse of unaffected certificates.
- Batch scoring and bounded parallel exact queries with no nested solver oversubscription.
- Early stop when analytical remaining gain cannot change the selected composition.
- Learn a small proposal ordering model only if simpler analytical/rule/search baselines leave measured headroom.
- Measure whether patterns speed up search even when the final best placement is unchanged.
- Separate cold construction, warm insertion, land expansion and rotation reuse; no claim from only one slice.

Explicit additional requirement: bounded random-weed and money-shortage repair. Prioritize early failure detection and low-cost compatible fixes alongside placement search, then test injected and natural failures through the complete continuation. Keep repair gains separate from changed placement/search gains.
# Cross-agent combinations (user steering, 00:36 UTC)

- Extract broad animal/crop zones, species placement, repeated service groups and quadrant patterns from development leaderboard agents.
- Match donor roles to the recipient's existing dated tasks; copy geometry without importing unrequested production, purchases or future observations.
- Recombine legal quadrant assignments or compatible service groups from different donors. Repair the permutation explicitly after crossover and retain fixed current occupants.
- Run population search with source, radial and donor-derived seeds; combine crossover with local swaps, relocations and later lifetime moves.
- Select using both predicted season bill and exact complete-plan coverage. Preserve diverse certified incumbents and compare with equal-budget local search alone.
- Do not treat teams sharing a common tape as independent parents or evaluation families.

# Quadrant decomposition (user steering, 00:39 UTC)

- Measure workers with productive tasks in multiple quadrants, their crossings and near-shed shared trips. Traversing a quadrant alone is not a cross-quadrant service dependency.
- Compare whole-board moves with block coordinate descent and quadrant populations; keep shared daily workforce and inventory evaluation global.
- Use quadrant summaries as additive screens only when conservative; do not call four separate minimum-workforce bills a valid season bill.
- Cache proposals per quadrant and reuse unchanged block features. Periodically allow cross-block worker service and legal cross-quadrant future-lifetime moves.
- Combine donor quadrants as an evolutionary crossover, then check ownership timing and complete contracts.

# Demand-aware relative placement (user steering, 00:37 UTC)

- Keep both cows and sheep when the composition requires both; change their relative distance using revealed milk/wool demand, forecast sale value and urgent deposit obligations.
- Compare service-count, input/output-trip, sale-value and deadline-weighted variants. Do not assume higher unit price alone implies a better near-shed occupant.
- In a strictly fixed-output, fixed-trade contract, prices do not change the value of an already guaranteed output. Demand can still enter through deposit deadlines, cash headroom and repair priorities; quantify this distinction.
- For broader composition search, pass demand-derived marginal output values from the sales planner rather than make the placement component predict opponents independently.
- Test same-composition cases where milk/wool demand differs, and preserve initial occupied tiles. Only unstarted lifetimes may move when later shops are revealed.
