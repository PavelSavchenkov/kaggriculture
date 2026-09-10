# Resume the work

The timed optimization session ended at 2026-09-10 05:00 UTC. Packaging did not promote another strategy. Start with the package check in REPRODUCE.md, then freeze a new experiment protocol before looking at its results.

## Next work, in order

1. **Measure usefulness in plan search.** Supply resource calendars from the plan compiler, keep the proposal set and day solver fixed, and add the retained timing component. Compare complete selected plans against the old selector and strong controls. Report realized margin and feasible useful plans per CPU second. A better score for fixed plans does not establish better plan selection.
2. **Fix rival scenario compatibility.** A historical rival with unrelated workers, crops or money is a stress case, not a continuation of the observed rival. Define a compatible entry-state contract, preserve required inputs, and explain failed forecasts and fallback choices. Do not add credit or drop failed cases to improve coverage.
3. **Revisit purchases only with explicit funded quantities and deadlines.** Keep one-turn seed prebuying, future-restock credit and held-output exchange paused. Their current evidence does not justify more tuning without a stronger hypothesis.

Every 20 minutes during renewed optimization, update LEARNINGS.md, FARM_RECOMMENDATIONS.md, IDEAS.md and PROFILING.md with the result, counterexamples, scope, cost and next decision. Prefer simple rules with a causal explanation. Separate replay patterns, controlled gains and untested recommendations.

## Implementation status

| Code | Status and purpose |
| --- | --- |
| `day_timing.hpp`: `delay_sales_within_day` | Best retained sale-timing component; 1,000 simulated financial turns per decision, shared across edits. |
| `day_timing.hpp`: `delay_within_day` | Simpler one-edit control; still wins in some storage-allocation cases. |
| `rival_stock.hpp`, `rival_delivery.hpp` | Audited public-information features; require consecutive history from known starting stock. |
| `market.hpp`, `calendar.hpp`, `continuation.hpp` | Exact supported financial transitions and resumable continuation evaluation. Not a route/biology certificate. |
| `project_resources.hpp`, `compile_calendar.hpp`, `own_world.hpp` | Adapter from selected worker actions and an estimated fixed course. Future funding is not certified. |
| `purchase_repair.hpp` | Useful deadline repair on the three courses; can change production. Keep distinct from sale-only comparisons. |
| `agents/room_keep` | Conservative direct improvement over the initial agent; contains sale-only compaction and strict storage relief. |
| `sale_priority.hpp`, `agents/sale_priority` | Challenger with a positive overall mean and negative opponent groups. |
| `compact.hpp`, `storage.hpp`, `value_storage.hpp` | Small primitives and continuation-valued storage experiments. Preserve refill/work conditions. |
| `baseline.hpp` | Cold and warm financial controls; longer input reserves did not solve all funding failures. |
| `timing.hpp` | Older one-turn controls plus utilities/memory used by retained timing. |
| `case.hpp` | Replay format and supplied accepted-purchase witness. Future own quantities are declared benchmark inputs. |
| `forecast_library.hpp`, `source/portfolio.cpp` | Scenario valuation and three-course selector; pipeline gain unproved. Historical-cash mode is not promoted. |
| `holding_exchange.hpp`, `agents/history_course_exchange` | Paused: fixes a motivating capacity conflict; broad gain inconclusive, with losses. |
| `seed_prebuy.hpp` | Paused: prebuy loses against its own slot-fill control; slot fill has only exposed replay evidence. |
| `seed_budget.hpp` | Global cap fixes simple leftover seeds, not the funding cascade. Future-restock-credit variant rejected. |
| `immediate_sales.hpp`, `agents/immediate_*` | Rejected controls: changed farm guards and lost heavily. |
| `profile.hpp`, `source/mine_trades.cpp`, reporting scripts | Descriptive opportunity/cost measurements, not proof of optimality. |

Older wrapper names intentionally remain available for ablations. Their presence is not a recommendation to enable them. The generic `history_course` constructor defaults to an older control: use `history_course_multiple` or set the documented flags explicitly.

## Data and unresolved limits

All 297 extracted public episodes are exposed. The 25 reserved episode IDs and acquisition hashes are in data/MANIFEST.json; their actions were not opened during packaging. The default preparation and verification scripts refuse them. Before using them, freeze the candidate, control, metrics and protocol, then record the exposure change. They provide a later-episode test, not a guarantee of independent source families.

The three live cow/sheep/goose courses share one implementation family. Public replay tests keep rival actions fixed and supply funded future own purchase quantities. Neither establishes transfer to every future agent family. The 12-opponent league also includes related packages.

Historical source cash eliminated forecast failures without improving numerical branch rankings. The fresh selector comparison remained inconclusive against the previous selector and demand chooser. Keep this failure visible when designing the next pipeline test.

No tile swap, early-deposit route or composition-staggering change was implemented and promoted. FARM_RECOMMENDATIONS.md contains concrete proposed tests. Preserve strong schedules while changing their financial orders or work requirements.
