# Results and retained components

The session produced a small direct improvement over the original agent and a larger reusable sale-timing improvement on supplied farm courses. General purchase optimization and better composition selection remain open.

Margin means our final cash minus the rival's final cash. Win utility is 1 for a win, 0.5 for a tie and 0 for a loss. A paired comparison uses the same seed, opponent, seat and supplied course or plan. Do not add gains from different controls or test populations.

## What improved

| Change | Paired comparisons | Mean margin gain | 95% interval | Status |
| --- | ---: | ---: | ---: | --- |
| `room_keep` versus the original `early_structure_cow` agent | 6,144 | +$24.22 | $20.50–$28.12 | Retain as the conservative direct improvement. |
| `sale_priority` versus `room_keep` | 3,072 | +$50.16 | $33.91–$66.17 | Useful challenger; several opponent means are negative. |
| Multiple-sale timing versus purchase repair on the same three courses | 9,216 | +$574.93 | $519.40–$635.71 | Retain the timing component on the tested courses. |
| Multiple-sale timing versus one-edit delivery timing on those same games | 9,216 | +$41.55 | $37.75–$45.67 | Keep the one-edit control and the nine small losses. |
| Unchanged multiple-sale timing versus original orders on the latest 68 public plans | 68 | +$269.50 | $183.07–$371.75 | Supports later-plan transfer under the supplied purchase contract. |

Live intervals cluster whole seeds across opponents, seats and shop panels. Public intervals cluster episodes across both player plans. These intervals do not establish transfer to independent source-code families.

`room_keep` removes guaranteed ineffective sale orders and sometimes sells before an automatic deposit when the refill preserves every baseline shed quantity. It has 2,732 gains and four losses, worst $2, with unchanged production and labor and no added faults. Every tested opponent/shop-panel mean is positive. This is the answer to the comparison with the baseline used before the session. Evidence: [broad baseline comparison](evidence/runs/broad_room_v0/AGGREGATE.json), [uncertainty](evidence/runs/broad_room_v0/AGGREGATE_UNCERTAINTY.json), [implementation](agents/room_keep/README.md).

`sale_priority` orders existing sale-only turns by available quantity times current quote. It has 969 negative comparisons, worst $752, despite positive overall mean and win utility. It is not a uniform replacement for `room_keep`. Evidence: [agent and checks](agents/sale_priority/README.md), [broad screen](evidence/runs/sale_priority_broad_v0).

The timing component keeps output until a later known demand event when funding and storage allow it and public history and travel bounds rule out an earlier competing sale. It preserves the existing work/calendar obligations, retains future sale commitments and uses at most 1,000 financial-turn evaluations per call. The 9,216-game confirmation has no losses against purchase repair, unchanged checked farming/resources and no extra faults. It adds about 0.96 ms per complete game over that paired control, including the live observation adapter and one-time course compiler. Three small own prediction gaps and six changed rival action hashes remain recorded. Evidence: [live confirmation](evidence/runs/history_course_multiple_confirmation_v0), [numerical gates](evidence/runs/history_course_multiple_confirmation_v0/PROMOTION_AUDIT.json), [API checks](evidence/runs/history_course_checks_v0/MULTIPLE_CHECKS.json).

The latest public confirmation adds $12.00 over one-edit timing (95% interval $5.57–$20.22), with 20 gains, no losses and 48 unchanged plans. Every job completes; checked noncash state and predicted own cash match. These are fixed-rival counterfactuals with supplied purchase quantities, not reacting-opponent games. Evidence: [final public confirmation](evidence/runs/multiple_sales_confirmation0425_v0/SUMMARY.md).

## What can be reused

Start with [REUSE.md](REUSE.md) for the input contract and call sequence. The financial solver consumes resource events, money, market state, a starting order plan and public rival-sale bounds. It does not choose our tile placement or worker movement.

- [Financial simulator](include/market.hpp) and [continuation evaluator](include/continuation.hpp): preserve funding, storage, event order and the price floor. Retain failed scenarios explicitly.
- [Rival held-stock history](include/rival_stock.hpp) and [earliest-delivery bound](include/rival_delivery.hpp): legal public information, with conservative uncertainty.
- [Sale-timing search](include/day_timing.hpp): one-edit and multiple-edit variants under the same deterministic budget.
- [Live example](agents/history_course_multiple/README.md): supplied cow/sheep/goose courses, existing purchase repair and the timing component. The default API branch is the cow course.
- [Replay purchase contract](include/case.hpp): a historical funded schedule may supply executed purchase quantities and dates. This is benchmark input, not knowledge of future live purchase fills.

The continuation result also retains ending shared market inventory, shops and time. Splitting and resuming a supplied financial scenario matches one-pass evaluation in all 111,672 checked splits across 297 episodes. This closes an output-contract gap for valuing later effects; it is not another strategy gain. See [continuation checks and cost](evidence/runs/continuation_resume_v0/SUMMARY.md).

The compact financial kernel passed 184,064 differential transitions; its narrow measured cost was 0.133 microseconds per turn. Full-calendar and live-policy costs are reported separately. Generic and typed runners, debug action validation, self-play, PASS games, deterministic budgets and independent instances were checked for the retained agent. These checks support this implementation, not every possible caller contract.

## What did not earn promotion

| Idea | Evidence and decision |
| --- | --- |
| Broad immediate selling on our inherited agent | Earlier sales changed exact-stock work guards and then the farm plan. Reject the overlay; it did not isolate financial timing. |
| Short-window valuation without ending shared market state | A wool wait earned $70, left three extra market units and lost $260 on the next batch. Retain the exact ending-market condition. |
| Cap seed buys using later requested restocks | Later requests were not reliably funded; work changed and a game lacked required workers. Reject the shortcut. |
| Global remaining-seed-use cap | Only $40 total gain over 394 exposed plans; it does not prevent an earlier funding cascade. Keep as a small endpoint tool. |
| Joint replacement of held output | Fixes a traced tomato/wool conflict, but the broad extra gain is inconclusive and has losses. Pause the prototype. |
| Earlier seed purchases to free sale slots | -$3.34 mean margin versus the same slot-fill control, with an interval crossing zero. More sale opportunities did not establish better selling. Pause the policy. |
| Preserve historical rival cash when valuing historical calendars | Removes forecast failures, but the fresh selector comparison is +$87.47 with interval -$245.16–$426.84 and 218 losses. No promotion. |

The scenario finding is particularly relevant to plan search. Every changed choice comes from avoiding the old no-eligible-branch fallback; numeric rankings never change. In a separate public-state audit, 908 of 1,280 observed rival states have no matching historical noncash public farm, and none matches every public field including cash. Historical scenarios can be useful stress cases; they are not automatically forecasts of the observed rival. See [selector confirmation](evidence/runs/scenario_cash_confirmation_v0/SUMMARY.md) and [compatibility audit](evidence/runs/scenario_public_compatibility_extended_v0/SUMMARY.md).

## Farm and meta lessons

Expose feasible sale dates after delivery. Keep required inputs and their funding separate from unnecessary exact output-stock guards. Storage has a cost even when nothing is discarded: a low-value hold can block a later valuable batch. A fixed-price seed prebuy is an option to price, not a reason to force an earlier sale.

Our own profile finds crowded order lists around many output arrivals; cows are already close to the shed. No specific deposit, tile or composition change has earned promotion. Concrete examples, negative cases and proposed controlled tests are in [FARM_RECOMMENDATIONS.md](FARM_RECOMMENDATIONS.md).

Public agents continue to show both immediate selling and long holds. Observed behavior does not establish optimality or author intent. Replays and notebook metadata were refreshed throughout the session; program IDs, exposure dates and source hashes are retained under `research/`. Keep new opponents unused until a future policy is frozen when testing independent transfer; the current extracted corpus is exposed.

The next integration should measure whether the retained financial component improves the main pipeline's chosen plans under coherent starting states, supplied resource calendars and reacting opponents. Preserve the strong controls, all failed cases, mean and tail margins, win utility and full compute cost. This session has not established that larger pipeline result. No root agent or Kaggle submission was changed.
