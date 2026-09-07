# Objective

For 24 hours, develop the strongest practical agent for the normal 30-day,
eight-shop game. Outer search proposes compositions: product, count, first day,
last day, with continuations selected using observed shops, prices, public
opponent farms, and our own execution state. Unknown future shops or seeds
must never reach the policy.

User clarification: learn from top-player replays and freely borrow useful
components, including placement rules, full day schedules, compositions and
branching. Keep the origin and modification history of each actual component
and idea in `LINEAGE.md`. Distinguish copied behavior from inferred logic.

The user explicitly permits changing this design at any time when evidence
supports a related representation, different module ownership, or more/less
detail. Composition search is a working hypothesis, not an architecture to
preserve at the expense of agent strength. Record the evidence and update the
design, dependency closure, and objective coverage when changing course.

A fast estimator simulates dated biology, greedy placement alternatives,
productive crop and animal service, labor, land, wheat and fertilizer supply,
capital, inventory capacity, purchases, sales, and shared market impact. It may
use approximate routes, day templates, sampled future shops and opponent flows.
Its outputs are heuristic estimates with measured errors, not proven bounds.
Reject impossible requests with a specific witness; do not call a strategy
uneconomic merely because our compiler cannot realize it.

Compile finalists into complete policies. Improve exact schedules, tile
placement and same-day trade timing. Measure estimated versus realized output,
resource use and profit, then use those errors to improve both the estimator
and compiler. Rebuild the full set of affected decisions after every edit.

Maintain a league of strong available teammate agents, public Kaggle C++ agents,
and retained previous versions. Repeatedly propose strategies against league
weaknesses, screen across shop scenarios, and promote only on exact full games.
Pull fresh top-player replays and compare against globally strong normal-game
players, even when their executable agents are unavailable locally.

Search recent public Kaggle notebooks for strong executable policies and useful
components. Port promising agents to C++, verify source parity, and use them in
the league. Audit archived and unofficial repository ports as well as the small
current catalog; a weak subset does not establish that all pulled agents are
weak. Inspect top and near-top replay strategies for direct reuse. Deduplicate
shared route families while retaining useful differences and source lineage.

## Session protocol

1. Scenario: default official game with random weeds and all eight random shop
   reveals. Main objective: average win utility (win 1, tie 0.5) with equal
   opponent-group weighting; report cash margin and lower-tail margin separately.
   PASS diagnostics report J = 0.8 mean cash + 0.2 lower-CVaR10 cash.
2. Outer decisions: dated product counts, lifecycle replacements and adaptive
   suffixes. Keep unresolved economic structure in search. Delegate exact tile,
   service, route and order choices only where a module earns that ownership.
3. Start with a deterministic biology/market estimator and a bounded greedy
   compiler with repair. Add placement alternatives and use the persistent day
   solver for difficult finalists if measured gains justify its cost. Explicit
   gaps include intraday cash, capacity, route feasibility and opponent response.
4. Combine cold construction, warm refinement, family insertion/deletion and
   larger rebuilds. Start with both crop-heavy and mixed animal proposals.
5. Funnel: cheap estimate; exact discovery games; common-seed paired promotion;
   fresh audit. Initial discovery seeds 1000+, promotion seeds 100000+, final
   audit seeds 900000+. Both seats, complete 719 transitions. Never tune on the
   final audit. Expand coverage as measured throughput allows.
6. Archive by realized behavior, preserving distinct product families, openings,
   resource regimes and conditional branches. Reserve search work for cold
   starts and substantial changes; tuning one incumbent is insufficient.
7. Report requested/realized milestones, animal-days and service rates, crop
   occupancy/yield, gross buys/sells, net flow, trade timing, labor, land,
   fertilization, weeds, failed actions, discards, terminal residue, compiler
   misses, estimator error and timing. Gross turnover is not production.
8. Change representation or module ownership when repeated counterexamples
   show useful families cannot be expressed, feasible requests cannot be
   compiled, or estimates rank them incorrectly. Require causal common-seed
   ablations for accepted changes.

Review all progress every 20 minutes. Reread the original user wording and
update `OBJECTIVE_COVERAGE.md` so no idea is silently dropped. Include deep invariant comparisons with
fresh globally strong external replays and decide whether to reorder ideas or
pivot. Keep the best validated complete agent and reproducible improvement loop
at the end. Do not submit to Kaggle without an explicit submission request.
