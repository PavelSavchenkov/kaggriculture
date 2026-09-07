# Profiling ledger

- Read the rules, strategy-search guide, experiment pipeline and local-agent API.
- Engine documents exact parity with official 1.32.7 and cheap C++ transitions.
- Persistent day solver V30 reports median 243 ms/day on exposed development
  inputs. Measure it for finalists; using it for every estimator node would be
  costly. Its contract ignores shed capacity, cash and sales; full replay is
  still required.
- External C++ agents have provenance/reuse metadata. The user's later explicit
  clarification authorizes component reuse in this experiment. Record exact
  origins, modifications and license information in `LINEAGE.md`.
- RTX 5090 detected: 32,607 MiB VRAM. It was already at 89% utilization at setup;
  inspect shared workloads before choosing training batch size.
- Replay analysis may use Python outside the policy/search hot path. Exact
  successful trade and service events are necessary: requested orders alone
  inflate production and labor diagnostics.

- Replay JSON sorts worker-inventory keys. Rebuilding each turn from those
  dictionaries loses insertion order and can misattribute shed overflow. Carry
  original order from prior actions; this fixed the $22 validation witness.
- No GPU need is demonstrated. Four-thread C++ 256-game batches ran in 0.084–
  0.986 seconds for the first tested opponent pairs. Profile before adding
  infrastructure. Pure-Python heredoc stdin through `conda run` did not execute;
  use saved scripts or `python -c` for reliable analysis commands.
- Baseline generic release and specialized debug pair produced identical full
  result rows across 16 PASS cases, despite different thread counts. Agent
  objects are reused per match worker and reset per episode.
- Teammate source parity now passes 7,190 actions, including both forced routes,
  against the original Python/ctypes shared library. Replay JSON omits shared
  step in seat 1; hydrate it from the recorded transition index as live runners
  do. Without hydration the reference incorrectly repeats turn zero.
- Public four-route port matches 8,628 actions, all four routes. Immutable table
  initialization and per-instance route state permit concurrent evaluation.
- Biological profiles for 72 observed compositions, with/without fertilizer,
  averaged 3.86 microseconds each across 100 repetitions. This measures biology,
  inputs and field operations, not complete economic valuation or scheduling.
- 128 exact micro-games validate per-day produced quantities and successful
  service/input counts. Default crop service stops after the last productive
  day. The engine decays surplus gradually after expiration; hour-zero salvage
  or deliberately later harvest needs the intraday model and is not covered by
  this first biology component.
- Exact zero invalid actions can coexist with poor production. The first
  compiler misses productive services and composition dates while emitting
  legal actions. Compare per-life output/service and milestones, not just faults.
- Separate paired cash from paired margin: eliminating waste can also remove
  beneficial price support, or change opponent behavior. Preserve both metrics.
- Same-turn wheat buy/sell events exposed inconsistent current/planned herd
  reserves. An unrelated $60 hiring reserve then hid cheap affordable hands.
  Fix dependencies and ablate each cause before attributing failure to borrowed
  service practices. Full diagnostic evidence is in research/review_06.md.
- Compare requested, attempted and realized compositions. Program116 has no
  realized geese in its source trace, yet transforming attempted orders changes
  other games. A zero source-Life difference is insufficient for deduplication.
- Causal species profiles must include opponent cash and product prices. The
  Junghoon goose-to-cow edit adds221owncash and removes499rivalcash on32paired
  cases; labor and most biological outputs are exactly unchanged.
- Source parity uses original Python/native on exogenous observations only.
  Forced mirror, route, financing and tie cases activate otherwise dormant layers;
  Finance7 matches all34512actions before gameplay comparisons.
- King RC4 contains inactive legacy layers. Trace the final entry point and
  rebinding graph before translating every historical definition. Its active
  Thomas source is byte-identical to an existing verified port; forced fixtures
  exercise all four continuations and rare repairs. All14,380 actions match.
- Check both biological intent and exact realization after service deletion.
  A ten-melon schedule predicts60 units but realizes38 with redundant fertilizer;
  deleting that dependency restores60. Zero unit faults did not reveal the gap.
- Terminal recall changes production: Justin loses four fertilizer collections.
  Additional realized sales still improve cash. Keep market-only, recall-only
  and combined controls so this is not reported as unchanged or greater output.


## 10:51 checkpoint

Review29: originalgoal reread10:46; generalcow/sheep/goose/wait with future reconsideration is main priority. Newlocalbest shop_herd_guarded_001_best promoted:996/1024teammate; all12paired fresh matchups nondecrease. Evidence results/shop_herd_guarded_validation.json. In14/64changed profiles15fewerhires/$1322saved/13fewerfaults,biology64/64equal,trades62/64equal. Official56074695COMPLETE; no anotherupload. PublicTITAN/Market-v4ports checked but weak. All evaljobsfinished; parallelpublicagent completinglineage. Review29fullglobalinvariants; next11:10. Fresh1250000+,final900000untouched. Goaluntil00:47tomorrow.


## 11:13 checkpoint

Review30complete,originalgoalreread11:11. Generalanimal+waittypedpolicy/valuationimplemented;24checkedentryplans across2herdcontexts,days11/15/18/21all3species. Day12UNKNOWN. Fixed/adaptivefullgamescreen building(session86561). Currentbestunchanged; noadditionalupload. ClearAtakan3speciesbranchwithidenticalprefixandcrop-to-structure reuse found; donorartifacts beingretainedbyparallelagent. Freshmetadata1112started. Nextreview11:33.


## 11:27 update

Mainvalidatedsearchreference: runs/animal_investments_001/proposals/animal_adaptive_r1_c0_b0. Usergeneralanimalideaimplementedasgoose/cow/sheep/waitwithfuturepurchase,recedinghorizonobservedshops/market/publicrivalherdvaluation. Oneadditionalday11investment pluspreviousday7shopchoices; arbitrarymany-animalconstructionstillopen. Fresh1250000..1250511both:1015/1024teammatewins, meanmarginall12opponentsimproves. Equalgroupwinutility85.27%→88.00%; Johnwins925→908despitebettermean/tail. Native256andPASSJimprove; allrequiredchecks pass. See results/animal_investment_validation.json. Waitingoperationalincludinglaterday21buy,butbestsettingdoesnotwaitinthissample. CPUestimate2.05us/alternative, marginrank.60–.64. Most9-productmismatchesare+2strawberryfromrestoredfertinputs; targetanimalbiologyalmostexact. Labourguardslostafternewspecies;11newV30daysnowchecked,combinationsearchnext. Review30next11:33; freshglobal1112analyzed18games,newThomas93.8notebookfullportinparallel. ExplicitAtakan3wayportfolioandplacementdonorsinresearch/animal_decision_replays. NoadditionalKaggleupload.

## 11:52 — worker dependency closure promoted

Current local reference: candidates/investment_context_guarded_001_best.
It adds 23 checked V30 days to animal_adaptive_r1_c0_b0, selected by actual
physical starting state for late sheep after either six or eight earlier cows.
Fresh 1300000..1300511, both seats, 1,024 games per 14 opponents: teammate
1000→1008 wins, King 742→767, public v5 764→798. All 14 mean margins and win
utilities nondecrease; no same-opponent game loses margin. Existing equal-group
utility 87.40%→88.44%. Native audits improve; PASS J and required operational
checks pass. Evidence: results/investment_context_validation.json.
Causal 64-game public comparison: biology, services, production, buys/sales,
timing, discards and rival actions/cash all unchanged. Sixteen changed games
save 11–12 hires, $1,076–$1,165, and 11–14 failed unit actions. The multi-context
runtime retains 2,304 full old single-context records exactly.
This closes a measured execution cost of general animal selection; it does not
establish profitable waiting or arbitrary multi-investment construction. Next:
Atakan complete three-species suffix portfolio in parallel; expand opportunity
compiler and improve full-farm flow/labor estimates. No additional submission.
Next review 11:55; fresh global metadata about 12:13. Final seed pool 900000 unused.

Review33 completed12:19: original objective reread, full82global metrics from new1212cohort, corrected early cow control384/384cash parity but no new early-choice improvement; Atakan demand/margin specialists validated; nonlinear future-shop/price error is next diagnostic priority. Details research/review_33.md. Next review12:39.

## 2026-09-07 12:45 UTC — Review34

See research/review_34.md for all82global/local metrics and original-goal coverage. Root early-animal and sampled-value screens produced no new incumbent; retain investment_context_guarded_001_best. Atakan sampled64 fresh specialist audit improves legacy47wins/3072, native checks pending. Prioritize productive crop fertilization with full supply/routing/harvest/sale dependency checks; preserve general animal selection including wait and larger composition edits. Next review13:05.

## 2026-09-07 13:02 UTC — Crop output and route closure

See results/fertilization_checkpoint_1302.json. All239source crop life harvest totals match the cheap biology model;414extra-fertilizer proposals generated in54–77µs. Single-day edits lose later optimized routes. Rebinding source routes restores labor; preserve original trade timing because sell-all altered pre-existing berry sales. Top corrected day20strawberry edit adds2sold berries in46/64activated games, mean cash+$56.78andmargin+$46.34with unchangedlabor. Wider league pending. Wheat extra harvest is currently discarded, requiring storage/deposit/sale rebuilding. Replay audit clarifies local strawberries already90.85%productive fertilizer coverage; wheat/carrot account for major within-crop gap. Explore a complete Mengfei wheat→tomato→carrot block next.
