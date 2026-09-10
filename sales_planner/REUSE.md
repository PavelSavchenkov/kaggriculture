# Using the financial components

The retained timing component is `delay_sales_within_day`: several sale edits under the original deterministic budget. On three supplied cow/sheep/goose courses it adds $574.93 average full-game margin over purchase repair, or $41.55 over one-edit delivery timing, in 9,216 fresh paired games. There are no losses against repair and nine small losses against one-edit timing; checked farming/resources match. Its full added policy cost is about 0.96 ms/game. Keep the simpler one-edit rule as a control. [RESULTS.md](RESULTS.md) separates these component gains from the direct improvement over the original agent and the still-unproved plan-selection result.

## What the caller supplies

For the financial search, supply:

1. Current money, market inventory, revealed shops and our resources after this turn's worker actions.
2. A resource calendar: dated production, withdrawals, uses, deposits, seed uses and paid commitments. It includes overflow behavior and the order of events within each turn.
3. A starting market-order plan. Accepted edits update this plan, including orders promised for later turns.
4. A conservative statement of when the rival could first sell each output product.
5. A deterministic work budget.

The financial search does not need our tile placement or worker routes. A compiler or schedule evaluator provides their resource effects. In a live agent, `project_resources` converts the already selected current worker actions into the post-worker account required by the financial call. This is an adapter, not another choice of worker actions by the sales planner.

An estimated calendar must be labelled as estimated. The three-course example compiles one at turn 226 using a fixed planning seed and nominal funding; it does not know the evaluation seed. Complete live games check the actual work and purchases afterward.

The current C++ arrays use absolute turn indices. `calendar[t]` and `plan[t]` describe game turn `t`; ending the vector at index `end - 1` evaluates through that turn. `PlannerObservation` is after our current worker/resource events and before market orders. Do not apply those current own events twice. The financial function receives their resulting account and buffers; it does not need the original worker actions.

## Public information about competing sales

`RivalStockHistory` in `include/rival_stock.hpp` maintains an upper bound on held rival output. It uses consecutive legal observations from the standard empty-inventory start, public harvest transitions, market inventory and our own accepted sales. It does not read rival-private stock. An ambiguous observation increases uncertainty; it does not invent certainty that the rival has nothing left.

`rival_sale_not_before` in `include/rival_delivery.hpp` adds public worker positions and ready output. For each nonbuyable output product:

- Possible held stock means the rival might sell now.
- Otherwise, an existing worker needs travel to a ready tile, a harvest and delivery to the shed.
- A newly hired worker may act next turn. Include its optimistic shortest route too.
- Stop the bound at the next day, when automatic deposits and new production may change the situation.

For a ready tile, let `s` be its distance to the nearest shed-access tile and `d` the smallest distance from an existing rival worker. An existing worker cannot sell before `turn + d + s + 1`. A new hire cannot sell before `turn + 2*s + 2`. Take the earlier bound across all ready tiles and the next day. These are lower bounds on the rival's earliest sale, not predictions of its chosen route.

The bound passed an audit over 47,989 actual sales in 166 exposed episodes. That audit supports the calculation; separate policy tests measure whether using it helps.

## What the sale rule changes

`delay_within_day` in `include/day_timing.hpp` moves one current output sale to a later turn after known demand, strictly before the rival could sell and before the next day. It evaluates at most 1,000 financial turns per call by default. This is a computation limit; the time window follows the game rules.

Each candidate starts from the existing order plan. It must preserve intervening work resources and all private stock and shared market inventory at the chosen sale target, and predict a positive cash difference. A successful call changes only the current and selected future order lists. Later calls retain those commitments. An exhausted budget keeps the existing plan.

The API agent example is `agents/history_course_delivery`. `agents/history_course_day` and `agents/history_course_floor` remain separate controls. Generic and typed debug runners, full self-play and zero-budget checks are recorded under `runs/history_course_checks_v0`.

## Why the whole continuation still matters

Equal state at a sale target does not guarantee equal state for the rest of a policy. More cash can make a previously unsuccessful purchase succeed. A concrete milk delay earns $42, then enables two unused $10 seed purchases: final gain $22 and two extra seeds. In another plan, an extra seed purchase uses cash needed for wheat feed and changes later farming. Keep these exceptions visible when integrating the component.

Price-floor effects require the shared market too. A wool delay earned $70 in its short window but left three extra market units; the next batch lost $260. The retained rule's ending-market check rejects that change. Matching only our shed and workers would miss it.

Thus complete marginal economics means comparing the whole affected continuation: changed revenue, purchases, funding, storage, work and later shared prices. The short search proposes an edit; complete scenario and game evaluation establishes its realized value.

`SeedBudget` in `include/seed_budget.hpp` is a separate purchase experiment. Its global form caps requests by all remaining seed uses plus the supplied ending seed requirement. It fixes simple leftover-seed exceptions, with only $40 total gain across 394 exposed plans; it does not fix the earlier funding cascade. A variant credited supplied later restocks, but failed a full-game action contract and changed farming. That variant is rejected: a future request is not a funded delivery. The global cap remains a small endpoint tool, not a broadly promoted purchase policy.

## What remains open

Use an explicit desired input quantity and delivery requirement when proposing a farm plan. A historical unsuccessful purchase request is not automatically such a requirement. Preserve required input sources and full work feasibility when changing purchases.

The general scenario evaluator is `evaluate_continuation` in `include/continuation.hpp`. Its historical rival calendars can be incompatible with a current rival farm or cash balance. Missing inputs must remain reported; do not manufacture credit or treat resulting phantom production as a feasible opponent continuation. Better scenario construction and improved plan-choice results are still needed.

`ContinuationResult::feasible()` checks recorded resource deficits and successful hire/land commitments. It does not certify biology, routes, every ending requirement or public-rival compatibility. The caller must check those requirements and distinguish a failed supplied witness from proof that no feasible schedule exists.

For promotion, compare the same supplied plans and starting states, retain every failed or changed case, and report mean margin, win utility, worst-decile margin, resource/production differences and complete compute cost. Keep source-family and seed dependence in the uncertainty estimates. A new strong public program is a reason to test again, not to discard the prior controls.

## Exact quantities in a supplied funded schedule

For a conditional fixed-plan test, purchases can be supplied as the quantities actually executed by a funded schedule, retaining their dates and order positions. `CaseTurn::purchase_witness` records that normalization. Failed requests become empty slots; sales remain the starting sale plan. This does not optimize purchases or infer unknown future funding.

On 197 exposed episodes, normalization alone preserves every checked state and cash balance at all 283,286 turn comparisons. With delivery timing enabled, 394 plans gain $255.91 mean margin over original orders, with no losses or checked noncash changes. All three unnormalized exceptions disappear. See `runs/purchase_witness_screen_v0`. These are supplied historical plans; live callers must provide their own funded quantities and verify them under each scenario. The existing live course compiler uses nominal funding and is not a source of certified future purchases.

## Several sale edits under the same budget

`delay_sales_within_day` repeats the existing edit while the original 1,000 financial-turn budget has work left. It removes the one-edit restriction without changing the day boundary or any funding, storage or rival-delivery condition. The live example is `agents/history_course_multiple`.

Fresh live confirmation adds $41.55 mean margin over one-edit delivery timing on 9,216 paired games (95% interval $37.75–45.67), with nine losses, worst $44, and no checked noncash changes. Keep the one-edit control; more edits are not uniformly better. Full policy overhead versus paired purchase repair is about 0.96 ms/game. See `runs/history_course_multiple_confirmation_v0` and its prediction-error audit.

`improve_sale_holding` in `include/holding_exchange.hpp` is a paused experiment. It can sell previously held output now to permit a better current delay, pricing both changes together with the remaining budget. A traced tomato/wool capacity conflict improves, but broad public and live screens are inconclusive and include losses. Do not treat this prototype as an optimal allocation of shed space.

## Historical stress cases and current-rival forecasts

`RivalCashSource::historical_case` keeps the source rival's recorded starting cash with its historical resource calendar. The existing default instead uses observed current rival cash. The explicit source-cash mode removes tested financial failures but is not promoted: the fresh 3,072-choice comparison does not establish improvement over the default selector or demand chooser. All 425 changed choices avoid the old fallback; numeric rankings are unchanged.

Neither cash choice establishes public-farm compatibility. At turn 226, only 372 of 1,280 audited current rival states have any borrowed source matching their full noncash public farm; none also matches cash. A caller must distinguish a historical stress case from a continuation conditioned on the observed farm. Keep missing inputs, unmatched public states and fallback use visible in plan-value reports. Evidence is in `runs/scenario_cash_confirmation_v0` and `runs/scenario_public_compatibility_extended_v0`.

## Ending a period and resuming it

`ContinuationResult` returns both accounts and resource buffers, shared `inventory`, `shops`, `n_shops` and `end_turn`. The state is after the supplied period and before `end_turn`'s shop arrivals and work. This preserves the information needed to value later market effects. Cash and private resources alone are insufficient, as the wool price-floor example shows.

To continue the same supplied scenario, reveal any scenario shop arriving exactly at `end_turn`, apply our pre-market resource events for that turn once, and build the next `PlannerObservation` from the result. Keep the rival account and buffers before its current work; the evaluator applies those events. Arrays remain indexed by absolute game turn. Add prefix and suffix commitment-error counts; accumulated resource deficits already travel with the buffers.

The check in `source/check_continuation_resume.cpp` verifies 111,672 split/resume outcomes across 297 public episodes and compares 55,836 prefix cash/market states with recorded states. It covers both seats, original and compacted orders, day/shop boundaries and interior cuts. This is an API correctness result, not an additional strategy gain. Exact commands and cost measurements are in `runs/continuation_resume_v0`; [REPRODUCE.md](REPRODUCE.md) lists the reusable run commands.
