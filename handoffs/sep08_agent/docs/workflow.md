# The optimization workflow

Paths in this document are relative to the restored
`experiments/v6/sep07_compositions_v0/` unless explicitly rooted at the repository.
The snapshot contains the working implementations and their failed predecessors.
Do new work in a new run folder; many generators intentionally fail if outputs
already exist, so rerunning them over the archive is not a continuation method.

## 1. Inspect real strong behavior and the current gap

Use `scripts/refresh_top_evidence.py` with a new refresh name and the previous
refresh name. It retrieves the current top12, recent games and public notebook
metadata. The latest completed snapshot here is `research/refresh_sep08_1708`:
72 player-games and three changed notebook listings. Those listings are not yet
audited new C++ agents.

`scripts/analyze_replays.py`, `summarize_replays.py`, `extract_compositions.py`
and the research exports recover cash, product flows, crop/animal instances,
service dates, land, workforce, failures, trades and complete daily states.
Check conservation and cash parity first. A requested action may fail, a purchase
may be only partly funded, and a high sale quantity is not production.

Read the complete active graph of promising notebooks. Extract payloads without
blindly executing notebook code. Verify source identity, dependencies, license,
branch conditions and dormant layers. Port useful complete policies to C++, or
make a controlled component ablation. Replays establish realized behavior;
they do not reveal all hidden branches of the original agent.

## 2. Propose a complete dated change

Represent crop/animal lifetimes, counts, tile assignments and dates, plus
structures, land, supply, harvest/deposit/sale obligations and relevant service
exceptions. Include a retain/wait alternative. Start from successful complete
farms, but also test new species combinations, insertion/deletion, earlier
investment, crop rotations, layout changes and independent cold construction.

Useful working entry points:

| Task | Source/run |
| --- | --- |
| Original broad warm/cold search | `src/search_compositions.cpp`, `runs/search_v0_001` |
| Dated biology and cheap economics | `include/biology.hpp`, `estimate.hpp`, `economics.hpp`, `animal_investment_value.hpp` |
| Four-way late composition choice | `runs/late_portfolio_001` |
| Multi-animal groups and earlier entries | `runs/animal_groups_sep08_001`, `animal_group_policy_sep08_001` |
| Flexible entry and displaced crops | `animal_group_policy_sep08_001/source/compile_flexible.cpp` |
| Independent cold crop timing | `runs/early_melon_sep08_001` |

## 3. Estimate before expensive scheduling

Compute dated physical output and input demand, remove displaced crop output,
value the whole portfolio through shared prices, and approximate workforce,
cash, land, inventory capacity and feasible delivery/sale times. Treat planting
already done as sunk cost. Separate fertilizer produced from fertilizer inputs
saved. Account for the last live day precisely.

Use observed shops plus sampled future demand, not real future shops. Include
the existing own farm and a public-state-based rival forecast. Rank by paired
match value or risk-aware margin, not gross output. Estimate a useful lower bound
on labor and flag cash/route uncertainty instead of declaring feasibility from
biological counts alone.

The original integrated estimator ran about45.4 microseconds per profile/scenario
in its measured setup. Speed did not make its initial greedy compiler strong:
583 proposals and36 exact candidates found no improvement. Keep both the timing
and that failure when deciding what to optimize.

## 4. Compile promising compositions

Use persistent `day_solver/` through the restored run CMake project. A fixed-day
contract includes beginning farm and inventories, required crop/animal work,
workers, purchases/sales and deadlines, and the required end state. The solver
returns a strictly replayed24-hour schedule or UNKNOWN at timeout. UNKNOWN is
not a proof of infeasibility.

Compile every affected downstream day. Keep and revalidate both original and
previously found schedules as incumbents. Do not replace a certified cheap day
with a more expensive schedule because a fresh search timed out. This caller
mistake caused a measured regression; root day_solver was correct.

Validate joint market funding as well as physical work. The mixed day9/10 case
demonstrated that a route can try to service an animal that was never bought:
new hires left money for only one of a requested two geese. Moving the second
purchase to hour10 funded it before pickup and preserved the route.

The next compiler improvement is a general post-edit inherited-order funding
repair. It has not been implemented. The existing helper can insert a new
funded order, but does not recheck every inherited order after extra hiring.

## 5. Compare estimates with exact reacting games

Use identical prefixes and forced alternatives. Verify baseline reproduction,
then record estimated/actual output, input spending, labor, sales, own cash,
rival cash, margin and guard failures. Replay the reacting opponent; equal
requested actions do not guarantee equal accepted purchases or future farms.

`runs/mixed_funding_sep08_001/FORECAST_FINDINGS.md` is the latest attribution
example. After correcting displaced active crops and terminal collection, all9
production deltas match. Measured-labor conditional margin is5414 versus33
exact; supplying actual shops reduces it to2660, actual own flows gives2682,
and both players' actual flows gives317. The largest remaining error is the
rival's response. Oracle rows diagnose error; they are not deployable policies.

Use disagreement to fix the correct layer: biology, costing, funding, scheduler,
market timing, runtime repair or rival forecasting. Do not fit a gate to conceal
a compiler error. Measure hindsight headroom: one narrow two-leaf selector had
only4.60 mean-margin headroom, making more selector tuning a poor priority.

## 6. Test a policy, then promote deliberately

Check source parity, active branch fixtures,719 actions, self-play, PASS, debug,
generic/typed equality, independent instances and thread determinism. Run cheap
paired discovery first. Freeze candidate code and gate definitions before fresh
confirmation. Separate independent-shop and native-RNG panels. Include teammate,
public threats, historical versions and distinct cold/specialist controls.

Report every opponent's wins, own/rival cash, margin and worst-decile losses.
Use seed-cluster uncertainty with both seats paired. Explain guard misses and
causal production/trade changes. A local win against the parent alone is not a
league promotion. Read league.md for the exact most recent declared protocol.

## 7. Keep an efficient research loop

Cache binaries by complete source/compiler/flags content. Batch games rather
than invoking an interpreter or compiler per game. Use the generic arena unless
typed specialization has a material measured benefit. Preserve successful day
templates, counterexamples and incumbent schedules. Log genuine solver timeouts
separately from invalid contracts and program errors.

Every20 minutes update the ideas and profiling ledgers, review unfinished jobs,
and decide whether broader alternatives or a module redesign has more value
than another small parameter search. Keep some opponents and scenarios unused
until a final candidate is ready. Do not turn the entire league into training
data and then call its familiar results a final audit.
