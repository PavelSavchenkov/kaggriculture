# What worked, what failed, and where to continue

The user's main ideas were productive: shop-dependent compositions improved exact league play; full replay schedules provided strong starting points; the day solver turned useful changes into real cash; and estimated-versus-realized comparisons exposed specific modeling and compilation errors. The largest unfinished part is reliable construction of arbitrary multi-animal/multi-crop strategies. The latest validated reference still relies heavily on complete donor courses and locally rebuilt branches. Version status and exact results are in [development history](development_history.md).

## Composition is a useful search representation, with execution attached

Dated product/count/lifetime descriptions compactly represent biology, capital and potential output. Biology was fast and testable: the initial 72-profile pass averaged 3.86 microseconds per profile; later target-animal estimates were about 2.05 microseconds per alternative; all 239 tested crop lifecycle harvest totals matched exactly. Recorded service/support improved early full-course margin ranking to Spearman 0.881.

Those successes do not establish that dates and counts alone determine strength. The first greedy compiler emitted legal actions while losing production, and 583 estimated proposals/36 exact candidates found no improvement. Missing pickup/deposit capacity, dated feed, seed funding, hire thresholds and route choices can dominate a nominally good composition. Preserve both requested composition and exact realized lives/outputs; report unmatched or immature tails rather than silently dropping them.

## Shop adaptation worked; opponent effects require the whole market

The submitted two-purchase rule is direct evidence for the user's intuition: more revealed wool demand can make sheep preferable to cows. Its changed branches alter output, not just labor cost. The later general selector adds goose/cow/sheep/wait and public rival-herd valuation, but only at one additional compiled investment opportunity. Profitable waiting, general multi-investment search, arbitrary placement and deeper game-tree search remain open.

Prices couple both farms. More own output can lower revenue from the rest of the herd and change rival revenue. Always measure own cash, rival cash and margin separately. A productive service can be economically poor; a lower own-cash branch can improve relative margin. Conversely, a local margin forecast can select the wrong branch when its future demand assumptions distort prices.

One bounded Atakan case makes the latter precise. Against public V5, seed1028, goose rather than cow was forecast to change own cash by +$6,563 and rival cash by +$3,005, predicting +$3,558 margin. Exact play gave +$3,020 own, +$16,715 rival, and **−$13,695 margin**. Rival actions, quantities, animal lifetimes and fixed costs were unchanged across the compared branches. Its milk revenue difference was +$15,483 versus only +$2,949 forecast. Forecast milk quantity was 258 versus 249 actual; there was no initial held milk or new future cow explaining the gap. Here the dominant failure was the price trajectory, not missed rival expansion. This is a bounded witness, not a universal classification of rival-model error.

## Oracle diagnosis led to a deployable specialist improvement

The [offline ablation](../workspace/experiments/v6/sep07_compositions_v0/runs/atakan_oracle_ablation_001/README.md) used 320 existing contexts ×three exact fixed continuations. These oracle inputs were diagnostic only and never entered a candidate agent. Regret is the exact margin lost relative to the best of those three branches.

| Estimator information replacement | Correct best branch /320 | Mean branch regret |
|---|---:|---:|
| Original legal model | 202 | $3,249 |
| Exact future rival trades only | 215 | $2,860 |
| Exact future own trades only | 200 | $3,300 |
| Exact future shops only | 288 | $475 |
| Exact rival trades and shops | 300 | $225 |
| Exact own/rival trades and shops | 313 | $15.44 |
| Above plus exact fixed costs/calendar | 314 | $11.46 |

The shop oracle mixes unavailable knowledge with correction of the old 35% unknown-demand discount; it does not by itself prove a legal predictor can recover the whole gain. It nevertheless identified the next useful experiment. Observation-only deterministic 8/32/64 stratified shop continuations replaced that discount without seeing the environment seed or actual future shops. The Atakan sampled64 margin specialist gained 47 strict wins/3,072 and +$658 mean margin on a fresh six-opponent audit. Its full three-branch evaluation averaged about 292 microseconds. Full-mean demand won the discovery comparison but reversed on fresh strict wins, so it remains a control.

The same sampling design did not improve the main single-animal incumbent. On its 20-variant, 10,240-game screen, the best late own-profit variants tied incumbent utility but lost about $30–43 mean margin. A better forecast component is not automatically a better policy across families, objectives and execution patterns. Preserve paired controls and test the whole agent.

## The day solver was useful within complete contracts

The clearest fixed-composition result saved 22 hires/$1,471 with equal per-tile/day biology, output, trade quantities/timing and rival cash. Later context-specific schedules saved another 11–12 hires/$1,076–$1,165 in changed games after a general animal choice. Earlier compiled sale timing also beat its parent in 95.46% of 2,048 fresh games. These are measured contributions from exact scheduling.

V30 does not own the complete economic problem. Its physical contract alone does not certify future cash, shed capacity, a later sale or a different opponent response. UNKNOWN within its budget is not a proof of infeasibility. A route can make an oversized market request execute more successfully and thereby change spending. Extract accepted quantities, retain order slots, keep economic trades separate from physical same-hour net flows, and replay the full continuation.

Any composition edit invalidates all dependent purchases, transfers, services, deposits, sales and later day guards. A successful extra fertilizer action initially disabled three later optimized worker days; the extra berries were insufficient to pay for that loss. Route rebinding restored workforce efficiency. Clearing all berries instead of preserving source requests changed existing sale timing and reduced value. Exact production, exact liquidation and exact profit are three separate checks.

## Replay learning should recover reusable behavior, not invent intent

Keep the original episode, seat, submission, snapshot, raw hash, normalized actions and dated actual flows for every donor. A strong fixed replay is evidence of one successful course, not the full donor agent. The Atakan cow/sheep/goose examples share 226 actions, but their exact input states differ; a replay prefix does not certify equal wheat, weeds, seeds or cash. Locally inferred rules must remain labeled as hypotheses.

Public notebook audits need the active entry point and exact payload hashes. TTV1 was a duplicate; Market-v4 was distinct but weaker; the newer Thomas V5 was useful; TITAN and Boatlee ports improved coverage without replacing the incumbent. Claims such as “93.8% win rate” belong to the notebook's own benchmark. Original-source parity proves fidelity in the checked observations, not competitive strength or universal action equivalence.

Strong players sometimes omit feed/care, allow animal escape, or retain structures after an exit. These are useful patterns to test. A skipped purchase, failed order or escape does not prove deliberate waiting or private optimization. Full productive service remains a reasonable default that local biology/economic search may override; it is neither mandatory nor disposable merely because exceptions exist.

## Crop gaps and next concrete work

The [crop audit](../workspace/experiments/v6/sep07_compositions_v0/research/crop_conversion_audit_001/README.md) distinguishes crop mix from within-crop conversion. Local strawberry productive fertilizer coverage is already 90.85%, while wheat and carrot have none. Local output per seed is 3.166 wheat and 2.645 carrot; several selected donors reach roughly 4.2 wheat and 3.4–3.6 carrot. Local melon already reaches its cap without fertilizer. Increasing an aggregate fertilization percentage is not an objective by itself.

Four frozen proposals include strawberry completion, wheat/carrot conversions, and Mengfei's wheat→tomato→carrot cell suffix. The larger suffix exchanges 13 wheat for eight tomatoes and one carrot, reduces planting/watering work, and needs extra fertilizer plus replacement feed and tomato sales. Its donor-quote shadow value is −$53 before labor and market effects, so copy it only as a branch to evaluate. Two smaller templates have positive shadow values, not verified marginal profit. Full observed farm accounting reconciles donor cash and fertilizer balances and separates harvested production from bought/resold stock.

Resume from `investment_context_guarded_001_best`. First finish the pending crop edit's full multi-opponent, native RNG and operational gates; keep it unpromoted until then. Next rebuild storage/deposit/sales for extra wheat and evaluate a complete tomato rotation with its funding/feed dependencies. Continue the general composition agenda in parallel with these bounded probes: larger family insertion/deletion, repeated rotations, dynamic placement, multiple investment opportunities, and compatible suffix reuse. Feed exact errors back into whole-farm quantities, workforce context and market forecasts before increasing search depth.

## Evaluation and reproducibility habits to preserve

Use CPU C++ for the cheap estimators and exact game batches. No GPU workload was needed for these experiments; the unrelated training job was left intact. Measure a prospective workload before adding training infrastructure.

Keep a league containing teammates, current public controllers, globally strong replay courses, specialists and prior versions. Opponent cycles were real; wins against one favored opponent did not establish broad strength. Preserve unused seeds for final audits and avoid drawing causal claims from different seed pools. Report strict wins, ties, mean margin, lower tail and PASS J (`0.8 mean + 0.2 lower-CVaR10`) explicitly.

Source parity, debug action validation, complete PASS/self games, independent episode state, generic/typed equality, thread determinism and native official RNG checks caught different failure modes. The exact packed submission also needed complete official-environment games against the original teammate, not just the unbundled C++ win rate. Frozen source and artifact hashes make the final answer reviewable and prevent later edits from silently changing what was actually uploaded.
