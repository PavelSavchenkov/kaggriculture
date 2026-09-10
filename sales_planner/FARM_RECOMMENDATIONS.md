# Farm changes that could improve sales

Keep this list alongside LEARNINGS.md. A recommendation is not an adopted change. Preserve the current strong schedules and compare each proposed change against them in complete games.

Our retained agents are one source of evidence, not the boundary of this work. Look for the same patterns in public agents, new program versions, different compositions and alternative delivery schedules. Rank ideas by the conditions they address rather than by the current leading agent. For each claim, record whether it is a game rule, an observed pattern in one family, a counterfactual improvement, or an improvement that transfers to other families and scenarios. Include failures and exceptions. Recheck the findings when a strong new public agent changes the set of plausible farms or opponents.

A farm gives the sales planner more freedom when goods reach the shed in time, cash can cover the next required work, storage can hold the goods if waiting is better, and market slots are available. A flexible farm is not necessarily a profitable farm: its products can still face weak demand or competing sales.

## Current evidence — September 10, 04:55 UTC

| Condition | What the tests support | Limit |
| --- | --- | --- |
| Output is delivered; funding and capacity permit waiting before the rival's earliest sale | Delivery-bound timing confirms a useful gain across live courses and public plans. Multiple-sale timing adds $41.55 over one-edit timing on 9,216 fresh live comparisons. | Nine losses versus one-edit timing; no checked noncash changes. This does not rank compositions. |
| Purchases specify funded quantities and dates | The normalized 394-plan development test removes all three state exceptions; 68 fresh public plans gain $270.81 over original orders with unchanged checked state. | Quantities are supplied plan data; purchase timing is not optimized. |
| Low-value held stock competes with a later valuable batch | Tomato holds blocked a $71 wool wait. A joint sale replacement recovers $68 in the traced case. | The broader public exchange gain is inconclusive; do not generalize from one case. |
| Work guards require unnecessary exact output stock | Earlier wool sales caused the inherited worker plan to switch. Separate required farm inputs from market-sale choices. | Removing all stock guards would be unsafe for funding and capacity. No root guard change is adopted. |
| Deposits coincide with hiring and purchase congestion | Our agents have many output arrivals when all ten orders execute. Price seed prebuys and selected earlier deposits as small plan edits. | Seed prebuying now has a controlled test, but its incremental margin is unproved. Route, tile and composition edits remain untested. |

## What we have measured in our retained agents

`runs/own_market_structure_v0` profiles room_keep and sale_priority on eight new seeds, three exposed opponents and both seats: 48 full games per agent. Both have the same production and placement statistics in this sample. Here, output means carrot, tomato, strawberry, melon, egg, milk and wool; wheat and fertilizer are counted separately because they are also farm inputs.

- 48.97% of this output reaches the shed at the day boundary or during hour 0. Only 6.74% of sold output is sold in hour 0. The profiler combines automatic overnight deposits and explicit hour-0 deposits under that timestamp; it does not distinguish them.
- There are 11.94 turns per game with ten accepted market orders. Across 48 games, 1,302 product-arrival events, containing 11,682 units, coincide with a full accepted order list and no sale of that product in that turn. These are opportunities to inspect, not proof that an immediate sale would be better.
- Every game reaches the 100-item shed limit; the shed contains at least 90 items for 9.31 turns per game on average. Nevertheless, neither agent discards any of these output products in this sample. Average fertilizer discard is only 0.5 units. Avoid spending a large effort on an overflow problem that the current baseline mostly handles.
- Movement is 45.34% of successful worker actions. Cows are already close to shed-access tiles: mean minimum Manhattan distance 1.15; sheep 2.18; geese 3.12. Strawberry placements average 3.91 and range to 7. Distances describe placement, not the cost of an actual route; workers can serve several tiles on one trip.

Evidence: `runs/own_market_structure_v0/SUMMARY.json`, the per-game profiles, and the saved command list. These agents share a farm-plan family; this is not a survey of independent compositions.

## Proposed farm edits and their tests

### 1. Make farm-work guards independent of unnecessary output stock

Observed obstruction: selling two wool earlier gives our agent $398 more cash, but its day-7 exact shed check fails because the wool is absent. It chooses different work at turn 168, hires differently at turn 169, and has additional failed work by turn 188. This prevents the market component from improving sales independently.

Proposed change: for a supplied farm plan, check required physical state, input quantities, available cash and future storage limits. Do not require an exact amount of an output product merely because the reference replay had it. Keep exact checks where later work or a policy branch really depends on that amount. Do not simply remove all stock guards: purchases, drops and capacity can make apparently harmless stock differences matter later.

Test: replay the same farm work with several earlier and later sales, compile every affected day, and require all planned services and outputs to remain feasible. Then compare complete league games and the old schedule. The immediate-sale overlay itself is rejected; it lost heavily because work changed.

Evidence: `runs/immediate_sales_screen_v0/FIRST_DIVERGENCE.json`; the root parent's `guarded_day.hpp` and day-service guard checks.

### 2. Free market slots when newly deposited goods become available

Observed obstruction: about half the output arrives around hour 0, while hiring and purchases occupy the market. The full-order arrival count above is substantial even though overflow losses are small.

First concrete edit to price: move a fixed-price seed purchase from a crowded hour-0 turn to an earlier unused market slot, if that earlier payment does not block required work. Seeds have separate unlimited storage. Use the freed slot to make an earlier sale available, then compare selling now with waiting. Hiring cannot generally move to the previous day because hired workers disappear at day end.

Test: preserve planting, service dates and worker actions; check every purchase and hiring deadline; recompute both players' cash. Report how many arrival turns gain a usable sale slot and whether that freedom improves held-out margin. Earlier sales are an option, not the objective.

Status after the controlled test: the move creates slots, but the tested policy adds -$3.34 mean margin over slot filling alone, with an interval crossing zero. Keep it as an optional edit to price, not an adopted rule. See `runs/seed_prebuy_screen_v0`.

### 3. Make selected deposits possible before the overnight batch

Proposed change: when a worker already passes a shed-access tile, try depositing a chosen output before the final market phase instead of relying on the automatic overnight deposit. This can provide a choice between selling before a rival's batch and holding through demand. A targeted PLACE retains overflow; DROP can destroy it.

Test the actual extra or replaced worker action. Charge lost harvesting, watering, care or movement, and check all later days. Reject an early deposit whose total margin gain is less than its lost farm output or added labor cost. Start with an existing route that already reaches the shed; do not assume that a dedicated courier pays for itself.

Status: motivated by deposit timing; no causal gain measured yet.

### 4. Judge tile swaps by their delivery calendar, not distance alone

Concrete candidates: examine distant strawberry tiles that cannot deposit before a valuable sale opportunity; try swapping them with a nearer, less frequently harvested crop. Compare with keeping the current nearby cows and sheep. A closer crop can displace an animal that needs daily feed and care, so the net route cost can increase.

For each swap, report worker actions, service feasibility, output amount, first possible deposit turn, shed peak and final league margin. Recompile both affected routes and all later days. The current animal placement is already close to the shed; “move the animals closer” is not a supported general recommendation.

Status: placement and movement costs are profiled; no specific tile swap is yet demonstrated to improve margin.

### 5. Price output concentration and harvest staggering explicitly

When proposing a composition, include the date and size of each delivery batch. Two plans with the same final output can differ because one fills the shed or needs more product sale orders on an already crowded turn.

Try small planting-date or harvest-window shifts before changing the entire composition. Preserve water/feed/care obligations and account for yield, decay and future tile use. More product types require more sale orders; producing more of an existing type can share an order but depress its price more. Neither concentration nor diversification is automatically better.

Concrete small variants include planting one strawberry a day later, or placing one cow a day later to shift its two-day production cycle. Price the lost or delayed output within the game horizon, the changed service calendar and any newly available sale window. Do not assume that smoothing a batch compensates for a lost final harvest.

Test: compare the same lifetime composition with a few feasible delivery calendars, then include the best calendar in complete plan valuation. Measure margin across supplied market scenarios and new complete games, not just average sale price.

Status: a concrete search direction; no general composition change is promoted.

## Examples of plans with more or less sales freedom

These are the clearest examples in the tested sample, not a universal ranking of optimal farms. A plan can fit more than one row.

| Example | What it demonstrates | Measured result and limit |
| --- | --- | --- |
| Otter's three recorded plans | Goods and funding can support some short waits without changing farming. Many opportunities occur on mixed purchase/hiring turns. | Guarded one-turn changes improve margin by $78, $31 and $177; physical work and final stock match. Only three exposed histories, with fixed rival orders. |
| Binghua, episode 107202048, seat 0 | A frequently selling schedule can contain useful timing freedom. | Eleven guarded waits gain $661 margin with unchanged work and final stock. Another Binghua history loses $353 under the same rule; the opponent and market matter. |
| Our room_keep and sale_priority schedules | A strong farm can preserve output yet constrain independent selling through exact stock guards and crowded deposit turns. | No output overflow in the 96 profiled games, but output arrives alongside full market lists and an earlier wool sale can switch the work schedule. Fixing those constraints is a candidate improvement, not a measured gain yet. |
| A batch facing already harvested rival stock: episode 107201424, seat 1 | Waiting is mechanically feasible but economically dangerous even when nothing is currently ready on the rival farm. | Delaying 12 melon across demand predicts +$23 own cash; actual two-turn changes are own -$295 and rival +$297. Current ready yield alone misses stored output. |
| A plan with a missed purchase and a later delivery deadline | A failed early buy need not force weaker farming if cash becomes available before the input is used. | The sheep-course diagnosis misses wheat purchases around turns 241–244 but needs the wheat around 314 with ample cash then. Deadline repair has a replicated benefit; it also has negative cases and changes production. |
| Repeated wool batches near the price floor: seed 2026091010078, sheep course | A wait can leave worse prices after private farm state returns to the baseline. | A two-turn gain of $70 leaves three extra market units; the next wool batch loses $260. Exact ending-market comparison rejects the harmful wait. Fresh confirmation of that guard is running. |
| A plan close to the shed limit | Storage can remove the option to wait, but eliminating every discard need not maximize final margin. | The current baseline loses almost no output to overflow. In a separate recorded plan, rescuing five wool with an extra sale loses $293 final margin. Price effects can outweigh rescued quantity. |

For future comparisons, report two things separately: which sale/purchase choices the farm actually permits, and how much those choices improve final margin across scenarios. A large gain from fixing a poor sales rule is not proof of a better composition; a small gain can mean the existing schedule already sells well.

## Review update — September 10, 00:55 UTC

The general recommendation about rival competition is now more concrete: inspect public harvest and market history, not only currently ready crops. A C++ history component passes its first bound-accuracy audit across 72 exposed episodes, but no sales-policy gain is measured yet. This applies to alternative public farm plans as well as our current agents. Keep earlier deposit, seed-purchase timing, tile-swap and harvest-staggering proposals unpromoted until their own controlled tests establish a benefit.

The new public sample is split before opening confirmation actions. Do not reclassify a schedule as generally friendly from its rank or one successful timing edit; record the required cash, storage and rivalry conditions and the cases where the same edit loses.

## Review update — September 10, 01:15 UTC

A more concrete sales-friendly condition now has controlled evidence: a product is already in our shed, known demand occurs before the next sale opportunity, funding/storage permit the wait, and public harvest/sale history rules out held rival output while none is currently ready. The frozen history rule gains $17.12 average margin on 34 new player plans with no noncash changes. This is a conditional opportunity, not a recommended composition or a claim that waiting usually beats immediate selling.

The counterpart remains important: a rival may have already harvested the competing product. Looking only at its current tiles misses that risk. History avoids the sampled losses of the older guard, but also skips profitable waits; its mean advantage over that guard is unproved. A prior seed-stock difference still prevents a universal identical-state claim.

Next test this opportunity with fixed farm courses against reacting opponents, where the calendar and funding repair are explicit. This directly tests whether the useful recorded-plan freedom survives actual agent interaction. Earlier seed purchases, selected earlier deposits, tile swaps and staggering remain proposed tests; no new placement or composition evidence promotes them in this review.

## Review update — September 10, 01:35 UTC

The three fixed cow/sheep/goose courses now provide a concrete live sales-friendly example. Their explicit delivery calendar and purchase repair permit a small waiting overlay without switching worker plans: +$31.52 mean margin on 9,216 fresh paired games, with no changed physical state, production, stock or faults. All three course means and all six opponent means are positive. This supports exposing financial timing choices alongside compiled delivery calendars. It does not rank the compositions against one another.

Retain the unfavorable case: sheep course, seed 2026091010078, against public_router_v52 loses $175 in each seat despite unchanged farming and rival actions. The short projection predicts +$85. Later market inventory and the price floor are being traced; no causal explanation is promoted yet.

The latest public sample again contains both rapid sellers (Otter, Binghua, feel the agi) and long holders (including Squirrel and Unknown Mother-Goose). Otter's 42 waits and feel the agi's 49 waits all coincide with ten successful orders. Squirrel has 2,166 waiting product-turns with unused order capacity, so capacity is not a universal explanation. Keep sale timing conditioned on the farm's actual funding, delivery and rivalry constraints. Seed prebuying, deposit changes, tile swaps and harvest staggering remain untested proposals.

The two live losses are now explained by the shared market, not by a farming failure. The wool wait gains $70 immediately but leaves three extra market units; the next wool batch loses $260. Plans with repeated batches near the price floor need the later price effect included, even when cash, space and worker obligations permit a wait. This is a concrete sales-unfriendly condition for a short valuation horizon. Starting quote $11 did not avoid the floor within the batch. See `runs/history_course_checks_v0/FLOOR_DIAGNOSIS.json`.

## Review update — September 10, 01:55 UTC

The one-turn rule with ending-market-inventory equality now has fresh-seed confirmation: +$31.39 mean margin over purchase repair on 9,216 paired games, with no losses or checked noncash changes in this sample. Keep this as the supported small timing improvement on the three supplied courses. Earlier wool losses remain part of the record and explain why the extra market-state condition matters.

A proposed wider sales-friendly window has a concrete game-rule basis: if the rival has no possible stored output of a product and none is harvestable, new output cannot appear until the next night. Try later known-demand sale turns within that day, subject to every intervening cash, storage and work obligation and unchanged ending market inventory. The corrected two-seed smoke is encouraging; the first integration omitted the ready-yield condition and is rejected. This longer window is not promoted yet.

No new evidence supports a tile swap, harvest staggering or earlier seed purchase in this review; retain them as proposed controlled tests. The public refresh is running again to check changing opponents rather than assuming the current local packages describe the whole field.

## Review update — September 10, 02:15 UTC

The same-day sales window now has fresh live evidence: +$336.04 mean margin over purchase repair, or +$305.80 over one-turn waiting, on 9,216 paired games across the three supplied courses and 12 opponent packages. No checked noncash state changes or losses occurred. The useful condition is concrete: delivered output, enough cash and shed space for all intervening obligations, a later known demand event before night, and no possible held or harvestable rival output. Ending shared market inventory must also match. Preserve this window when compiling a supplied farm plan; do not force the earliest sale merely because delivery is complete.

This condition also helps 26 of 38 new public player plans, with +$251.92 mean margin and no measured losses or noncash changes. That broadens the evidence beyond our courses, but rival actions are fixed in this replay test. The 21 reserved public episodes have not yet been evaluated.

Repeated wool batches near the floor remain a concrete unfavorable example for short valuation: unchanged private resources alone do not protect later prices. The exact market guard has completed fresh confirmation; the earlier table's “running” status is superseded. No specific planting, placement or movement change is promoted this review. Smaller timing gains do not establish that a composition is inferior; its original selling may already be better.

## Review update — September 10, 02:35 UTC

Ready rival output need not mean an immediate competing sale. A public movement bound now supports a concrete extra opportunity. In episode 107305772, Tarang222, seat 1 has 24 strawberries in the shed at turn 552. Rival Otter's farmer is at (4,4), with no hired workers; its closest ready strawberries include (5,2), (6,3), (7,4). Even an optimistic new hire cannot deliver before turn 558. The tested rule moves the 24-unit sale to 557, predicting an extra $428 for that edit. The whole edited game gains $3,898 over recorded orders, or$3,307 over the day rule, with unchanged checked farming/resources. This is an exposed example, not an isolated proof that every such delay earns $428. See `runs/delivery_wait_public_screen_v0/EXAMPLE_TARANG_EVENTS.json`.

The general recommendation is to retain delivery-time freedom and evaluate it against the rival's earliest possible sale. This strengthens the case for measuring arrival dates when comparing routes; it does not establish a beneficial own tile swap. The delivery rule's2,304-game live screen gains $211.18 over the day rule without sampled losses or noncash changes. Fresh confirmation is next.

Keep unfavorable conditions explicit: possible already-held rival output gives no guaranteed waiting time; a ready product close to a worker and shed may leave no later safe sale turn; funding, capacity, order slots and price-floor effects can still remove the benefit. Specific seed prebuys, early deposits, tile swaps and staggering remain untested. Public Squirrel plans with no day-rule gain are not thereby worse compositions.

## Review update — September 10, 02:55 UTC

The delivery opportunity now has fresh live confirmation: +$193.95 mean margin over day timing on 9,216 paired games, without sampled losses or checked noncash changes. The public confirmation gives +$116.03 over day timing but includes the explained seed-stock exception; do not describe every supplied schedule as equally safe to edit.

A concrete sales-unfriendly schedule has purchase requests whose filled quantities depend on spending nearly all available cash. In SpaTaro episode 107273702, a $3 sale gain enables an extra $10 seed purchase; the resulting $7 cash reduction prevents a later wheat-product purchase from filling equally and changes subsequent farming. Explicit required purchase quantities and funding deadlines are better inputs than blindly retaining every requested buy, including unsuccessful ones. This is a contract recommendation supported by the trace, not a demonstrated change to our own composition or tiles.

A global seed-use cap fixes final leftover seeds in two simple cases but does not prevent this earlier funding cascade. Next price purchases against the next supplied restocking opportunity and verify every service and endpoint. Do not adopt that idea before testing it. Earlier deposits, fixed-price seed prebuys, tile swaps and harvest staggering remain untested farm edits.

## Review update — September 10, 03:15 UTC

A sales-friendly plan specifies intended purchase quantities and funding deadlines, rather than orders that mean “buy as much as the current cash happens to allow.” In 394 exposed public player plans, fixing purchases to the quantities executed by the supplied funded schedule removes all three state exceptions under delivery timing. The timing rule then gains $255.91 average margin over original orders with unchanged checked farming/resources. Normalization alone leaves every checked original state and cash balance unchanged.

This directly addresses the unfavorable SpaTaro case: a small sale gain must not activate an extra seed purchase that takes money from required feed. It is an input-contract recommendation, not a measured improvement to our own tiles or composition. General purchase-date optimization remains open. The proposed future-restock cap is now rejected: later requested buys were not reliably funded, and the resulting plan changed farming and eventually lacked required workers. Do not use that shortcut in a compiler.

No new evidence supports earlier deposits, seed prebuying, tile swaps or harvest staggering. Keep those as controlled-test proposals. Continue testing the timing conditions across changing public programs, not only the three local courses.

Concrete storage tradeoff after review: in the sheep course against Salem, seed 2026091060015, extra tomato holds prevent a later eight-wool wait worth $71. The resulting shed would contain 105 units at turn 717, exceeding capacity 100. The extra small gains total only $17, so multiple edits lose $54 versus one-edit timing despite unchanged farming and exact price predictions. See `runs/history_course_multiple_screen_v0/CAPACITY_DIAGNOSIS.json`. Preserve the option to revise market-sale commitments when later output needs the space. This is not evidence for changing sheep placement or production dates.

## Review update — September 10, 03:35 UTC

Multiple-sale timing now has fresh live confirmation: +$41.55 over one-edit timing on 9,216 paired games, with nine small losses and unchanged checked farming/resources. The simplest supported recommendation is to keep feasible sale dates open within a supplied delivery calendar, subject to funding, storage and public rival-delivery information. It is not necessary to change the composition to obtain these gains.

Storage has an opportunity cost even when nothing is discarded. The tomato/wool case makes this concrete: one joint replacement recovers $68, but the broader exchange rule adds only $0.60 on the 2,304-game live screen, with an interval crossing zero and 30 losses. Pause the rule; do not promote a general storage policy from the example.

Next test the earlier seed-purchase recommendation directly. Move a fixed-price seed purchase into an earlier empty market slot only when the required quantity can be funded, and compare sale-slot filling with and without that move. Report intended earlier seed availability separately from other physical changes; preserve every plant/service, final seed requirement and complete-game failure. No seed prebuy, deposit, tile or composition edit is promoted yet.

## Review update — September 10, 03:55 UTC

The seed-prebuy proposal now has a controlled test. It creates more usable sale opportunities without changing farming or ending resources, but the tested policy does not use those opportunities well enough: -$3.34 mean margin over the same slot-fill control, with nine gains and 17 losses. Mengfei Li episode 107271700 seat 1 loses $854; episode 107294155 gains $218. A different supplied plan, episode 107312273 seat 0, gains $1,792, including reduced rival receipts. These are fixed-rival counterfactuals, not live-policy gains. See `runs/seed_prebuy_screen_v0/EXTREME_EXAMPLES.json`.

Keep seed prebuying as an optional plan edit to price, not an adopted general rule. Its fixed price and separate storage make it feasible in useful cases; they do not make every earlier sale profitable. The public earliest-sale bound does not predict that the rival will sell as soon as possible. No deposit, tile or composition change is promoted.

The stronger existing recommendation remains supported: expose feasible sale timing alongside the delivery calendar. Multiple-sale timing adds $11.53 over one-edit timing on 64 new public plans, with no losses or checked state changes. The latest confirmation is in `runs/multiple_sales_confirmation0345_v0`.

## Review update — September 10, 04:15 UTC

A borrowed farm plan must be priced from a coherent entry state. In the scenario audit, assigning the observed rival's cash to another farm's historical inventories/work makes many of those calendars fail. Restoring that historical farm's recorded cash removes 2,019 failed branch forecasts. This does not make it the observed rival's farm: public composition and placement compatibility still need checking.

All actual fixed-branch outcomes stay unchanged in this test; only selected compositions differ. The selector's mean gain over its cash-splice version comes entirely from avoiding fallback, with substantial individual losses and no proven advantage over the older model chooser. Do not label the selected cow/sheep/goose composition generally better from this audit. The full new-seed comparison is pending.

No seed-prebuy, deposit, tile or composition edit is promoted. Keep the measured sale-timing freedom and the negative seed/holding examples visible while improving how scenarios are supplied to plan valuation.

## Review update — September 10, 04:35 UTC

Preserving feasible sale-date choices remains the strongest supported recommendation. The unchanged rule adds $269.50 over original orders on 68 new public plans, and multiple edits add $12.00 over one-edit timing, without sampled losses or checked state/prediction exceptions. These tests supply funded purchase quantities and fixed rival actions; do not describe them as optimizing purchase dates or changing the composition.

The broader scenario-selector test does not promote any cow/sheep/goose choice. Its small uncertain gain over the previous selector includes 218 losses and five negative opponent means. Actual fixed-branch outcomes remain identical, and every changed choice comes from avoiding fallback. A plan must retain its stated entry state when valued: a borrowed farm with different public composition, services or cash is a stress case, not the observed rival's continuation. The extended audit finds no complete public-state match among the supplied pairs.

No seed prebuy, earlier deposit, tile swap or composition edit is promoted. Keep the concrete exact-stock guard, funding cascade, floor-price and tomato/wool storage conflicts as unfavorable examples. Cows remain already near the shed. Latest public patterns again include both immediate sales and long holds, so neither timing style defines a generally optimal farm.

## Review update — September 10, 04:55 UTC

A local farm-plan edit can affect prices after its stated period, even when private stock matches. Preserve the ending shared market state when passing that edit to later valuation. The financial API now returns this state and passes the full split/resume audit. This is an interface improvement supporting complete marginal accounting; it is not a new planting, placement or route result.

The measured recommendations remain: keep feasible sale dates open, supply funded input quantities/deadlines, and avoid unnecessary exact output-stock guards in work selection. Preserve the unfavorable funding-cascade, price-floor and tomato/wool capacity examples. The tests still do not promote seed prebuying, earlier deposits, tile swaps or staggered compositions. Cows are already close to the shed.

The final result summary separates our original-agent gain from fixed-course timing gains and the failed broader selector promotion. The newest 25 public episodes remain unused for the next frozen integration; no sales-pattern claim is made about their uninspected actions. Generality must be tested again as programs change.
