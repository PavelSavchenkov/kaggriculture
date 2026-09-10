# Placement contract v0

The requested end state is a reusable, reasonably fast placement optimizer that improves the composition-search pipeline. Start with an exact baseline and preserve broad inputs; supported subsets are milestones, not a substitute for the complete objective.

## Input

`PlacementProblem = (start, period, lives, work, land, resources, incumbent, budget)`

- `start`: actual farm, initial inventories, cash and allowed public context. Initially support dawn starts, matching the root estimator. Existing crops, animals and structures cannot be relocated.
- `period`: next turn through an explicit end, normally turn 719 exclusive. Turn 718 is the last real action.
- `lives`: named crop/animal lifetimes with fixed species, counts, planting/placement dates, end dates and legal tile sets. Lifetimes may share a tile only with compatible ordered clearing/replacement. An animal never moves after placement.
- `work`: required biological actions, their targets, ordered dependencies and timing windows. Production, biological state and service do not change merely to improve placement cost.
- `land`: owned tiles and committed unlock times. Land-purchase choices may be supplied as separate outer alternatives.
- `resources`: dated input releases, output delivery/withdrawal obligations, order slots, allowed hiring menu, cash assumptions, required stocks and continuation endpoints. Nonlabor orders are fixed in the first isolated comparison. Their actual accepted quantities must succeed.
- `incumbent`: optional complete certified assignment and schedule. Preserve it on failed or timed-out searches.
- `budget`: total CPU/wall budget, query limit, solver budget, deterministic search seed and core count.

Input records must distinguish source traces, estimated calendars and certified calendars. A replay's future adaptive choices are a conditional fixed plan, not a runtime prediction. Source family and evaluation outcomes cannot enter deployable scoring.

## Output and objective

Return a legal assignment, complete day contracts, best certified full remaining schedule, exact resource calendar, total hire bill, timing/query counts, and status. A witness must cover every affected later day. UNKNOWN is unresolved, not infeasible. Return the first failed obligation and its day when useful.

For fixed nonlabor orders and mandatory output, minimize the actual season hire bill at a fixed total compute budget. Feasibility is mandatory. Do not score missing work as cheap labor. Track complete-plan coverage separately from bills on common successful cases.

If successful quantities, order times, realized prices and the opponent continuation are all fixed, the nonlabor cash contribution is constant. Then a placement's final-cash difference is exactly its negative hire-cost difference. Milk price alone cannot justify a new distance weight when both placements already guarantee the same milk sale. Demand becomes relevant when delivery deadlines, retained stock, funding risk or a financial policy's chosen trades can change. In that broader comparison, export delivered and discarded quantities and evaluate both calendars with the same financial policy and information rules.

The root day solver remains responsible for routes; the estimator ranks queries without claiming certification. Full engine replay checks cash, ordered purchases/sales, shed capacity, output and later state. A fixed-layout worker optimization is the primary control, so routing improvements are not mislabeled as placement gains.

Integration also evaluates the same frozen financial planner/policy against each candidate calendar, then checks live-opponent games. This tests pipeline usefulness beyond the restricted fixed-order benchmark. A financial module from an active experiment must first be copied as a minimal frozen source closure or published to a persistent root package; never depend on its experiment path.

## Search scope and implementation

Start with consistent whole-course tile permutations and local changes to future lifetimes, then support legal reuse after crop removal. A static permutation is one candidate family, not the final definition of placement. Support crop, animal and mixed plans; warm improvements and cold construction.

Use C++20, contiguous task/calendar data, incremental affected-day features and immutable precomputed distances. Cache by full physical/economic day contract and solver version. Retain every useful certified schedule. Reuse identical days only after checking entry state. Use estimator confidence/unsupported flags and exploration: difficult additions must not disappear under a confident-looking bad estimate.

Use up to four benchmark workers initially, with each solver single-threaded; do not multiply parallelism. Build with two jobs while memory use is measured. Prefer CPU/wall profiles before SIMD, LTO, PGO or heavier models. No fast-math. Public-code payloads are inspected before extracting; no notebook setup/submission cells are executed.

## Main risks to test

Daily coordinate permutations can teleport existing entities. Whole-course plans must preserve physical continuity. Source weeds, source overflow, unsuccessful orders and source-specific cash are not universal obligations. Hire counts and tile occupancy may change live opponents or native random consumption. Report conditional fixed-world evidence separately from policy transfer.

If placement headroom is small across strong controls, or the adapter dominates iteration cost, review the scope with evidence and prioritize the compiler bottleneck needed to make this component useful. Do not silently replace the task with day-only geometry tuning.

## Bounded repair (explicit user addition)

Include repair rules for occasional random weeds and cash shortages. Input is the actual allowed observation, expected continuation and remaining budget. Output is a compatible repaired action/day/suffix or an explicit list of lost obligations. A local runtime rule cannot access the live evaluator, random seed or future state.

Weed candidates: clear before planting/building using idle same-tile visits; route a nearby spare worker and return; reassign an unstarted future life to a compatible empty tile; recompile the smallest affected closure. Never relocate an established plant or animal. Do not rebuild an entire plan for an irrelevant weed.

Cash candidates: identify the first unfunded commitment; use already available non-reserved inventory through the financial planner; split or delay a purchase before its true input deadline; move optional hires within their permitted menu; retain a cheaper compatible schedule. If satisfying the original plan is impossible/unresolved, report the unmet work and separately evaluate an explicitly revised plan. Never let missing purchases turn into repeated phantom service.

Test injected weed/funding faults plus naturally occurring source and fresh-world faults. Compare no repair, simple rules and bounded recompile. Record full-game cash/output recovery, repair latency, remaining-plan validity and new failures on unaffected controls.
