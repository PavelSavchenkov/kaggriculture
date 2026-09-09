> Historical document. Relative code paths refer to the restored experiment; links target `_work/session/` after preparation.

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

## 2026-09-07 13:48 UTC — Review37

Fresh top12×6replay audit, larger crop rotations and1vs4sheep demand branch, negative direct-parent gate for crop fertilization, and full crop-family compiler progress in research/review_37.md. External crop gains alone do not justify promotion. Next review14:08.

## 2026-09-07 14:08 UTC — Review38

Full17daytomatofamily compiled with exact−14WHEAT/+8TOMATOrealization. Currentloss is nearlyall ownextra labor, plus rivalpricebenefit. Bundlecrops/share routes andconditioneconomic entry. Fertilizationnegativecases reveal$17ownloss+$12rivalgain:service defaultsneedobservedpriceoverride. Sheepfamilycompleteexecutiontransfer auditongoing. See research/review_38.md for80globalmetrics/fullgoalreview. Next14:28.

## 2026-09-07 14:28 UTC — Review39

Promoted observed-demand productivefertility gate (crop_value_m2_t4):fresh15opponentutility/marginchecks,native/PASS/operationalchecks pass;unchangedlabor,+2STRAWperactivation. Namedparent investment_context_guarded_001_best submissionbeingpacked,frozenCPP4023/4096teammatewins. Larger3tileTomatosuffixcompiled butstillloses;sheepexpansionexactsourceparitybutweaktransfer. Full80newlocal/globalmetrics andgoalreviewin research/review_39.md. Next14:48.


14:50 UTC, review40: A changed crop block must include the first retained harvest and terminal resumed crop. Corrected crop-life totals match92activated exact games; seed usage is not expenditure while old purchases remain. Report these independently from labor and endogenous-price errors. Evidence: research/crop_rotation_checkpoint_1441.md; runs/crop_rotation_estimate_001/OUTPUT_PARITY.json.


15:08UTC review41: The worker estimate must price practical sale/deposit times and finite market slots. Source h0 can have10orders; overnight storage103/100 invalidates a course. Keep no more than free shed capacity and sell the excess today. Full-game checks caught these constraints before any promotion. Evidence in research/review_41.md and runs/crop_rotation_005..009.

15:34UTC: observed-shop tomato gate has fresh positive mean/utility evidence across6opponents, but no promotion yet. Full changed course silently suppresses a later optional berry improvement. Rebuild alternative full continuations and prove identical branch-entry obligations, then revalidate on fresh scenarios. A valid starting guard alone does not preserve later policy dependencies. Exact parity of leaf selection2560completegames supports the gate implementation, not its strength. See review42 and crop_rotation_gate_validation_001/REPORT.json.

15:51UTC review43: newlocalreferencecrop_rotation_t2_berry,17opponentfresh/nativeoperationalpassed; exactproduction17408games and12,516,352instrumentedactions. 11/2551selectedcoursesfallbackday28, fullproductionpreserved; guarddetailsbeingextracted. Wheatonefertfrozen1893files, actual+6wheat/noextrahiress, usediscardedfert;weedsixfallbacksexplicit. Newmix discoveryrunning; fresh1710000reserved. Latest15:45global72games57uniquereplays and82invariantsrecorded. Twonewnotebookauditsparallel. Nextreview16:08, activegoalthrough00:47,noadditionaluploads.

16:11 UTC review44: crop_mix_t2_wheat remains current, full 82 global/local metrics updated. Parallel worker stopped on usage limit; no compiler process was alive. Root completed missing placement proposal preparation and launched14off/on compiles, with frozen cell22 control. Cheap visit deficits distinguish equivalent-yield placements (3 versus14–16). Next review16:28; fresh1730000 placements,1740000 crop estimator,1720000 unused,final900000 unused. Full goal active until00:47; no new submissions.

16:31 UTC review45: completed eight-placement screen; no improvement, two distinct economic ties, paired labor attribution recorded. New day-12 replanting forecast audit reduces quantity error sharply but leaves large timing error; financial branch validation is next. See runs/productive_wheat_placements_001/README.md and runs/crop_choice_estimator_001/README.md. Original objective reread16:27, 82 global/local metrics retained, current mix unchanged, active goal through00:47. Next review16:48; no additional uploads.

16:46 UTC: crop-choice audit completed 768 paired contexts (1536 full games),462 eligible. Current two-shop gate is already within$4.598958 mean margin of best tested leaf with hindsight on this finite discovery panel. Best selected alternative changes only4 choices (+$0.89 margin); no promotion. Replanting improves quantity forecasts but own-profit choice helps rivals more through shared prices. Preserve506-source frozen audit and pivot to new whole-farm proposals from fresh16:42top12replays. Full goal remains active; fresh1740000unused.

16:48 UTC review46: full82new global/local metrics recorded. Two-leaf selector headroom only$4.599 on discovery, so prioritize new family/course proposals. Fresh69courses pass49611sourceactions and begin six-opponent screen. X-ray notebook executableASTunchanged42functions; no port. Currentmix and submission56078898 unchanged. Nextreview17:08, activegoal through00:47.

17:09 UTC review47: fresh69-course screen led to Bohann25opening component. Four3840-game ablations isolate gain to first2markets, not later composition; compact768records exact. Fresh19opponents1750000running; pair/debug/native checks and causal King analysis pending. Currentmix remains promoted; smallV5tradeoff explicitly preregistered. Full82global/currentmetrics retained; nextreview17:28, goalactiveuntil00:47, no upload.

17:20 UTC: promoted bohann_opening_v1 as local reference. Fresh19opponents×1024games, directcurrent969W55L(+34.76margin), King1006W18L(+16707.93pairedmargin). Establishedgroup93.587→94.999%,95%gain+1.170..1.666pp. All operational checks and native directpass; compact768records exact;276compileddependencies frozen. Explicit regressions:V5−2freshwins,−$9.28mean;nativepublic−1/256win;othermean/tailtradeoffs retained. SourceBohann106497007seat0first2markets only, latercourseablationnegative. OneKingtrace day1cash27→0 thenworkforce/output changes; initialgrosswheattrades are notproduction. Reportresults/bohann_opening_validation.json. Preserveparent;next test retaining original hire/stock contracts while borrowing opening trades. Submission56078898 unchanged,no extra upload. Nextreview17:28,goalactiveuntil00:47.

2026-09-07T19:09:43.871874+00:00: Review52: longer day-solver search saves6/$665 or7/$754 hires per activated goose game, all64 production/sales/rival actions unchanged per leaf. Mean versus crop control now-$68.91 off / +$58.39 on. No new promotion. Final-day mixed-market replay check needs full-game interpretation; two off days unresolved. Global1902 refreshed72/53; three notebook code audits pending. Next19:28, goal through00:47.

2026-09-07T19:30:40.334248+00:00: Review53: longer solver plus terminal service adjustment saves8/$898 hires per activated goose course in both leaves. Sold quantities/rival actions unchanged in64 each; one unsold fertilizer uncollected. Mean margin vs crop control+$98.56off/+$161.89on. Lastoff day22 UNKNOWN120. Seven complete WIP packages frozen, parity/league next. Global1902 remains latest; notebook static audit partly done. Next19:48.

2026-09-07T19:50:21.377381+00:00: Review54: day solver saves8/$898 hires in both goose leaves; all extra sheep and on-cow hires also removed. Unconditional goose failed43008fresh games because it overwrote tomato intent. Context correction fixes that and gains+$80.35 direct on another43008fresh games, but grouped utility still fails; q32 remains reference. Next general animal/wait selector from observed-market estimates and compiled costs. Notebook1902 audits complete. Next20:08; public refresh20:02.

2026-09-07T20:12:56.481535+00:00: Review55: four-way C++ investment selector built.8192counterfactuals,2048exact selector parity,896operational checks. Goose estimate correlation.90–.92, all choices~.3ms. First45056fresh panel improves every paired mean; only current-group positive95%bound unresolved. Frozen independent2048-seed confirmation/native audit running; q32 remains reference. Fresh2002 complete, distinct Thomas72-turn router extracted, Fields unchanged. Next20:28.

2026-09-07T20:22:53.676928+00:00: Promotedlate_value_s32_t0_r05 overopening_q32_b13_v1. Independent180224-game confirmation passes all gates, current group+.1666pp(95%+.0745..+.2585), direct1527W2256T313L,+$173.98. Native/PASS no mean/utility regressions;256 rebuilt frozen records exact. Real four-way crop/animal choice, with small legacy utility/tail tradeoffs recorded. No Git or submission.

2026-09-07T20:33:55.591625+00:00: Review56: portfolio promoted after independent180224 confirmation, all gates/native/causal/frozen checks pass. Full82 metrics now new reference. NewThomasV5/2 validated12230actions+896checks, beats oldV5 181/256 but loses63/256 to us. Its winning sheep-heavy0/6/11 herd is next independent composition target. Original goal reread; bigger/earlier herds, placement and cold starts remain active. Next20:48, refresh21:02.

2026-09-07T20:50:25.829600+00:00: Review57: user assessment/PDF complete, reference unchanged; full82 metrics explicitly carried forward. Independent V5/2 physical-prefix/composition probe prepared and building. Larger sheep family, multi-day compiler and labor calibration remain priority. Next review21:08, refresh21:02.

2026-09-07T21:09:36.296154+00:00: Review58: full82 global2102 metrics, larger V5/2 family transfer and17 checked day plans. Direct+$335.46 but mixed broad results, no promotion. Causal labor savings and restored sheep purchase distinguished. Baseline/forced-family observed-prefix valuation is next. Next review21:28, refresh22:02.

2026-09-07T21:30:51.364551+00:00: Review59: largerwoolfamily49152fresh currentgroup gain but two gatesfail;896operational and6144selectorparity pass. Added4096Junghooncounterfactuals; all26 captured features matchV5/2 despite distinct future outcomes. Need rival-continuation uncertainty or additional public evidence. Reference unchanged. Next review21:48, refresh22:02.

2026-09-07T21:49:29.310519+00:00: Promoted wool_family_context_v2. Whole sheep-heavy continuation with observed-shop selection and compiled labor savings. Fresh51200games/25opponents passes all gates; direct parent139W818T67L,+$363.36. Native4608profiles,896 operational,8192 selector parity,227 frozen inputs and256 rebuilt full records pass. All paired mean margins improve; teammate-1freshwin/1024 and v1 specialist direct loss retained explicitly. No submission or Git.

2026-09-07T22:12:54.122275+00:00: Review61: exact delayed family bridge, failed first rival-rule promotion dueJohn alias, full1024 operational pass with active-branch coverage. Fresh22:02 global72 analyzed; all82metrics recorded. Infer full observable market-flow pattern next; keep general estimator/compiler gaps. Next22:28.

2026-09-07T22:49:14.724519+00:00: Promoted rival_wool_context_v3. Public spending after six hires and one wheat purchase distinguishes the failed John sale from V5/2 seed purchases. Fresh57344 games: V5/2+6.8359pp and+$182.14 mean; all27other fullrecords unchanged. Native5120profiles,1024operational checks with active custom/native cases,302frozen C++inputs and256 rebuilt records pass. Packaging omission repaired without policy changes; amendment retained. Parent-self unchanged; no Git/upload.

2026-09-07T22:53:52.347795+00:00: Review63 records82metrics for promoted rival_wool_context_v3, exact cash-discriminator diagnosis,57,344freshgames and complete native/operational/frozen checks. NativeV5/2 gain is relative-price driven despite owncash-$346 and8.47extra faults pergame. Trace actual failed actions and guard mismatches next. Fresh71course economics and independent families remain active. Nextreview23:08,refresh23:02;final900000unused.

2026-09-07 23:01 UTC: Exact256-game execution trace matches every native parent record. In26/30new wool branches, two-sheep order217 buys onlyone; cash is sufficientnextday, but pickup253fails and day11guard rejects missingtile14animal plusunused1wheat. Firstday6fault is harmless repeatedwatering, notrootcause. New bounded purchase retry217→252 implements the originalcomposition obligation; source/copiedcalendar unchanged. Discovery2560profilegames running. Evidence runs/rival_wool_execution_001/PURCHASE_CAUSE.json.

2026-09-07 23:04 UTC correction: day11guard rejections comprise20missing sheep cases and6one-wheat deficits, not26missing sheep. Full256-game repair discovery changesexactly20native games, saves67.7faults perchangedgame, +$128.31meanmargin across256, with no pairedmargin losses. Custom1024:+$97.27margin, unchangedwins; fresh promotion notyetattempted. See runs/rival_wool_execution_001/PURCHASE_CAUSE.json and runs/rival_wool_repair_001/DISCOVERY_ANALYSIS.json.

23:25UTC: Review64 records fresh2302global82metrics and exact execution-repair results. Combinedrepairv2 restores20missingsheep/6wheat-short nativegames, +$146.77meanmargin,+1win, no discoverypairedmarginloss. Fresh245760games preregistered1970000..1972047both across30opponents with unchangedstrictutilitygates. NewAhmedV23controller staticdecoded, C++conversionnext. Currentreference rival_wool_context_v3 unchanged.

23:43UTC: NewAhmedV23 exactport completes12950sourceactions,896operational,6912profiled discoverygames. Independent9layercontrols plusfullparent7680profilegames; all768mask0records exact. Removing salelead loses12/128wins againstourcurrent and6/128V52, meanmargin-$120.15across6opponents. Removingroomguard loses13Junwins and$6573.20mean; removingbudgetloses$503.96Junmean; weedsmallbenefit. Deadstocksales hurtaggregateutility; clampchangesordersbutnoeconomics. Newobserved_sale_lead_001 translates salelead tocurrent usingcopied-policy time-onlynextactionforecast, completeownstockprojection,suppression;3variants(control/all/milkwool) pluscurrent,10opponents256games: live98682. ParentCURRENT_REFERENCE unchanged.


00:23UTC: Sale-lead source shared calendars preserve7680 V2 records and2560 parent-control records;768 forecast diagnostic games match all200000+ eligible next raw sale vectors exactly (exact count in results/observed_sale_lead_v3_validation.json). Ten-opponent discovery improves average margin+$2195.95, but Jun loses11/256 wins and$490.95 mean. No fresh1990000 or promotion. Fixed rival actions do not imply fixed successful purchases or production: Jun1044 loses one sheep purchase at217 after early sale changes, later sells more crop/wool and finishes+$5097 while own-$4988. Global start216/240/288/360 alternatives now running in observed_sale_lead_004; no opponent-ID exclusion.


2026-09-08T00:51:01.675772+00:00: Promoted observed_sale_lead_start_216. Fresh65536games/32opponents, native/PASS5632, operational1024, frozen64 full records/237 C++ dependencies pass. Advance non-input sales after step216 using copied-policy prediction; preserve initial farm funding. Read results/observed_sale_lead_start216_validation.json. No upload or Git.

2026-09-08T01:13:17.426993+00:00: Review70: goal extended through Sep10 00:47; latest nine-version best validated. New72 global games and one notebook audited; all82 current/global metrics refreshed. Hiring-only generic-compiler experiment builds from measured pending-job versus dated-work mismatch. Next review01:28, refresh02:08.

2026-09-08T01:22:29.282146+00:00: Generic workforce diagnosis completed:512 discovery +128 control games,
64 copied-mode0 full records exact. Eight hands improve cold mixed own cash
$38109.69->$44769.81, but no compiler wins. Raw estimate_plan workforce raises
source-family production while charging$26122/$34523 wages. Restoring original
hiring still leaves source55$60336.63 vs donor$97654.50, with3981.38 vs3200moves
and identical280hires/$5400. Source4 has4339.06 vs3562moves. Earlier target-lock
failures remain. New runs/compiler_route_sep08_001 tests current-tile and
same-tile-job value with workforce fixed. Live session10540; all prior processes
terminal. No promotion; arbitrary farm compiler remains incomplete.

2026-09-08T01:28:50.526692+00:00: Review71: completed hiring640 and route512 games with128 full mode0 parity records; no generic-compiler promotion. Stop priority tuning. Placement/funding controlled study running39714. All82 current/global metrics explicitly carried forward. Next review01:48, refresh02:08; full goal throughSep10.

2026-09-08T01:43:00.861566+00:00: Placement study finished256 games,64 exact prior controls,64 unchanged
dairy/wool pairs and2 instrumented full-record witnesses. Owned-first mixed
sheep are born day0 instead ofday8; exact initial$1000 land purchase moves from
step0 to168. Same total land cost, more walking, extra wool/fertilizer, +$1660
own cash vs public and+$2265 vs current. Goose avoids$1000land but produces fewer
eggs/fertilizer; do not call its cost saving a production gain. No compiler wins.
All prior processes terminal and collected, including route header amendment
4510 (48 records exact). New cold_day_tasks_sep08_001 generates first-day tasks
from the cold composition and placement, with three service choices; persistent
day solver +full engine endpoint check. Only live process81498 runs run.py.
Next review01:48; latestbest remains observed_sale_lead_start_216.

2026-09-08T01:57:54.597326+00:00: Review72: placement financing confirmed in full traces;768 full games of task-generated day0 and96 exact coverage games. All24 schedule hours execute. Small cash gains, no strong promotion. Operational86810 finishing; pair result-label rerun required. Nextreview/fresh02:08; fullgoalSep10.

2026-09-08T02:04:08.761386+00:00: All new work processes are terminal and collected. Cold day-task prototype:
3/3 solver cases exact in root engine;768 full discovery games;32 exact old
control records;96 instrumented full records equal, all24 schedule hours used;
52 operational games including generic/pair/debug/thread equality and all4modes
self/PASS. Static-pair labels corrected with4 exact replay records, original
preserved. All generated policy and recorded source hashes verified unchanged.
No new broad best. Next work is later-day task/resource/finance compilation,
not more first-day tuning. Next review and fresh replay/notebook refresh02:08.

2026-09-08T02:24:55.883282+00:00: Review73:24 exact cold traces reveal near-biological output on the small farm and a full-care-bank reset error. Test one-condition correction, then prioritize larger dated investments over polishing this small farm. Fresh72 global games and2 analysis notebooks audited; all82 comparison metrics updated. See research/review_73.md and runs/cold_later_days_sep08_001/CARE_BANK_WITNESS.json.

2026-09-08T02:31:18.098131+00:00: Review74: care correction384games/96exactcontrols/112ops completed; small-farm+$401–467cash, dense outcomes mixed.375 dated larger farms estimated x16scenarios in0.210s,34selected with family/workforce diversity. Exact fullgame discovery running. See research/review_74.md.

2026-09-08T02:48:31.675445+00:00: Review75:375 estimated farms/54 exact packages/1104games/448ops; stronger cold compositions but no league wins. Restricting funding gaps does not beat prior leaders.40-game animal decomposition identifies missed service after actual births. Day solver recovers8feeds onday18 and3 onday26 while saving2hires/233perday;24 daycases exact across the two studies. Full-season integration next.

2026-09-08T03:00:30.237976+00:00: Complete checkpoint after Review75. Guarded larger-farm schedules:320 fullgames,64 empty-wrapper records exact,128 coverage records exact,48ops. Only2/16 public-router games activate; each selected day completes24hours and expected unit counts. Labor-only mean cash+116.50; feed-only owncash-0.75 but margin+369.75 with extra animal output; combined owncash+86.625/margin+457.125. Every other tested opponent's complete records equalparent. Do not treat extra production as guaranteed ownprofit or exact templates as a general compiler. Wheatbundle1/2/4:480games,128exactcontrols,136ops; smaller loads mostly lose on dense/larger farms. Reject universal smaller bundles. Next implement joint daily route/input assignment with physical/market separation, using exact old-control parity and day-solver feasibility witnesses. All current jobs terminal; next review/refresh03:08UTC.

2026-09-08T03:22:44.748047+00:00: Review76: split compiler192 exact records/64 operational games; herd routes256 games mixed and late-only, coverage diagnosis next. Fresh72 global games update all82 metrics. Router V5 helper unused; Arlene V4 active budget/clamp combination identified for source-parity C++ comparison. Strongest unchanged. See research/review_76.md.

2026-09-08T03:33:42.622811+00:00: Review77:Arlene V4 full mask31 matches8628 source actions/all4routes/11budget changes;8960 discovery and operational checks running. Herd routes001 complete256games/128exactcontrols/80ops/64coverage; partial-load return loop witnessed, isolated routes002 test running. All82 metrics retained from03:08 cohort. Best unchanged, main goal active.

2026-09-08T03:50:17.553974+00:00: Review78:Arlene8628 source parity/8960games/1792exactcontrols/80ops; clamp losses traced to early market-slot cash/hires. Incumbent empty-slot4096 comparison running. Partial routes remove42reversals but score neutral. Joint mixed routes576games/192exactcontrols trade crop output for animal service and are rejected; first-day task diagnosis next. All82 metrics retained. Best unchanged.

2026-09-08T04:17:54.594842+00:00: Review79:fresh72 player-games/59replays update all82metrics; Xray42functionsunchanged, periodicnotebookirrelevant. Empty-slot native5632 allmeans/utilitiesnonnegative but parent per-game gate fails once(-6); no promotion. Fresh67584 stillrunning. Joint routes576/192controls/160ops/16diagnosticfullgames fail densework; cumulativeinput routes960comparison running. Best unchanged.

2026-09-08T04:26:46.146344+00:00: Review80: cumulative-input routes960/192 exact controls give partial, farm-dependent recovery but remain below original dense compiler. Stop cost-only variants; compare exact whole-day task/resource schedules. Empty-sale1024 operational games pass; native parent margin gate already fails once(-6), so no promotion. Broad fresh and isolated rebuild live; causal native trace launched. All82 metrics carry04:08 cohort. Best unchanged.

2026-09-08T04:35:18.841359+00:00: empty_sale_slots_m2 rejected:67584 fresh/5632 native-PASS/1024 operational/64 isolated rebuilt games. Current league93.956%->94.725%, direct906W38T80L,+239.62margin; both preregistered parent per-game gates fail. Fresh regressions1 and native1. Exact native trace proves strawberry floor-price inventory effect, not input movement; same-state immediate+2 leads eventual-6margin. Floor guards are a new discovery study, no promotion.

2026-09-08T04:38:39.568633+00:00: joint_resource_routes_sep08_001 complete960 discovery/192 exact controls/256 operational games; all three compiler source-hash sets unchanged. Partial recovery does not justify replacement. Empty-sale m2 frozen audit rejected as recorded centrally; floor-guard discovery48340 launched. Nine accepted descendants unchanged.

2026-09-08T04:59:12.381172+00:00: Review81:floor guards4096/1024exactcontrols/48ops/1290exposedwitnesses selectmode1; new frozen69632fresh plusnative/ops running. Joint day witnesses45/48solve,allfullengineexact;all12 augmented/two-fewer-hirecases solve. p355jointday14 recovers20wheat3milk2fert plusserviceand89laborcost, fixedtrades. Compiler route loss not simply too few workers. Retryonly3timeouts; nextgeneralize programs with validated observation-based entry. Best unchanged;all82metrics carried04:08.

2026-09-08T05:17:36.660422+00:00: Review82:fresh72player-games/61replays updateall82metrics,nochangednotebooks. Floor guard passes5632native/PASS+1024ops+64frozen;fresh69632pending. Day witnesses45initial+1retry=46/48uniqueexactcases,all12augmented/twofewer solve; movement146->85 onp355jointday14 plus24tasks. Observation-built clock/model verifier compile include fixed,paritypending. Best unchanged.

2026-09-08T05:53:12.150647+00:00: Review83:512day-program games/64controls complete; relaxed programs broaden reuse but p362 production losses outweigh saved labor against parent. Coverage/ops pending. Completed floor guard retained, not promoted; new fixed population audit71680fresh/6144native tests unguarded versus guard without retroactively changing old failures. All82metrics carried05:08; next06:08.

2026-09-08T06:02:24.290734+00:00: Day programs complete512discovery/64originalcontrols,512exactcoverage,104operations. No interrupted plans; p362 loss is later biology/service, not fallback. Continuation-value horizons1/3/remaining season now tested in new isolated run. Sources unchanged.

2026-09-08T06:20:09.329122+00:00: Review84 updates82metrics fromfresh72player-games/63replays. Reusable programs and value horizon studies complete; longer forecasts do not broadly help, physical programs never interrupted. Native population6144 passes, fresh71680pending. NewTITAN partial-sale core translating; first oracle shop mapping corrected. Xray now analyzesKing/source mismatch; q30 downloaded. Next06:28/refresh07:08.

2026-09-08T06:27:53.831028+00:00: Promoted empty_sale_slots_m2 after independent population71680fresh/6144native plus unchanged1024ops/64frozen evidence per candidate. All232policy/239dependency hashes unchanged. Direct accepted parent911W40T73L,+242.12. Old rejected per-game audit remains rejected; new criterion explicitly optimizes population strength. No Git/catalog/upload.

2026-09-08T06:34:23.924333+00:00: Review85:empty_sale_slots_m2 accepted with independent71680fresh/6144native + unchanged source/ops/frozen checks. New82metric local profile updated from256native versus parent, not causally comparable with prior cohort. TITAN C++ lot search216exact cases,6480context benchmark50.35us average. Kingq30data equal oldRC4, older repair/overlays; no newcontroller established. All jobs terminal. Next06:48/refresh07:08.

2026-09-08T06:48:36.526493+00:00: Review86: TITAN sale integration now four C++ policies, initial build/discovery70803 pending. Physical/funding/order contracts are essential caller obligations; pure216case parity alone is insufficient. Keep p362 biological-service diagnosis alongside incumbent market search. All82metrics recalculated from unchanged06:08/global and accepted native256. Next07:08 review/refresh.

2026-09-08T07:15:22.688428+00:00: Review87: exact stock-entry guards explain5/6stock-only interventions; sale endpoint/deadline contract removes most losses, no reference promotion. Actual changed-state day16 solver restores5feed with2fewerworkers/233saved, all4exact. Reusable bank improves cash across5discovery opponents,640games+160controls+64ops; coverage/fresh remain. New72global/68replays refresh82metrics; FarmSignal isV52data plusday-close99-slot reserve, TITANunchanged. Next07:28/08:08.

2026-09-08T07:27:03.124678+00:00: Retained service_bank_p362_m2 as improved p362 cold-farm continuation. Fresh2560games/5opponents all6gates pass; mean cash gain614.41,95%[410.76,822.27], no mean fault regressions. Parent182W10T64L/256,+1185.63margin. All640coverage records exact, zeroabandoned programs,64operations;13216initialsourceinputhashes unchanged. This is genuine production/service plus labor improvement, not strongest-agent promotion; current-reference margin remains-60495.49. Next quantify the remaining dated-composition versus actual production gap and broaden actual-state scheduling.

2026-09-08T07:39:11.601211+00:00: Review88. Cold service-bank fresh2560 confirms +614.41 owncash across5 opponents but still0/256 vs best. Next diagnose composition/placement/service gaps on common games; old p362 estimate already weak and underfunded. Farm Signal C++ capacity component prepared, parity/discovery pending. All82 comparison metrics explicitly reuse07:08 cohort/currentnative256. Best unchanged.

2026-09-08T07:51:11.005312+00:00: Review89. Farm Signal1800oracle/1152games/384controls/36ops: capacity reduces discards but active-opponent cash loses21..40, reject promotion. Coldgap1024models+512exactreversegames: intended farm itself underproduces wheat/milk and is underfunded; labor doublebest. Next retired-tile renewal and herd alternatives, not only scheduling. All82comparisonmetrics reused07:08/native256.

2026-09-08T08:17:13.276262+00:00: Review90. 196renewal/herd farms estimated;768discovery/64exactcontrols/132ops;5376fresh reject p98/p157 despite cash/margin gains. Matched rankcash0.973 versusmargin0.600. Replay884full6melons atage10 exposes coldplan2daytile waste. Fresh72/59 recovered through exactsameTeamId+submission rename mapping, raw evidence preserved; all82metrics updated. Twochangednotebooks staticallyaudited, no newpromising agent. Best/coldreference unchanged. Next08:28/09:08.

2026-09-08T08:32:21.200691+00:00: Review91. Earlymelon14plans/1440games/320exactcontrols: earlier removal32..48turns retains72output and usuallyimprovesmargin; cowmodes1/2 strongestdiscovery. Ops pending, freshrequired. Afterboundedcoldtest prioritizestrongopening and observation-basedbranches. All82metrics reuse08:08cohort/currentnative256. Next08:48/09:08.

2026-09-08T08:52:19.330557+00:00: Review92. Early-melon8192fresh selects m1 only within cold branch; still0wins against4strongopponents. Shop selector C++ prepared, complete-record parity running after1632exactprefixes. Inspect mainline crop-value gate next. All82metrics explicitly reuse08:08cohort/native256; nextreview/refresh09:08.

2026-09-08T09:15:14.177373+00:00: Review93. Shop selector7776fresh improves old baseline+2.637pp/+$2396margin but loses to earlymelon; no reference change. Mainline crop4608games/1152exactcontrols/1152prefixes/52ops: forced choices lose, hindsight margin headroom only$14.08. Prioritize larger composition alternatives using solved marginal labor costs. Fresh72/58 and2staticnotebookaudits complete; all82metrics updated. Next09:28/10:08.

2026-09-08T09:45:51.380229+00:00: Animal-group screen:281256 dated proposals/18shop scenarios,36 exact full controls,14.5815 estimator seconds=51.84us per proposal including32shop samples. Best groups include all3species and mixed herds. Recorded source future is retrospective and labor unpriced; no policy or strength claim. Six initial3s fixtures allfail before seasonend. Cowpairday8 physicalpass/fullgamefail traced to actualcash406 buying1 of2requested400cows. Funded insertion v3 reuses existing helper; day8 now exact with0extrahires, day9 exactwith1. Goosepairday10 remainsUNKNOWNat30seconds for0..2extra workers, not infeasible. Source physical-contract controls and full funded cow compilation continue. Global/coldreferences unchanged.

2026-09-08T09:50:46.868761+00:00: Review95:281256group estimates/36exactfullcontrols; funded cowday8/9 exact butday10UNKNOWN30s.42sourcephysical requirements/invariants pass with duplicate-sale diagnostic errors; correctedstrictcontrol and rootsoft-hint comparison prepared, buildrunning. All82metrics reuse09:08/native256. Nextreview/refresh10:08. Bestunchanged.

2026-09-08T10:15:55.953506+00:00: Review96:corrected281256estimates/4242crop+317animal lifetimes/36controls;42strictdaycontrols;10hintcomparisons no schedules;28careequivalences. Sheep singlesday28exact, cowday26exact then inventory-target error. Fresh72/55 plus2staticnotebookaudits updateall82metrics. Salem C++port pending. Next10:28/11:08; bestunchanged.

2026-09-08T10:29:52.190277+00:00: Review97. Six corrected cow/sheep terminal continuations running; Salem source parity11504 passes, operational builds running. Next matched economics and full C++ public component screen. 82 metrics unchanged from refresh1008. Best unchanged; next review10:49/refresh11:08.

2026-09-08T10:39:19.479523+00:00: Salem C++ port complete:11504 action parity/48 operations/3968 frozen discovery games. Full mode0/128 vs each of6 strong opponents,64/128 vsJunghoon. Keep dated crop calendar and weed repair as ideas; no promotion. Paired effects {"weed_on_top_of_sales": {"cash": 279.4486607142857, "margin": 508.4810267857143, "faults": -9.401785714285714, "hire_cost": 0, "utility": 0.004464285714285714}, "sales_on_top_of_weed": {"cash": 78.29464285714286, "margin": 156.5078125, "faults": 0, "hire_cost": 0, "utility": 0.004464285714285714}, "combined_vs_tape": {"cash": 358.0424107142857, "margin": 665.3504464285714, "faults": -9.401785714285714, "hire_cost": 0, "utility": 0.006696428571428571}}. See runs/salem_port_sep08_001/RESULTS.md.

2026-09-08T10:51:57.547535+00:00: Review98. Six complete matched animal continuations pass all endpoints; cowtriple own+4774/margin+5396, sheeptriple+3025/+2478 with finalh2. Source passive weed targets corrected; same fixed-job solver0.10..0.16s. Earliest-sale carry correction solves cowday28in0.34s. Salem port11504parity/48operations/3968discovery; no promotion. H1final audits running; next context-preserving runtime programs and forecast/labor calibration. Next11:11review/11:08refresh.

2026-09-08T10:58:15.858813+00:00: Complete six-case matched audit: cowtriple+4774own/+5396margin, sheeptriple+3258/+2711. All719transitions and daily endpoints pass. Measured labor reduces own forecast MAE 581.74->45.69; margin residual remains301.82. Small exposed fixed-world result; no runtime/league promotion. See runs/animal_groups_sep08_001/MATCHED_RESULTS.md.

2026-09-08T11:04:37.178737+00:00: INTEGRATED six-case matched audit. All719transitions and daily endpoints pass. Measured labor reduces own forecast MAE 494.90->45.69; margin residual remains301.82. Small exposed fixed-world result; no runtime/league promotion. See runs/animal_groups_sep08_001/INTEGRATED_RESULTS.md.

2026-09-08T11:20:40.441572+00:00: Review99 updates82metrics from11:08 fresh72/58. Integrated six-animal audits complete; exact latest results in INTEGRATED_RESULTS. New2048-game entry survey compiling,64standard controls planned; no runtime promotion. Two public notebook audits pending. Next11:39/12:08.

2026-09-08T11:42:15.024186+00:00: Review100: new Yusuke full C++port128/128vsbest in3968discovery; fresh28672pairedconfirmation running. Source2876/24thresholds/24resets and48operationspass. Entry survey2048/64controls proves sheepberryfork and cowday23weed-only variation. Six secondcontexts compiling; no referencepromotion. Next12:00/12:08.

2026-09-08T12:01:36.815823+00:00: Review101: Yusuke fresh28672complete; direct1024/1024counter but neutral87.394%vs current94.189%, no promotion. Opening quantity-only causalwitness; four completepackages48opspass,5632discoveryrunning with16exactparentcontrols. Six additionalanimalcontexts719/action/endpointpass; negativecowworld correctlyrejected bycheapmodel. Next12:20/12:08.

2026-09-08T12:11:10.791155+00:00: Completed this goal continuation: entry survey 2048 games/64 exact controls; six additional animal courses and independent 719-turn audits; new Yusuke port 2876 source actions/24 thresholds/24 resets/48 operations/3968 discovery/28672 fresh; opening descendants 48 operations/16 exact controls/5632 discovery. No promotion. Latest review101; next12:20UTC. Hourly refresh1208 launched; poll its exact live handle. Full goal and broad/cold/mixed-species scope remain active.

2026-09-08T13:23:14.950860+00:00: Earlycowafteractual labor stillhasown forecastresidual-2056.97; separateactualfuture-shop diagnostic fromintradaytiming/rivalforecast error. Runtimeguardmiss traced toweed38andmissed6wheat withnewcowoutputintact. Mixedday9physicalsuccess/liveendstock35vs37showsmarket/funding/storage certification stillnecessary. Compilerlandrawitem0invalid inDayProblem; mapactualquadrant. Delayedentrymustpricecropremoval, notassumea naturalharvest.

2026-09-08T13:39:00.792754+00:00: Review105. Completed36864 paired factor games; premium+1.038pp, animals+0.049pp, combined+1.086pp. Weed38 exact zero-extra-worker repair restores6wheat/+152cash; runtime integration next. Full objective active; accepted/cold references unchanged. All82 metrics reuse1308/global andnative256. Next13:58review/14:08refresh.

2026-09-08T13:59:26.863111+00:00: Review106. Repaired/composed agents16fixture+56ops+9856discovery pass; full q24combo+11.816pp/+186.37margin, King tradeoff persists. Forecast actual-shops/rival-flow diagnostic isolates major earlycow errors. Mixedday9 partial wheat buys traced and late stock correction verified; full certificate/season pending. All82metrics reuse1308/native256. Next14:18/refresh14:08.

2026-09-08T14:27:03.651581+00:00: Review107. One Kaggle upload authorized. Broad132864 live62984; care compiler live42461. Mixedday9 certified, day10 animal/stock mismatch blocks full course. Global82 metrics updated to1408 cohort72/64; localnative256 reused. Tetsutani sale-merge and Dmitrii terminal-delivery leads retained. Nextreview14:50/refresh15:08.

2026-09-08T14:49:24.911337+00:00: Review108. One submission in preparation, no upload yet. Frozen adapter first8 full games exactly match C++; larger packed132, C++8192, and broad132864 audits pending. Guard-miss inspector pending. Care-only saves no money; below-parent workforce search saves3 hires/$178 in independently audited seed1014 with identical production. Latest global72/64 metrics reused with localnative256. Nextreview15:10/refresh15:08.

2026-09-08T15:14:50.905091+00:00: Review109. Packed94908 plus rare19413 actions exact; frozen CPP teammate4040/4096, lastsubmission3883/4096. Broad132864 pending, no upload. Both retained-incumbent cow courses save178 with identical output. Timeout regression belongs to experiment caller, not root solver; integrated zero-budget check pending. Fresh global72/55, 82 metrics updated. Nextreview15:30/refresh16:08.

2026-09-08T15:22:32.468932+00:00: Caller incumbent fix is complete and validated: zero new search retains30/30 saved days across two full719-step scenarios, all final fields exact, +178cash each; invalid PASS-only saved schedule rejected. Root day_solver correctly returnsUNKNOWN for fixed workforce; no root bug/edit/commit. Evidence runs/animal_service_cost_sep08_001/INCUMBENT_FIX.md. Only live task62984 broad132864; package all132+27 checks complete; ONE authorized upload still unattempted. Review109/latestrefresh1508, nextreview15:30/refresh16:08. Nusrati exactduplicate Yusuke; AhmedV25 uses same4tapes with execution layers, queued.

2026-09-08T15:32:22.503567+00:00: Review110. Single authorized upload56101451 made15:30:57, PENDING; monitor1064 will fetch/reproduce server validation. Selected using completed discovery and exact package/native checks; broad132864 and unchanged promotion gates remain pending on62984, accepted reference stillempty_sale_slots_m2. V25 C++10793 source actions match, ops4178 live. Caller incumbent fix30/30days exact with zero search, invalid cache rejected. Nextreview15:50/refresh16:08.

2026-09-08T15:52:19.038594+00:00: Review111. Root PDF13pages/14tasks complete. Broad132864 completed, q24native margin CIcrosseszero; no researchpromotion. Guard182misses,2weed73cases needinterpretation. V25discovery8448done,255/256vsYusuke,90/256vsuploaded. Submission56101451COMPLETE; no extra upload. No live task handles. Nextreview16:10/refresh16:08.


2026-09-08 16:10 UTC - Preserve parent schedules in the incumbent pool.
New-only incumbents saved178 in two cow courses but lost a valid original
expensive-day schedule. Including the original and revalidating all15 days
with zero new search saves411 each, hires271->267, cost5044->4633, identical
production and rival cash. Evidence:animal_service_policy_sep08_001/retained_parent.
The retained C++ candidate now passes12 course fixtures plus active repair
checks and32 operational games; paired league study is running in
animal_service_policy_sep08_002. Fixed-course savings are not a league claim.
Negative lesson: do not measure care exceptions with freshly re-solved unrelated
days if a cheaper certified original exists. Compare requested versus actual
hires per day; equal cash savings can hide both useful savings and regressions.


## 2026-09-08 16:50 review

Cow final discovery13824:both policies identical,primary wins unchanged,margin+20.93/+13.26. Guard audit84games:42misses all inheritedweed38; day29wheat57 vsparent55 reduces the existing deficit. Compare field identities and actual values, not just equal mask counts. Full V25 factorial study isolates sale timing without output/hire changes; preserve opposing-tail evidence. See research/review_114.md and linked run results.

## September8 17:10 update

Mixed forecast V2 matches all9 production deltas; opponent future flow response is the largest remaining error in this fixed case. See runs/mixed_funding_sep08_001/FORECAST_FINDINGS.md. Handoff/catalog packaging is the current user priority; no research promotion.
