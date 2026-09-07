# Strategy Search Guide

## Purpose

This document defines the shared language and permanent requirements for
Kaggriculture strategy-search work. Every strategy-search session should use
these terms and preserve these requirements unless this guide is explicitly
revised from new evidence.

This guide does not choose a concrete strategy schema, parameter count, solver,
objective formula, or compute budget. Those are replaceable designs. The guide
defines what any such design must accomplish.

## Framework model

The relationship below is established even though the concrete outer schema,
module boundaries, and algorithms remain replaceable:

```text
rules + scenario + objective
              |
              v
search proposes outer strategies
  - searched parameters
  - optional contracts
              |
              v
compiler coordinates planning modules
  - lifecycle and biology solutions
  - capital and market schedules
  - placement, workforce, and route solutions
              |
              v
complete plan candidates
  - one fixed course for each assumed history or continuation
              |
              v
policy
  - observes the real game
  - follows or selects plan continuations
  - applies verified repairs
  - emits actions
              |
              +----> canonical behavior forms the phenotype and archive key
              |
              v
exact replay on scenario realizations
              |
              v
results and diagnostics feed search and framework redesign
```

Every edit invalidates and rebuilds its dependency closure across these layers.
A fixed scenario may need only one plan and a thin policy that follows it. A
normal game needs an observation-driven policy because shops, weeds, market
state, and opponent behavior are revealed during play. A 719-action tape is one
possible fixed-plan artifact, not the universal definition of a strategy or
policy.

## Shared vocabulary

Use these terms consistently.

### Rules

The exact game mechanics and configuration. Rules are inputs, not searched
parameters.

### Scenario

The complete external environment, or distribution of environments, under
which a strategy is planned or evaluated. A scenario states which gameplay is
enabled and gives known facts or distributions for shops, weeds, market state,
opponents, and other external events.

An artificially narrowed game, such as no shops and a `PASS` opponent, is a
scenario. The fully enabled official game is also a scenario class, but it is
not fully specified for local evaluation until the opponent distribution and
other unknown external behavior are stated. One concrete seed, shop sequence,
weed pattern, and opponent episode is a scenario realization.

The term is needed because the rules alone do not define the optimization
problem. The best strategy can change when shops, weeds, opponents, or allowed
information change. Naming the scenario prevents results from one specialist
or curriculum environment from being treated as evidence about another or
about the full official game.

Observed scenario facts become inputs during execution. Unknown future facts
must not be encoded as if they were already known.

### Objective

The declared quantity being optimized, together with its scenario and opponent
distribution. Examples include fixed-scenario terminal cash, a risk-aware cash
score, cash margin, win utility, or league performance.

Two concrete examples:

- In the no-shop, no-weed, `PASS`-opponent scenario, maximize our terminal
  cash.
- In the official-game scenario against a declared opponent league, maximize
  expected win utility while reporting lower-tail cash margin separately.

Results from different objectives are not directly comparable without an
explicit translation.

### Outer strategy

The compact, typed object searched by the global search. It contains decisions
that have not been delegated to bounded subproblem solvers because we cannot
yet choose them competitively, reliably, and cheaply enough that way.

The outer strategy expresses economic or structural intent. It should not
contain execution details merely because current implementation happens to
emit them.

### Plan

A complete fixed course of action for one assumed sequence of events. It
contains or references every choice needed to emit a fixed action sequence,
including exact timing, locations, assignments, purchases, and sales. It may
remain a typed schedule with semantic provenance rather than a raw 719-action
tape.

For example, one plan may plant 12 specific melon tiles during days 0--1,
perform their scheduled watering, harvest them around day 10, reuse those tiles
for wheat, and sell each harvest after deposit. Another may build and place four
cows on chosen tiles by day 3, buy and reserve their dated feed, assign their
feed, care, harvest, and deposit work, and liquidate the milk before game end.
These prose summaries omit execution detail and do not define a required plan
format.

### Contract

A typed semantic commitment or constraint that may be part of the outer
strategy. Examples include "place four cows during days 2--3," "use 12 tiles
for wheat and then strawberries," and "reserve two feed-days of wheat."

Contracts state what should be achieved, not the raw actions. The compiler and
planning modules expand them, together with other outer-strategy information,
into plan contents such as dated jobs, purchases, placements, dependencies,
and sales. The plan should report unmet contracts and preserve contract
provenance where it helps dependency-closed rebuilding.

### Policy

The executable branching logic that maps legal observation history to actions.
A policy selects, adapts, or repairs plans according to what actually happens,
such as a shop reveal, opponent action, weed, or failed precondition. In a fully
fixed scenario it may simply follow one plan; in a dynamic game it chooses
between plan continuations and applies verified repairs.

### Planning module

A bounded solver for one owned subproblem, such as biology, lifecycle choice,
capital, inputs, placement, routing, workforce, inventory, or market timing.
A module may use an exact algorithm or a heuristic algorithm. It produces typed
subproblem solutions, such as a lifecycle proposal, layout solution, market
schedule, task graph, or route solution. The compiler combines compatible
solutions into complete plans.

The framework should be implemented in C++ by default, including planning,
compilation, search, evaluation, and deployable policy code. A different
language may be used only when there is a well-articulated reason, together
with a documented compatibility and performance boundary. Core components
must use direct typed interfaces compatible with the game engine. They must be
engineered and profiled to be fast enough for their assigned place in the
search funnel. Optimization should target measured end-to-end cost, including
state construction, candidate generation, scoring, and validation, rather than
only an isolated inner kernel.

Except for a basic subproblem with a valid proof, a planning module must not be
treated as truly optimal. It is a practical approximation that should be
continually pushed toward better decision quality and greater usefulness to
global strategy optimization as new evidence, counterexamples, and stronger
methods become available.

Calling a module deterministic means that fixed inputs, versions, and node or
work budgets produce reproducible results. It does not mean that the module is
globally optimal.

### Compiler

The coordinator that uses planning modules to turn an outer strategy into
complete plan candidates and packages plan continuations and repairs into an
executable policy. It owns module dependencies, feedback, invalidation, and
final legality checks.

### Phenotype

The canonical behavior produced after compilation, including complete plans
and policy branching. Two different outer representations that compile to
equivalent behavior are phenotype duplicates even if their parameter values
differ.

### Dependency closure

All decisions and artifacts whose correctness or value can change after an
edit. A dependency-closed edit rebuilds or revalidates this whole set rather
than leaving stale purchases, routes, sales, or policy state behind.

For example, changing "place three cows by day 3" to four cows also affects the
animal purchase, pasture and tile assignment, pickup and placement, wheat
supply, feed and care jobs, workers and routes, milk and fertilizer arrivals,
shed capacity, sales, prices, and later cash. A dependency-closed edit rebuilds
those consequences or proves each reused result is still valid; it does not
only insert one cow purchase and placement.

### Exact replay

Execution by the exact game engine. Exact replay proves the behavior and score
of that policy in that replayed scenario. It does not prove global optimality.

### Cold start, warm start, and basin escape

- A cold start constructs a strategy from rules, scenario, and objective
  without requiring an incumbent policy.
- A warm start uses an existing strategy or phenotype as optional prior work.
- A basin escape destroys and rebuilds a materially coupled part of a strategy
  instead of making only small local changes.

## Permanent requirements

### 1. Cover the known strategy and optimization frontier

The framework must represent every materially different, evidence-backed strong
strategy family currently known: specialists and mixed farms, exact-zero
products, different timing, capital, inputs, land, labor, inventory, and
adaptive branches. Every known useful optimization must be reachable somewhere,
including crop and animal service, placement, workers and routes, input and sale
timing, capacity, terminal closure, and recovery.

These behaviors may come from outer edits, module alternatives, typed plan
transforms, dependency-closed neighborhood moves, or verified repairs; they do
not all need outer parameters. The framework has a gap when known useful
behavior still requires manual raw-action surgery.

Expressivity audits must construct from an empty farm, insert and remove product
families, replace a complete economic backbone, cover materially different
resource regimes, and compile fixed and adaptive families into legal behavior.
New missing families require a semantic outer decision or module capability,
not many raw-action genes. Replays are evidence and seeds, not the frontier.

### 2. Produce dynamic, observable policies

The framework must support strategies that adapt to shops, opponents, prices,
weeds, and own execution state. Policies must obey non-anticipativity: equal
observation histories produce equal actions until a new fact is observed.
Scenario facts belong in state, not in strategy parameters as privileged future
knowledge. Conditional changes must rebuild their semantic dependency closure.

### 3. Separate outer and planned decisions by evidence

The framework should minimize the effective outer search dimension without
hiding unresolved choices in weak modules. A decision remains outer until a
module can choose it competitively, reliably, and cheaply across relevant
basins. A module earns ownership by working without an inherited tape, handling
family insertion and replacement, realizing requests under exact replay,
returning legal outputs or failure witnesses, exposing important alternatives,
and demonstrating adequate quality and speed. Ownership is reversible.

Modules are improvable approximations unless optimality is proven for their
exact scope. Typed module interfaces must report ownership, objective, bounds,
budgets, constraints, feasibility, truncation, performance, residual gaps, and
invalidated consumers. Important structural alternatives require a diverse
portfolio or a way to request one; one opaque greedy result must not define the
global frontier.

Compactness means active semantic levers, not array length. Remove or redesign
inactive, shadowed, equivalent, and frequently compiled-away fields, but never
trade away expressivity or faithful realization merely to reduce dimension.

### 4. Preserve dependencies and diagnose failure correctly

Every decision must have one named owner and explicit downstream consumers.
Edits invalidate their dependency closure; reuse requires equivalent relevant
state and obligations. Distinguish economic rejection, proven or witnessed
infeasibility, compiler realization failure, estimator error, and runtime policy
failure. Compiler failure is not evidence against the requested strategy, and
repairs must never silently change it.

### 5. Estimate value before full compilation

The framework must cheaply estimate whether a high-level candidate deserves
deeper work. At suitable fidelity it must cover dated biology, endogenous market
impact, external demand or opponents, capital, inputs, land, labor, inventory,
orders, terminal value, and exposed compiler gaps.

Keep distinct: certified upper bounds with stated valid relaxations; feasible
lower bounds from legal compiled policies; heuristic expected estimates; and
named stochastic, estimator, route, and compiler uncertainty. Optimism alone
does not create an upper bound, and an interval is not a confidence interval
without calibration. Prices are derived from own and external market flows,
not normally fixed inputs.

### 6. Support complementary search modes

The same framework must support cold construction, adaptation to a new
objective, local incumbent improvement, complete family insertion or deletion,
and dependency-closed destroy-and-rebuild at several scales. Preserve a diverse
phenotype archive and reserve evaluation budget for cold starts and large basin
changes. Deduplicate by canonical phenotype where practical.

Semantic locality is desirable but must be measured after compilation; real
cash, hire, land, route, capacity, and irreversible-investment discontinuities
remain. Smooth search is for exploitation, not global discovery. Mutation and
crossover must exchange or rebuild semantic closures, not arbitrary action
ranges.

### 7. Promote only through exact evaluation

Estimates and relaxed solvers may rank, prune, and propose. A policy is promoted
only from complete exact evaluation under the declared objective and coverage.
Use common scenarios or seeds for paired comparisons and separate discovery
from promotion or audit data where overfitting is possible. Report objective,
coverage, outcome distribution, failures, discards, aborts, terminal residue,
and relevant policy activations. Incomplete or stale evaluations are
non-results. Every claim must state scope and evidence; exact replay proves only
the evaluated policy on the evaluated cases.

### 8. Keep the framework introspective and evidence-driven

Compiles and evaluations must expose requested versus realized behavior, dated
economics, biology and terminal residue, land and tile use, work and routes,
workers, inventory and orders, binding constraints, repairs, estimator gaps,
and inactive or shadowed outer fields where relevant.

New evidence may reshape the outer representation, module ownership,
neighborhoods, estimators, compiler, archives, and evaluation league. Record
accepted lessons as shared tests, counterexamples, and design updates rather
than isolated manual knowledge.

## Deliberately unsettled choices

This guide does not establish any of the following as ground truth:

- whether the outer strategy uses production contracts, economic output intent,
  another typed representation, or a staged combination;
- a maximum or target parameter count;
- a fixed number of phases, contracts, crop blocks, or animal cohorts;
- which specific decisions currently belong to the outer strategy;
- a particular optimization algorithm for any planning module;
- a fixed portfolio size, mutation distribution, or archive implementation;
- a universal cash, risk, margin, or league objective;
- a universal time, CPU, RAM, or patience budget;
- a claim that local smoothness, a compiler, or an estimator is already good
  enough without measured evidence.

Concrete experiment designs may choose these values. They must identify them as
current hypotheses or implementation decisions and remain compatible with the
permanent requirements above.

## Storing framework designs

Store each materially different framework in a folder whose name starts with
its creation date, such as `strategy_designs/aug29_<design>/`. Start a new
design when the outer representation, module ownership, core interfaces,
search architecture, or evaluation funnel changes substantially. Bug fixes,
parameter tuning, and individual evaluations remain in the current design or
in `experiments/`.

Each design must be independently understandable and buildable. It must not
import code or artifacts from another design or from an experiment; copy and
own anything adopted from them. Stable repository resources such as the game
engine, rules, and shared typed interfaces may be dependencies when listed
explicitly. Keep the design's documentation, C++ implementation, tests,
examples, known gaps, and evidence summary inside its folder.

For a substantive framework, use `README.md` as a short index, not the whole
design. Separately specify module ownership and objectives, typed interfaces,
exact and heuristic algorithm portfolios, dependency invalidation, search and
evaluation, acceptance tests, and implementation order. Add implementation and
fixtures as they exist. A single long README is sufficient only for a genuinely
small design with no meaningful module split.

## Session checklist

Before starting strategy-search work, state:

1. The scenario, objective, allowed observations, and evaluation coverage.
2. The outer strategy representation being used and why each searched decision
   remains outside a planning module.
3. The modules involved, their ownership, budgets, approximations, and known
   gaps.
4. Whether the work is cold construction, warm adaptation, local improvement,
   basin escape, compiler improvement, or evaluation.
5. The estimate and exact-promotion funnel.
6. The phenotype diversity and local-maximum escape plan.
7. The diagnostics that will distinguish economics from compilation failure.
8. The evidence required to change representation or module ownership.
