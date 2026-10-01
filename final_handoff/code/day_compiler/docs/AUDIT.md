# dc11 full-read audit (hidden assumptions)

Source: experiments/v10/sep25_compiler_overhaul/dc11/ (= snapshot v95). For each component: the assumption, where it is, the
evidence vs M&M, and what dc12 does with it. Other sessions' parts are linked from their findings files
(work/mm_handoff/findings/): bind (BC), funding (Weaknesses), market + executor seller (Imitation).

## Router (router.hpp, router.cpp; read in full by the Day compiler)

| # | Assumption | Where | Evidence vs M&M | dc12 |
|---|---|---|---|---|
| R1 | Every worker turn costs $2 in the route cost (`wage_per_turn` x the route's span). Hires are paid once per day (Fibonacci), so a hired worker's turns until the day's end are free. The fictional turn cost makes routes compact, ends them early (idle after), and leaves work undone whose value is below its turn cost, even when workers have slack. | walk(): `e.cost += pb.wage_per_turn * span`; site_cost; final-return test | M&M's crews work to the day's end (idle 0.6 vs ours 5.6 per day); M&M waters one-shot melons / tomatoes and serves animals more fully. Untested as the cause. | Tested (18:35, router gate days 12-25, M&M intents): turn cost 0 -> hires 12.06 -> 12.66, moves +21 / day, dropped 0.41 -> 1.88, value -140 / day, waterings not up. It works as a compactness regulariser: keep. |
| R2 | Deposits earn the sale DP's value of a unit reaching the shed by hour; the DP sells in the evening, so early deposits earn nothing extra. | walk() shed stops, `pb.gain` from deposit_values() | Our animal service is ~2 per kind later than M&M's (days 12-17). | Deposit values come from the dc12 seller. |
| R3 | Hires start at h1 (first 10) or h2 and work the whole day; there are no later hires. | start_of(), first_wave | M&M hires at h0-h2 after its first sales on $0 dawns; hires are per day, so no wage saving from hiring late. Late crew tested: no gain. | Keep. |
| R4 | Only the farmer can use dawn shed stock at h0; hires' pickups wait for purchases. | pickups(): `w.hour == 0 ? dawn_stock` | Realize buys only what the shed lacks, so this is timing only. | Keep; recheck in realize. |
| R5 | A stop is atomic: one worker does all of a tile's steps in a row. | Stop / walk | Accepted in dc11 (splitting lost on animals). | Keep. |
| R6 | Pickups are sized to the trip's peak need, one PICKUP per item type; a deposit DROPs everything. | pickups(), shed stops | No supply is dropped and re-picked (probe: 0 of 26,847 walks). | Keep. |
| R7 | The last deposit trip is made only when the DP's evening deposit value beats the trip's turn cost; otherwise the cargo is deposited automatically at night. | walk() final return | Depends on R1 and R2. | Follows the dc12 seller and turn value. |
| R8 | Unserved work costs fixed tiers (survival 100000, output 10000, new 3000, extra 0) plus its value. Fertilizer-only collections are optional, worth 0.9 x price. | PRIORITY_COST, bind | With R1, optional work below $2 / turn is skipped even with slack. | Values stay; the turn cost goes (R1). |
| R9 | Hire search: up to the first complete crew, +1 while the cost falls, then down by dissolving routes (v94 dropany). Cost = wages + route costs. | route_day() | Crew at M&M's size with dropany=6. | Keep (dropany on). |
| R10 | Search: greedy construction (survival first, later release first, far first); best insertion over the 6 cheapest detours; relocate / swap (radius 4) / tails / reverse / member / shed / entity passes; 6 rounds. | Search | Waits ~0; routes full. | Keep. |
| R11 | Sites: 5 candidate tiles per new entity by site cost + nearness; site cost = weight x $2 x shed distance; crops avoid shed-adjacent tiles before day 16. | place_entity(), site_cost() | Layout matches top teams on average (dc11 DESIGN #12); per crop inverted (melons farther). | Keep; revisit with R1. |
| R12 | Moves are Manhattan (x first, then y); units do not block each other. | move_to() | Engine: no collisions. | Keep. |
| R13 | The farmer's work: no fixed assignment (an older compiler always sent the farmer to the animals). | router route 0, realize (the farmer keeps route 0) | route 0 is filled by the same search as the hires' routes; the farmer starts at h0 from its tile, hires at h1 / h2 from spawn tiles. | Keep. |
| R14 | Step-down cost: the hire search's step down (dropany) searches each dissolution candidate in full; the final level searches all 7 only to confirm none fits. | route_day | 71% of routing time (52 full searches per compile, 2.3 per level); short probes lose 16 / day. | Keep; an exact infeasibility bound would be needed for a safe prune. |

## Market model and executor seller (Imitation; work/mm_handoff/findings/imitation.md, "dc11 audit" M1-M11, S1-S7)

Headline from a per-decision log (DC11_AUDIT) on 72 M&M worlds:
- Against M&M's fixed recorded schedule, the model predicts its planned lots' prices within ~$1 per unit.
- In the mirror (copy vs d3crop, both our DP), later lots realize $3-8 per unit below prediction, and the opponent sells
  +0.5-1.2 units more than forecast by the evening.
- So M1, a point forecast of the opponent with no response model, is the core fault. It feeds the deposit values (M9) and the
  hold value (M5).
- M2: our own sales run 1.5-2.7 units behind our own plans (time-inconsistent re-planning).
- M6: the night-room charge binds in 17-38% of thin-product decisions at ~$12 per unit. M&M carries the same thin stock
  overnight (19 vs 19-21) with a fuller shed (91 vs 74), so room size is not the difference.

## Bind: waterings (BC, 87 unseen M&M games, days 6-16, CPP5c; full bind audit pending in findings/bc.md)

- Melons: the network's water intent equals M&M's (66.5 vs 67.2 per game). dc11 executes 64.3-64.9 vs M&M's 67.8: it drops
  ~2.6 yield waterings per game. The live-world gap of -11 melon waterings is mostly fewer second-wave melons (volume).
- Tomatoes (ongoing; dc11's rule alone decides): ours 39-41 vs M&M 56.6 per game (-15 to -17). The rule waters an ongoing crop
  only when dry, or when a doubled yield is due (compiler.cpp bind, retained crops): a hidden assumption to test.
- Wheat is equal.
