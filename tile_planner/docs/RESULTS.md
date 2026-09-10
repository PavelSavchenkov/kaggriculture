# Tile placement: results and decision

Research window: September 10, 2026, 00:10:28–08:10:28 UTC. Completed; the exact finish time is recorded in STATUS.json. Machine-readable paired results and source hashes are in [RESULTS.json](../evidence/RESULTS.json); the full positive/negative ledger is [LEARNINGS.md](LEARNINGS.md).

The useful reusable result is the C++ lifetime-placement interface, complete-course compiler, exact estimate caches and strictly checked schedule reuse. Placement changes can help particular compositions, but the new search does **not** meet the declared general quality gate against spending the same time on the existing layout. Keep the placement proposals available for offline experiments; do not make them the default improvement path on the evidence here. [GATE_DECISION.md](GATE_DECISION.md) records each original gate's outcome and the remaining measurement limits.

## The separable problem

The outer composition search supplies species, counts, dates, service requirements, land purchases and financial commitments. Placement assigns legal cells to complete lifetimes. Already placed animals and growing crops stay fixed; future lives can use released cells. A candidate must preserve the supplied biology and required orders across the whole continuation.

The output is a cell assignment, exact daily work contracts, a complete executable course, a delivered/discarded resource calendar and measured hire/cash outcomes. With fixed trades, minimize actual season hire cost. With the same financial policy evaluating changed calendars, compare final cash or margin too. At every compute limit, retain a complete verified incumbent. A solver timeout is UNKNOWN; an unfinished prefix is not a cheap solution.

[DESIGN.md](DESIGN.md), [PROTOCOL.md](PROTOCOL.md), [API.md](API.md) and [HANDOFF.md](HANDOFF.md) specify the contract, objective, promotion thresholds and integration boundary. The supplied whole-course CLI starts at day zero. The C++ daily compiler accepts an actual dawn state; a general mid-game suffix CLI is not implemented.

## What the source review established

The work started with the sealed September 7 compositions archive and the root PDF. The copied [prior assessment](../../experiments/v6/sep10_tiles_placement/evidence/assessment.md) records why existing small placement experiments did not already establish a broad win. For example, the earlier owned-first improvement also delayed investment, and a large archived wheat saving came from unchanged-layout scheduling.

This session inspected 48 development replay courses from six teams, 16 reserved courses from four more teams and a final eight courses from four reserved team IDs. These are **courses, not independent strategy families**. Public code and worker-command comparisons show extensive tape sharing. Terry, cooked and pensukesan in the final set are heavily related to public tapes. The two Blurry/Unknown Mother-Goose courses differ substantially from that family; the name change is tied to stable team ID 16730612.

Broad tile roles are stable within seasons: in the final profiles, about 94–95% of used tiles keep one crop-versus-animal role throughout a sampled season, while only about 21–33% keep one exact species. These fractions are averaged over two games per team; they do not measure cross-game agreement. Animals sit much closer to shed access than crops: mean radius about 1.6–1.7 versus 4.5–4.7. These are observed patterns, not evidence that those positions are optimal.

The inspected public router uses fixed work tapes with branches based on currently revealed shops and inventory, plus execution/sales repairs. Newer public source also changes workforce and some investment. Replays alone cannot establish whether a different agent computes placement online. No relevant public source was found for Unknown Mother-Goose, so its mechanism remains unknown.

A separate [cross-game comparison](../../experiments/v6/sep10_tiles_placement/research/cross_game_roles_001.json) compares the same day and cell, excluding cells empty in both games. The final Terry pair has identical role calendars; cooked has 97.4% exact-species agreement, pensukesan 93.6%, and Unknown Mother-Goose 66.4%. Unknown Mother-Goose still has 96.6% crop-versus-animal agreement where both games have an occupant. Occupancy and timing differences count as disagreement in the first metric. These dependent pair comparisons describe behavior; they cannot identify the placement algorithm.

Quadrants are useful proposal blocks, but they do not make four independent labor problems. In the final profiles, 24–30% of productive worker-days serve multiple quadrants. All accepted quadrant interventions were evaluated with shared workers and resources globally. Source details are in [the review manifest](../../experiments/v6/sep10_tiles_placement/research/source_review/MANIFEST.json), [family audit](../../experiments/v6/sep10_tiles_placement/research/source_review/FINAL_TAPE_FAMILY_AUDIT.json) and [final placement profiles](../../experiments/v6/sep10_tiles_placement/research/final_source_profiles/SUMMARY.json).

## Placement quality: gains exist, broad transfer failed

The central comparison gives both methods the same complete starting course and total wall budget. Both can refine workforce and reuse newly found schedules. Search additionally proposes legal placement changes. Any useful new route is offered back to the unchanged layout before attributing a gain to placement.

| Panel, 180 seconds per method | Search wins / ties / losses versus fixed layout | Scope |
|---|---:|---|
| Six synthetic development compositions | 1 / 5 / 0 | One expanding farm gains 1,803 cash; this is development evidence |
| Five complete replay-derived development incumbents | 0 / 4 / 1 | Search gains 288 on binghua; fixed refinement gains 432 |
| Six supported frozen final inputs | 0 / 6 / 0 | Neither method improves its complete starting incumbent |
| Two distinct-family inputs, separate format extension | 0 / 0 / 2 | Fixed gains 650 and 89; search gains 362 and zero |

The 362 in the last row comes from unchanged-layout refinement. The changed-layout search spends time compiling proposals that fail, time out or do not improve the accepted result. All bounded runs in these panels retain a complete verified course. The final method supports six of eight selected biological inputs; the two original rejections remain in its coverage denominator.

A separate later diagnostic increases the day-query budget to thirty seconds for fourteen previously unresolved proposals and seven matching controls. Only one candidate and its control become feasible. Both full courses verify, but the changed crop chain costs 2,671 versus 2,404 for the original layout, with new witnesses shared. This is additional bounded negative evidence, not a replacement for the frozen test or a proof of optimality.

The [tool-time breakdown](../../experiments/v6/sep10_tiles_placement/research/search_time_001.json) explains why faster proposals alone do not solve this. Proposal generation uses about 0.5% of tool time on the six frozen final searches; changed- and unchanged-layout day queries use about 97%. On the two separate distinct-family searches, changed full-course compilation uses about 83%. These are observed allocations of a bounded implementation, not lower bounds on the problem's cost.

There is real development headroom. On an expanding mixed farm, complete interventions reduce hire cost from 1,876 to 1,643 for a crop-chain move, 1,664 for two animal swaps or a quadrant-count change, and 1,698 for an animal move. A different animal move improves the target day but increases the season bill to 2,147. The quadrant change saves 212 hiring coins yet loses 731 final cash in the original shared-shop scenario. Delivery and shed overflow explain why identical biological output does not imply identical saleable stock.

A warm equal-radius cow/sheep swap saves 199 on a DeeperNet course and transfers positively to all eight sampled DeeperNet courses after matched refinement. On eight related mtmr_s1 courses it wins once, ties twice and loses five times. The reserved swap selectors also perform poorly on complete matched continuations. These results reject a universal cow-before-sheep distance rule.

Weighted radial assignment, service/input/output weights, hiring-threshold weights, donor matching, population crossover, quadrant blocks, local swaps, annealing, equal-radius moves, dated future-life moves and crop-chain interventions were explored. Strong simple source/nearest/animals-first placements remain necessary controls. Estimated gains frequently fail exact compilation. Preserving every existing productive action's hour is too restrictive: a fast geometric screen admits no nontrivial swap out of 900 on each of six representatives. [SEARCH_COVERAGE.md](SEARCH_COVERAGE.md) maps the tested ideas and remaining gaps.

## Demand rules: intervention first, selector second

The experiment first certified same-composition interventions, then tested frozen rules using only shops revealed before the changed animals were placed. Two binary examples on one expanding composition used eight development and eight reserved shop sequences.

The early rule selects one swap on day 4 when revealed milk demand is positive. It gains 852.375 mean own cash on the reserved sequences, compared with 535.25 for always swapping. The later rule selects another swap on day 10 when milk demand is at least two: 494.75 versus 482.5. All sixteen actual runtime decisions match the chosen complete branch and pass the financial calendar check.

Neither is promoted. Both eight-scenario bootstrap intervals include zero, the early rule still selects a loss, and there is no independent composition-family result. Both swaps move a cow **farther** from the shed. Local service interactions and retained deliveries matter more than a simple “more milk demand means closer cow” rule in these examples. See [conditional runs](../../experiments/v6/sep10_tiles_placement/runs/conditional_live_001) and [the live intervention panel](../../experiments/v6/sep10_tiles_placement/runs/probe_live_001).

## Speed improvements that preserve the checked results

| Change | Checked workload | Before → after | Meaning |
|---|---|---:|---|
| Cache unchanged lifetime days | 4,608 assignments, 18 synthetic inputs | 10.05 → 5.33 seconds | Every prediction bit-identical |
| Same cache with fixed real finance | 1,536 assignments, six development inputs | 11.40 → 8.71 seconds | Every prediction bit-identical |
| Same cache on supported final inputs | 1,536 assignments, six inputs | 10.06 → 6.86 seconds | Every prediction bit-identical |
| Same cache on repeated-service extension | 512 assignments, two inputs | 2.86 → 2.20 seconds | Every prediction bit-identical |
| Packed versus text schedule bank | 18 paired full courses, 36 runs | Mean 0.691 → 0.057 seconds | Same bill and both players' cash; all complete |

The packed-bank comparison includes loading, compilation, full engine execution and artifact writing. Its separately run financial check also passes all 36 courses. The 269-entry bank occupies 422,757 bytes. A larger 1,212-entry development bank occupies 3,977,383 bytes and loads in 0.044 rather than 2.415 seconds; all 1,440 validation lookups match. Prior certificate construction is excluded from these reuse timings and must be charged separately by the caller.

A separate three-repeat check measures packing and validation from existing certificates. Four banks with 9–1,212 entries recover that measured packing cost after roughly two or three repeated loads. A single fresh pack-and-use does not pay for itself here. This calculation still excludes discovering the source schedules; [PROFILING.md](PROFILING.md) gives the timing scopes and per-bank values.

Caching is scoped to an immutable input program and finance context. A packed schedule is only a proposal until it strictly replays against the requested day. The serializer passes malformed/truncated input checks and a byte-identical round trip. These are scoped reuse improvements, not evidence that a new unseen layout can be solved in 57 milliseconds.

The real small-bank follow-up shows the limit of the large-bank speedup: packed format alone saves about 16% of total compile-plus-verification time on eight supplied real incumbents. The loader now also compiles each identical lifetime specification once. Fresh-oracle comparisons agree on all 15,715 lifetimes and 471,450 daily records from 75 plans. In 120 interleaved real-course runs, old text-bank loading versus the new loader plus packed bank gives mean total time 0.13190 versus 0.10655 seconds, with every paired action and outcome identical. This is 19.2% less full wall time, and 23.1% less compilation CPU excluding the separate calendar check. See [PROFILING.md](PROFILING.md) for scope, timing differences and memory.

Brief refinement is a separate quality/time tradeoff. On twelve new synthetic inputs, 0.1-second refinement of reused days matches or improves each full-self-refinement bill, with total 1,806 versus 1,826 and 167.53 versus 334.07 process seconds. On six real development inputs, two-second and nine-second caps each complete five, but the two-second cap trades about 60% less aggregate CPU for a 4.6% larger common complete bill. Smaller caps complete only four. The [five-budget curve](../evidence/quality_time.png) and [per-team table](PROFILING.md) show that most of the loss is concentrated in one case. Keep a stronger complete incumbent instead of replacing it merely because a brief run returned.

## Repairs and contract corrections

Idle-worker preclearing recovers 280 of 384 injected weed faults completely, versus 242 with queue/PASS absorption. The corrected rules leave all 48 clean controls valid. This improves recovery coverage, but it still has four fixed-shop cash losses against queue-only and negative mean native cash change. Changing action timing can change later native randomness and shops.

Emergency-sale rules recover some money faults, but even the strongest tested forecast guard has substantial losses. Its fixed-shop mean gain is about 72, while the worst loss is 3,762; native mean gain is negative and the worst loss exceeds 21,000. No general cash-shortage rule is promoted. A repaired local action must be judged by the full continuation.

Several boundary fixes are necessary for meaningful comparisons:

- Preserve ordered sell-before-buy stock requirements, not only net hourly withdrawals. The adapter passes 70,981 enumerated checks.
- Preserve repeated same-day biological services explicitly. Version four imports both formerly rejected final inputs exactly; its results are reported separately from the frozen method.
- Encode missing item arguments as invalid no-ops when converting replays. A corrected final trace passes full parity; original traces and reports remain preserved.
- Read recorded rival no-ops without weakening own-certificate validation. The separately corrected calendar checks all 719 transitions of the affected course.
- Report a future resource deficit as an incomplete result with day, item and missing quantity. A saved Terry prefix reproduces a two-carrot shortage on day 29 with zero solver queries.

These fixes do not turn incomplete plans into successes. Within-day overripe decay is still outside the lifetime format. Fixed replay trades can become unfunded with changed delivery timing or another opponent. [UPSTREAM_ISSUES.md](../../experiments/v6/sep10_tiles_placement/evidence/UPSTREAM_ISSUES.md) records the exact boundaries and local fixes.

## What to retain in the pipeline

Use the dated lifetime input, exact daily compiler, packed strict-replay bank, day cache, complete-incumbent retention and delivered/discarded calendar as the reusable component. Keep source/nearest/animals-first assignments and strong fixed-layout refinement in every comparison. Use placement search where a composition has demonstrated complete-course headroom, then measure whether it beats fixed-layout work at the same total budget.

Do not claim the 5% held-out placement-quality gate passed, promote the two demand rules, or enable emergency sales as a general repair. The next useful engineering work is to improve certification of changed layouts across all affected later days and to represent intraday supply/decay commitments explicitly. Fitting more price-distance rules before fixing those measured failure modes has weak support here.

The [compact source archive](../../experiments/v6/sep10_tiles_placement/artifacts/placement_component_source.tar.gz) builds in a fresh directory using only the four declared persistent root packages. Its example and bounded optimizer pass the [clean-build checks](../../experiments/v6/sep10_tiles_placement/artifacts/CLEAN_BUILD_CHECK.json). The [delivery audit](../../experiments/v6/sep10_tiles_placement/artifacts/DELIVERY_AUDIT.json) checks frozen comparison tools, archive contents, current core code, result evidence and runtime dependency hashes. No agent submission or promotion into a persistent package is part of this result.
