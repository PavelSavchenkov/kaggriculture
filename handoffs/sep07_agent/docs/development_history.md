# Development history and version status

This session produced a submitted shop-adaptive agent, stronger later local versions, useful league specialists, and a partial composition-search pipeline. It did not finish a general compiler that reliably turns arbitrary farm compositions into strong play. The initial [objective](../workspace/experiments/v6/sep07_compositions_v0/research/user_objective.md) remains the reference for continuation; the user redirected immediate work to this handoff during the fertilization experiments.

| Status at handoff | Agent/artifact | Meaning |
|---|---|---|
| **Actually submitted** | `shop_herd_s6_m3_g1`, submission 56074695 | Exact upload and validation in [submitted lineage](submitted_lineage.md) |
| **Latest validated broad local reference** | `candidates/investment_context_guarded_001_best` | Later animal selection plus 23 context-specific rebuilt days; not uploaded |
| **Validated complementary specialists** | `atakan_demand`, `atakan_integrated_s64_margin`, earlier ticket/species/sale variants | Useful counterstrategies and estimator probes; not overall replacements |
| **Unpromoted implementation work** | `runs/fertilization_001` through `fertilization_005` | Biology and route-rebinding work; best crop edit has a small one-opponent discovery gain, wider gates incomplete |
| **Uncompiled research proposals** | `research/crop_conversion_audit_001` | Dated fertilizer and wheat→tomato→carrot blocks with full observed inputs/outputs and lineage |

Raw exact-game JSON, rather than early prose summaries, is authoritative. Strict win rate is wins/games; win utility is `(wins + 0.5 * ties) / games`. Do not compare different seed pools as if they were paired experiments. Promotion gates use common seeds and seats, report own cash and margin separately, and retain lower-tail results.

## From generic composition search to executable source courses

The first phase built observation-only C++ opponents, a generic/typed arena, dated lifecycle extraction, cheap biology/economics, a greedy realization compiler, and exact diagnostics. The strongest teammate adapter matched 7,190 original-source actions. Thomas's public four-course router matched 8,628 and won 97.12% of 2,048 fresh games against that teammate. Older official and archived agents were inspected and tested; a newer public notebook title was never treated as strength evidence by itself.

Fresh top-player replays supplied full courses, per-tile lifetimes, successful services, market flows and day contracts. The first library had 72 courses and 51,768 checked actions; later refreshes reached 180 courses and 129,420 checked actions. The library's per-program imports preserve player, episode, seat, submission, global snapshot and replay hash. Replays were selected from current global leaders, including players who were not yet runnable league opponents.

The first real outer mutation/estimate/compile/exact loop evaluated 583 estimates, compiled 36 candidates and played 288 discovery games. It found no improvement over its best source. The initial greedy mixed farm was legal but weak; correct action shape did not ensure crops and animals were actually serviced or sold. Six independent cold families and maintained-count interfaces demonstrated construction, but did not match complete borrowed schedules. Dated biology matched isolated engine cases; source-supported complete-course margin ranking reached Spearman 0.881, while the approximate labor model degraded ranking. This established useful modules, not proof that counts and dates alone were sufficient.

See [objective coverage](../workspace/experiments/v6/sep07_compositions_v0/OBJECTIVE_COVERAGE.md), [ideas ledger](../workspace/experiments/v6/sep07_compositions_v0/IDEAS_LEDGER.md), and [profiling ledger](../workspace/experiments/v6/sep07_compositions_v0/PROFILING_LEDGER.md). Their chronological rows may be superseded by later checkpoints.

## Early routed policies and the useful day solver

Program 55 (`feeltheagi_55`) beat the public router in 2,048/2,048 fresh games but lost to older skomuro/Deniz/Arman policies. A public early-hiring observation selected between compatible source courses and produced `opening_router_v0`. Fresh leaders then exposed new counterstrategy cycles.

V30 became useful when given complete successful day contracts. An earlier sale with all worker routes rebuilt, `advance_sales_001_7`, won 95.46% of 2,048 fresh games against its parent; `opening_router_v1` retained it. Copying Mao85's complete day-0 market block restored production that the previous funding path had lost. Copying only the first one-unit trade difference, or all later markets, did not explain or reproduce the gain. Combining the useful opening and sale block produced v2.

Worker reduction produced v3, then physical starting-state guards produced v4. Several savings preserved biology and trades; some routes also changed later financing and opponent responses. v4 won 860/1,024 fresh games against v3, yet remained weak against new Junghoon/Binghua/John courses. This phase was not merely reducing the teammate's labor: these policies combined replay/public-controller branches, local market changes and new exact schedules. It also had not yet delivered the user's intended broad composition adaptation.

Species and purchase experiments retained narrower discoveries: Junghoon's wool sale timing won 968/1,024 fresh games against its source, and Binghua's traced cow→goose ticket won 732/1,024. Many whole-species substitutions and cold reconstructions failed through funding, transport, storage or service differences. See [component lineage](../workspace/experiments/v6/sep07_compositions_v0/LINEAGE.md) for exact source IDs and validation links for each intermediate agent.

## Justin source, terminal recovery, and the submitted shop rule

The 08:02 global refresh introduced Justin program150, a substantially stronger broad source course. Terminal recall/liquidation ideas from King/Dusta and Lynn were implemented over that course. The combined terminal variant improved exact games, but recall also removed four fertilizer collections; it was not an unchanged-output optimization.

Twenty-two locally solved V30 days formed `justin_guarded_hires_001_best`. In 32 paired profiles, hires fell by 22 and labor spending by $1,471, with equal per-tile/day service, production, trade quantities/timing and opponent cash. Failed unit actions fell 26→10; intraday action order and life start/end hours were not identical. On its fresh pool it beat the terminal parent 967/1,024 and the teammate 969/1,024. This is direct evidence that the day solver was useful.

The next strategic change implemented the user's milk/wool shop intuition at two day-7 cow purchases. When both switch, the observed production change is −54 milk/+48 wool; some old workforce guards stop matching, costing about $1,432 in the inspected changed case. The economic gain can still exceed that cost. The retained mode counts Yarn demand twice and keeps cows on ties.

On common fresh seeds 1100000–1100511, both seats, `shop_herd_s6_m3_g1` won 972/1,024 against the teammate versus its incumbent control's 961/1,024, with mean margin improvement +$1,309.13. Against King it won 725/1,024; against the public router 891/1,024. Against its own guarded parent it had **270 wins, 658 ties, 96 losses**, utility **58.4961%**, mean margin **+$1,581.72**. Earlier wording “599 wins” was incorrect: 599 is the win-equivalent count including half the ties. Mode 2 remains a conservative alternative; mode 3 is not better on every tail or opponent measure.

After frozen C++ and packed official-runtime verification, exactly this version was uploaded once. See [shop validation](../workspace/experiments/v6/sep07_compositions_v0/results/shop_herd_validation.json) and [submitted lineage](submitted_lineage.md).

## Later validated broad improvements, not submitted

`shop_herd_guarded_001_best` added 15 worker schedules for changed sheep branches. All 12 common-seed matchups improved or tied in mean margin and utility; teammate wins were 996/1,024 on its own fresh 1200000 pool. Paired profiles isolate the execution repair: changed games saved 15 hires/$1,322; biology matched all 64 profiles, while trades matched 62/64.

`animal_adaptive_r1_c0_b0` implemented a more general observed-state investment choice. It considers goose, cow, sheep and waiting, with entries compiled for days 11,15,18,21 in two herd contexts; day12 solver attempts remained UNKNOWN. Its valuation uses observed shops, current market and the public rival herd. It still changes only one additional planned investment, alongside the earlier two day7 choices. Waiting and later entry are operational, but the strongest tested setting buys immediately; no profitable waiting claim was established.

On fresh 1250000–1250511 both seats, the adaptive investment variant won 1,015/1,024 against the teammate and improved mean margin across 12 opponents. John strict wins fell 925→908 despite better mean/tail. These counts are not paired with the different seed pools above. [Animal investment validation](../workspace/experiments/v6/sep07_compositions_v0/results/animal_investment_validation.json) records the controls, native RNG audits, PASS objective and limits.

The latest broad reference, **`investment_context_guarded_001_best`**, adds 23 V30 days covering late sheep after either six or eight earlier cows. On the paired 1300000–1300511 pool, its parent-to-new wins were teammate **1000→1008**, King **742→767**, and public V5 **764→798**, each out of 1,024. All 14 same-opponent comparisons had nondecreasing mean margin and utility; none of those paired individual margins decreased. This does not mean direct self-parent play never loses: direct results were 346 wins, 622 ties, 56 losses. Against the submitted agent it had 417 wins, 554 ties, 53 losses and +$1,293.52 mean margin.

In 64 causal profiles versus the public router, biology, services, trades, timing and rival cash were equal; 16 changed games saved 11–12 hires/$1,076–$1,165. PASS J rose 151,983→152,324. Generic/typed/debug/thread checks and native256 audits passed. See [latest validation](../workspace/experiments/v6/sep07_compositions_v0/results/investment_context_validation.json).

## New public agents and independent Atakan portfolio

Public notebook audits continued alongside the main improvements. TTV1 exactly duplicated the existing terminal router. Market-Impact Router v4 was distinct but weak; its own-shed projection made the market overlay misleading, though not universally inert (5 wins, 28 ties, 95 losses versus Thomas in 128 games). TITAN/Kaito43 and BoatleeV29 were faithfully ported as controls; none replaced the broad incumbent. Source-exact parity and frozen licenses/attribution are in their league packages.

Thomas's new 93.8% public-state router was genuinely distinct and became `public_router_v5`. The port retains five tapes and the active shop-based selector literally; its notebook headline is not our measured league win rate, and its selector does not use rival state. It passed 10,073 original-source actions and operational checks. See [V5 validation](../workspace/experiments/v6/sep07_compositions_v0/results/public_router_v5_validation.json).

Repeated Atakan Aldemir replays supplied cow/sheep/goose courses (programs156/160/157) with a common 226-action prefix. Full exact states were retained; the goose donor differed in wheat stock, so action-prefix equality did not certify equal inventories. The local branching formula is inferred and labeled as such, not claimed as Atakan's private rule. The [replay audit](../workspace/experiments/v6/sep07_compositions_v0/research/animal_decision_replays/findings.md) and [portfolio package](../workspace/experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/README.md) retain donor IDs/hashes and fixed-source controls. Correct display name: **Atakan Aldemir**; the frozen portfolio README's “Atakan Mamedov” is a naming error.

On 320 common discovery contexts against five opponents, the demand selector won 221 games versus 163 for the strongest fixed goose course, with +$4,702 mean-margin gain. The original margin estimator selected worse than the simpler demand rule in several contexts. Oracle ablations and legal future-shop sampling then improved it: on fresh 1,450,000-series seeds, six opponents ×512 games, sampled64 margin won **2,028/3,072** versus legacy **1,981**, mean margin **+$658**. Demand still won 2,058 games and remains complementary. Neither specialist replaced the broad reference. [Sampled report and checks](../workspace/experiments/v6/sep07_compositions_v0/runs/atakan_sampled_shops_001/README.md) preserve all controls and frozen sources.

## Checkpointed failures and crop work

Earlier purchase compilation and 20 legal future-shop valuation variants did not produce a new broad incumbent. Full-mean and sampled estimates improved some weak early models, but still lost substantially to the current reference. One early control initially changed an inactive patch; the corrected control reproduced 384/384 parent cash outcomes before conclusions were retained.

The crop model matched all **239/239** source lifecycle harvest totals and generated **414** positive fertilizer estimates in 54–77 microseconds. The first exact edits lost later guarded schedules or discarded their extra wheat. Rebinding later routes repaired the worker loss; preserving original strawberry sale requests avoided moving existing sales. The best corrected discovery edit added two sold berries in 46/64 activated games, mean own cash +$56.78 and margin +$46.34, unchanged labor. Wider validation was incomplete when the handoff request arrived. It is **unpromoted**. See [fertilization checkpoint](../workspace/experiments/v6/sep07_compositions_v0/results/fertilization_checkpoint_1302.json) and [review35](../workspace/experiments/v6/sep07_compositions_v0/research/review_35.md).

The separate [crop audit checkpoint](../workspace/experiments/v6/sep07_compositions_v0/research/crop_conversion_audit_001/README.md) freezes 36 donor-game comparisons, four exact service/rotation proposals, cash/material balances and all source hashes. It corrects an aggregate impression: the major fertilizer gap is wheat/carrot, while local strawberries already have 90.85% productive coverage. Its larger Mengfei wheat→tomato→carrot suffix is a proposal, not an implemented improvement.
