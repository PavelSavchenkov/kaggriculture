# Profiling learnings

Record successful and unsuccessful diagnostics with measured cost and decision impact.

- Measure the entire planner call, including scenario construction and resource checks. A fast sale-price inner loop alone is insufficient.
- Batch repeated games and policies in C++; no subprocess or compile per game.
- Keep requested and accepted purchases/sales distinct. Missing inputs and full storage can invalidate counterfactual tapes.
- Separate original-world reproduction, fixed-calendar transfer, and reacting-opponent games. They answer different questions.
- Cache only deterministic facts whose keys include all relevant inputs. Keep deterministic work counts for comparisons.

## New evidence

- Full own resource projection from legal observations agrees on 86,280 states at about 0.83 microseconds per call. Suppressing market and day-end work in an observation-built local simulator is valid only for the returned inventory projection; its resulting farm/time state is not a continuation state.
- Storage counterfactuals exposed six financial/full-engine disagreements despite no reported input deficits. Source calendars fixed actual accepted PICKUP quantities; larger changed stocks can make literal future actions take more. Full engine checks distinguish a funded financial calendar from unchanged realized production.

- Kernel v0: 184,064 differential transitions pass, 0.133 microseconds per tested turn. Sparse random markets are not representative policy throughput; benchmark real calendars separately.
- Calendar v0 full-engine check exposed a malformed-action encoding error: itemless PLACE became PLACE WHEAT1. Item defaults must remain invalid. Six corrected calendars then matched all719 transitions in finance and full-engine execution. Retain rejected fixtures/results; do not weaken parity assertions to make a replay pass.
- Order-compaction v0 exposed a benchmark contract error: hire/land checks used original order slots. The resource plan needs successful counts at the stated turn; slots are financial decisions. Corrected checks compare actual counts by type, including unexpected extra hires/land. Retain v0 results as superseded and rerun before quality claims.

- Eight-scenario continuation valuation averaged 0.50 ms/full financial season on 80 development calendars (p95 3.22 ms, maximum 10.72 ms); 0.17 ms on 32 fresh calendars. Exact narrow storage alone is about 0.11 ms. Source loading/extraction is outside these rollout times and must be included when benchmarking a plan-search batch.

- Separate market timing effects from policy-state guards: in the immediate-sale screen, worker actions first differ at turn168 and worker count at169, before any added action failure. Candidate cash is higher. The exact shed guard sees two fewer wool. Trace the earliest action/state difference, not just the final revenue loss.
- Count trade behavior using stock after worker actions and executed per-order quantities. Distinguish product-turns from turns; count seeds separately from shed products. Order-slot limits explain all11 Otter premium waits in the latest sample.

## September 10: interpretation and checks

- Replays identify behavior, not its optimality or the author's reason. A failed immediate-sale overlay on our agent cannot settle whether another agent benefits from delayed sales. Test the other farm plan directly and preserve its work (`runs/otter_wait_v0`).
- Compare live physical state after every turn when claiming fixed work: tiles, workers, positions, carried stock, seeds, hires and land; also check terminal shed stock, production, discards and faults. Final cash agreement alone can hide changed farming.
- Freeze the selected player seats before aggregation. A paired replay file includes incidental opponents that must not be reported as additional Otter observations.
- Report positive and negative conditional effects. A purchase repair may lower work failures yet reduce a particular product or final cash. `compare_portfolio_variants.py` preserves all paired cases and negative cases; it clusters uncertainty by seed across opponents, seats and branches.

- Seed stock and terminal input stock can change even when every financial obligation, production total and final cash prediction agrees. Track them separately; current two-turn checks do not constrain later purchase fills. This exposed two noncash differences in the 138-plan timing transfer test.
- Raw public replay action lengths can exceed active workers. Do not truncate them casually: inactive PLANT requests can still contribute to seed demand in the exact engine. Preserve source semantics in an explicitly labeled offline replay mode; enforce the stricter local API on actual agents.
- A deposit-timing profile may label both overnight arrivals and explicit hour-0 deposits with the same turn. Report them together unless the source records the phase. Do not claim all such arrivals are automatic night deposits.
- Estimate full order congestion only from a proved lower bound reaching the game's cap. Our profile counts each accepted fixed-cost order plus distinct bought/sold product directions; if that reaches ten, all ten slots were used. A lower count does not prove free slots, because repeated same-item orders may be collapsed.

- Review 00:55: separate belief validation from strategy validation. Check possible harvests, private-stock bounds, own accepted sale quantities, and inferred rival sales against truth before judging a timing policy. Truth belongs to the evaluator only.
- Above the price floor, market-inventory changes for products that cannot be purchased identify total sales after known consumption is added back. Subtract our known sales to infer the rival's. At the floor, some sales add no inventory; retain uncertainty rather than treating zero inventory change as zero sales.
- Report both underestimation and conservative false risk. Zero missed private stock can be achieved by a useless always-positive bound; the audit also counts exact and correctly empty estimates. Standalone update timing excludes observation construction and own-action projection.

- Review 01:15: freeze the candidate binary before opening confirmation actions; retain the original baseline, an attribution baseline (same output-product scope with old guard), and all failed or changed-state cases. The 17 reserved episodes are now used. Different submission IDs and teams do not establish independent code families.
- The history rule has zero bound errors in the additional 211,386 discovery observations. A separate policy test is still essential: despite stronger downside results, the mean comparison with the old guard remains uncertain. Report both reference comparisons, not only the favorable one.
- Public notebook reading reinforces two diagnostics: compare executed trades rather than requested orders, and compare action variation within the same observed shop sequence. Identical sampled actions are behavioral evidence, not proof of shared source code; crude opening-agreement clusters must not define independent test families.

- Review 01:35: live paired execution distinguishes a pricing error from changed farming. The two confirmation losses preserve all checked physical state and rival actions, yet realized gain differs from the two-turn prediction. Inspect shared market inventory after a timing edit as well as private farm resources; later quotes may retain an effect even after shed stock matches.
- Generic serial, typed pair parallel and mask-checking debug parallel agree on all 16 complete PASS game records and action hashes for history_course. Full self-play also passes. Complete own policy cost includes public observation construction and the one-time course compiler; the history overlay adds about 0.41 ms per game in confirmation.
- Replay team names can change. Unknown Mother-Goose appears as Blurry in its three selected replays. Episode metadata confirms submission 56127667 and team 16730612 at seat 1 in all three. Resolve this by stable IDs, not by guessing from the opponent name; preserve both names and the mapping method in SELECTED_SEATS.json.

- Confirmed short-horizon accounting failure: equal terminal private stock does not imply equal shared inventory when some sales hit the price floor. The wool trace identifies the first residual market difference after the delayed sale and the next affected cash flow. Report those phases rather than attributing the loss to opponent reactivity; rival actions were identical.

- Review 01:55: enforce essential preconditions in the reusable function interface. The day planner now requires current ready yield separately from the private-stock uncertainty mask. An implicit caller obligation was missed even though the agent computed the needed information.
- Preserve paired controls across implementation changes. The exact inventory guard changes only the two bad wool decisions in the used 9,216 cases; all other action hashes agree. Generic serial and typed debug parallel match on 16 complete guard-agent records, and self-play passes.
- Separate smoke strata. A PASS opponent can make longer storage look much more valuable than an active seller. The corrected smoke has exact predicted/realized own gain but only two seeds; its pooled interval is not evidence of broad strength.

- Review 02:15: the full same-day agent adds about 0.61 ms per game over its paired purchase-repair control in fresh confirmation (5.624 vs 5.013 ms, including observations and the one-time compiler). Do not compare absolute timing across separately running control batches: host contention changes their compiler time. The 38-plan public replay batch takes 0.66 seconds including whole-game checks and event output; this is end-to-end throughput, not standalone decision latency.
- A larger safe horizon follows the game boundary: no held or currently harvestable rival output means no new competing output before night. Enforce that precondition in the function, compare resources at intervening turns and shared inventory at the target, then verify the whole game. Generic/pair/debug checks for the new day wrapper remain pending; large custom-driver tests already validate actions.
- Public refresh metadata yields 19 discovery and 21 reserved episodes. One incidental appearance of a new program was moved to reserve using submission IDs before opening actions. New team names and versions are not proof of independent code. Keep live confirmation, public discovery and reserved confirmation separate.

- Review 02:35: audit every previously issued earliest-sale bound against each later actual sale, not just next-turn sales. The C++ audit covers1,670,956 product observations and47,989 sale events with zero violations; 97,312 ready-output observations permit extra time. Bounds include new hires and overnight deposits.
- Delivery timing's broad screen adds about 0.95 ms/game over its paired repair control; compare paired cost because separate batches have different host contention. One complete80-plan replay variant takes about1.27 seconds with whole-game checks and event output. Live screening wall time is dominated by the sale_priority opponent (about 32 seconds/panel versus2–4 seconds for most packages), not the financial overlay. Do not optimize an unrelated opponent to claim planner speed.
- The day wrapper's full100,000-expansion PASS run agrees exactly across generic serial and typed debug parallel on 16 games; low-budget checks alone could leave its compiler incomplete. Self-play and zero-budget fallback also pass. Delivery-wrapper checks are pending its promotion stage.
- New public metadata has31 episodes absent from the166 exposed corpus and 7 already used. The 31 are reserved for unchanged-rule confirmation before action inspection. Program names are not independent-family labels.

- Review 02:55: trace a single sale edit through the whole game before naming the cause of a production difference. In 107273702, the first seed difference is followed by a cash shortage and a smaller wheat-product buy; it is not evidence that seed availability directly enabled extra planting. Keep original and modified cash, seed counts, orders and the first non-seed difference.
- Separate requested buys removed from money actually saved. The global seed cap removes 127 requested seed units across 394 plans, but saves only $40: most removed requests would not have filled. Final seed equality is now reported explicitly alongside interim seed differences, other physical differences, production and faults.
- Delivery confirmation adds about 0.82 ms per game over its paired repair control (6.271 vs 5.449 ms, including observations and compiler). Its exact generic/debug pair records agree in full-budget tests. No additional broad repeat is needed without a new implementation change or unresolved concern.

- Review 03:15: normalize purchase requests separately from evaluating sale changes. The C++ check changes 8,716 requests yet preserves both players' cash, physical state, shed, output, discards, market inventory and shops at all 283,286 turn comparisons in 197 episodes; complete runtime 1.01 seconds. This isolates the input-contract change from the sale rule's gains.
- Treat actual source purchase quantities as supplied plan data in this benchmark. They are not available as future observations in a live agent. Keep the unnormalized control, all three state exceptions, the failed restock batch and the four changed witness cases. The development corpus is exposed; a new public refresh will provide prospective confirmation cases.

- Review 03:35: the multiple-sale wrapper adds about 0.96 ms per game over its paired purchase-repair control in fresh confirmation (6.717 vs 5.759 ms, including observations and compiler). Do not infer speed from separate timing batches affected by host contention. All edits share the original 1,000 financial-turn budget.
- A correct local cash prediction can still miss a better future alternative. Earlier tomato holds block a more valuable wool wait; both policies' predicted final gains match. The combined replacement fixes that case, but broad economic value is inconclusive. Separate arithmetic accuracy, feasible work, and optimization quality.
- Source quantities and output chronology are explicit conditional inputs. Normalization parity and live confirmation answer different questions. The new public snapshot includes two newly selected submission IDs, but no independent-family claim or new notebook performance claim follows.

- Review 03:55: report intended treatment differences separately from failures. The seed-prebuy screen changes interim seed counts in 37 plans, as intended; no non-seed work, production, fault or ending-resource changes occur. This is a purchase-date experiment, not a sale-only exact-state comparison. Both positive and negative cash effects remain in all-case statistics.
- Counts of freed slots and advanced output can increase while cash decreases. The prebuy variant advances 1,702 output units versus 286 for its slot-fill control, yet lowers mean own cash. Measure actual margin and both balances, not activity.
- The numerical promotion audit uses the existing PROTOCOL.md thresholds, converting utility fractions to percentage points. Multiple-sale live confirmation passes all reported groups. Physical scenario compatibility and better plan selection remain separate, unpassed pipeline requirements.
- The 32 newly reserved public episodes now confirm the unchanged multiple-sale rule. New selected submission IDs include kanno 56133182 and pupen_o 56132319; they are not automatically new or independent code families.

- Review 04:15: check forecast rankings separately from eligibility and fallback. The cash-source ablation changes no numeric best-branch ranking, but changes 144 choices by removing a no-eligible-branch fallback. Actual fixed-branch results are identical across all 3,840 pairs. A selector gain need not mean its relative cash model improved.
- The original cash splice fails at least one rival world in 2,019/3,840 branch forecasts; preserving historical cash yields zero such failures on the used cohort. Both own forecast-failure counts are zero. Mean complete compile-plus-eight-world evaluation is about 4.4 ms per course in both variants.
- Numerical feasibility of a historical resource calendar is not public-state compatibility. The next audit tests necessary observed counts, placements, worker positions and land. Matching those summaries would still not prove compatibility of ages, services or private inventory.

## Review 22 — September 10, 04:35 UTC

Keep selector attribution separate from branch evaluation. The cash-source confirmation has identical actual outcomes for all 9,216 fixed branches and identical numeric rankings in all 3,072 decisions. Every changed choice comes from eligibility/fallback handling. A zero-failure forecast report is not a plan-selection improvement. Keep all 218 selected losses and the five negative opponent groups visible; source-conditioned scenarios remain unresolved.

The extended compatibility audit compares every public tile field, ordered workers, land and hiring state. It reproduces the earlier 372/1,280 noncash matches; adding cash leaves zero full public matches. Cash alone matches ten incompatible King cases. Compare the joint state, not independent marginal match counts. The evaluator may inspect truth, but this audit and the agent interfaces use legal public observations for compatibility.

Final temporal public confirmation keeps the same binary and all 34 newly acquired episodes before descriptive profiling. It records zero job failures, checked state changes or own cash prediction errors. The subsequent C++ trade profile takes 0.251 seconds for 38 episodes; acquisition, extraction and Python reporting are outside that time. Exact commands, source hashes and exposure times are retained.

The new public notebook has no saved outputs and was not executed. Its fallback fingerprint counts requested plants/land, unlike its observed hiring count. Do not mix those semantics. Treat self-play action hashes as observed behavior, not proof of adaptation or shared lineage; parse version components before ordering versions. Source-reading claims are separate from replicated experiments.

## Review 23 — September 10, 04:55 UTC

Test period boundaries by comparing one-pass evaluation with prefix plus resumed suffix. The new shared-state output passes 111,672 such comparisons, including day/shop boundaries, plus 55,836 prefix comparisons with recorded accounts and market inventory. Check resource insertion order, deficits, discards and cumulative commitment errors, not cash alone. The full 297-episode diagnostic takes 8.70 seconds including case loading and source certification; address/undefined-behavior sanitizers pass on two complete episodes.

Preserve benchmark controls when the return type changes. The old continuation header's hash matches its original build metadata. The longer paired median suggests about 1.2% added time for the existing caller, but individual pairs range from -0.4% to +15.7%. Report the spread and distinguish financial evaluation from loading and complete policy latency. No speed claim follows from these noisy runs.

Final document checks resolve every local link in the result/reuse/reproduction pages, verify headline source records and hashes, and confirm all 25 reserved final-refresh episodes remain unextracted. Exact commands and binary identities are retained. Do not reopen those actions before the next policy or integration is frozen.
