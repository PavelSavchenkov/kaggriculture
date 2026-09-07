# Progress

## 2026-09-07 00:47 UTC — start

Deadline: 2026-09-08 00:47 UTC. Next full review: 01:07 UTC.

Read objective and required repository instructions. Created the self-contained
experiment. Started refreshing the official leaderboard. Located existing
top-replay download/analysis tools, verified engine, local C++ opponent catalog,
and the persistent day scheduler. No candidate or superiority claim yet.

## 2026-09-07 00:53 UTC — goal and replay checkpoint

Downloaded fresh leaderboard (00:48:32 snapshot) and 56 unique episodes covering
72 player-games from the current top 12. Full replay analyzer detected a $22
maximum money mismatch; diagnose before accepting its economic statistics.

User asked why a GPU was needed. No current evidence supports that need; CPU
C++ is the default, and speculative GPU/surrogate work is deferred.

User saw no attached goal. Earlier create_goal reported active, but get_goal
now returned null. Recreated the full faithful objective and immediately
verified active state. Saved tool state in research/goal_state.json. The cause
of the missing state is not established. Original 24-hour deadline is unchanged.

## 2026-09-07 01:07 UTC — review 1

See `research/review_01.md` for the full global invariant audit, baseline tests,
scope limits and reprioritization. Replay analysis passes exact cash validation.
C++ teammate adapter, generic arena and debug pair runner work. PASS/self-play
checks passed; generic/pair actions and results match. Teammate beats indar,
kaito and boatlee in all 256 discovery games per opponent. This is a baseline,
not an improved candidate. Prioritize stronger replay-derived opponents and
composition/day-template extraction. Next review 01:27 UTC.

## 2026-09-07 01:27 UTC — review 2

Expanded public-agent audit to archived C++ ports and twelve recent notebooks.
Seven copied historical packages do not beat the teammate, but newly ported
public_router wins 251/256 discovery games against it. Its C++ actions match
8,628 reference actions including all four branches. Broad league/promotion
testing is next; do not yet call this a promoted result. See research/review_02.md
for global invariants, source audit and gaps. Core composition estimator and
compiler remain pending and are now the implementation priority. Next review
01:47 UTC.

## 2026-09-07 01:47 UTC — review 3

See research/review_03.md. New public reference passes fresh promotion against
the tested teammate and broad discovery versus eighteen older ports. Both
adapters pass original-source parity. Replay compositions/days and the first
biology component are implemented. Fertilization, intraday tile reuse and
input/output versus net sales are concrete estimator calibration issues.
Next: full local invariant measurements and fast economics, followed by cold
composition compilation. Next review 02:07 UTC.

## 2026-09-07 02:07 UTC — review 4

Implemented exact local profiling and a first conditional financial evaluator.
Profiled 32 games; original action hashes/results are unchanged, and service,
input and trade-accounting checks pass. Four full financial fixtures match
engine market/cash behavior in their tested scope. See research/review_04.md for
the direct comparison of local and global service, output, net flows,
milestones, labor, land, weeds and discards. Full economics/placement integration
and a cold compiler remain next. Next review 02:27 UTC.

## 2026-09-07 02:27 UTC — review 5

First C++ compiler realizes program 0 from an empty farm using observed dated
lifetimes/layout/support and independently generated worker actions. Eight exact
games expose crop yield, service, timing and overflow gaps. Review write-up
completed at 02:35; see research/review_05.md. Source service masks and input
reserve ablations were next.

## 2026-09-07 02:47 UTC — review 6

Completed seven common-seed compiler configurations. Found inconsistent
expansion feed accounting and an early hiring cash-reserve cliff. Recorded
service improves some crop output after fixing hiring, but does not beat the
first compiler or public reference. Copied the exact program 0 action trace as
a teacher: 719-action parity, 2/8 wins versus public_router, much better crop
realization. See research/review_06.md for all global/source invariant checks.
Next: integrate the fast composition estimator and use exact realization gaps
to guide the compiler. Next review 03:07 UTC. Goal active.

## 2026-09-07 03:07 UTC — review 7

Integrated heuristic estimator at 45.4 us/composition/scenario. Recorded support
and service give first margin-ranking correlation 0.881 across complete replay
courses; own labor estimation is poor. Both greedy compiler modes lose all
1,152 exact games. All 72 recorded courses are now C++ and pass 51,768-action
parity. feeltheagi_55 wins all 2,048 fresh games versus public_router and 255/256
native RNG games, but older skomuro/Deniz/Arman agents counter it strongly.
Keep the resulting counterstrategy cycle in the league. Full evidence and
global profiles in research/review_07.md; next review 03:27 UTC.

## 2026-09-07 03:27 UTC — review 8

opening_router_v0 fixes the prior counterstrategy cycle through a legal turn-1
public-worker branch: all 1,024 fresh games won against each of four reference
opponents. Six parent behavior comparisons and generic/debug/thread checks pass.
A fresh global leaderboard introduced three new top-12 players; 18 games are
reconstructed and all 90 courses now pass 64,710-action parity. New Mao courses
and one 3정훈 course counter the branch, exposing its limited generalization.
See research/review_08.md. Next priority is an actual composition mutation and
estimate/compile/exact loop, with arbitrary cold inputs and better labor/cash
realization; later compatible branching also remains useful. Next review 03:47.

## 2026-09-07 03:47 UTC — review 9

Arbitrary typed proposals compile; six independent cold farms execute with zero
unit faults, and custom program 0 matches its indexed behavior in eight profiled
games. The first four-generation C++ search estimates 583 proposals in 0.175 s,
tests 36 in 288 exact games, and retains source program 4 without improvement.
All lose public_router. Evidence in runs/search_v0_001/ and review_09.md.
Prioritize stronger schedule realization and conditional-estimate calibration
over larger search volume. Port newer six-day teammate variants to complete the
requested strong-opponent audit. Next review 04:07 UTC.

## 2026-09-07 04:07 UTC — review 10

Three six-day policy modes ported; 28,041 total source actions match. Broad
256-game/opponent comparisons confirm cash collapse and the reserve repair's
mixed effects. None displaces our strong references. All package checks pass.
Day-solver V30 now called directly in C++: 26 recorded day contracts valid,
21 solved and all 21 equal in full-game replay, five UNKNOWN, three source
overflow days excluded. Accepted market quantities and physical netting of
same-hour round trips are essential. Exact inputs, schedules and failures are
retained in results/day_rebuild_55_v2/. Kaito v58 modules statically unpacked;
port still pending. See review_10.md; next review 04:27 UTC.

## 2026-09-07 04:43 UTC — late review 11

First V30 sale-timing search compiles two improvements; proposal 7 beats its
parent in 95.46% of 2,048 fresh games. opening_router_v1 incorporates that day
and wins 989/1,024 against v0, plus all 1,024 each against six older public/
teammate references. Generic/debug/thread and PASS/self checks pass. Thirty-two
paired profiles preserve daily biological contracts, output, input buys,
workforce and land; the gain is timing, not production. New replay counters
remain unresolved. See review_11.md. Kaito v58 C++ port next. The scheduled
04:27 review was late; next scheduled review remains 04:47.

## 2026-09-07 04:50–05:09 — reviews 12 and 13

Kaito v58 and named fresh Junghoon/Mao course ports pass source and deployment
checks. Causal market-course audit isolates Mao85's day-0 financing as a useful
component of program55; later/all-market swaps regress. Paired profiles show
restored livestock and output with unchanged hires/land. Fresh 05:04 global
snapshot introduces SpaTaro and kwa. See research/review_12.md and review_13.md.

## 2026-09-07 05:31 — review 14, scheduled 05:27

opening_router_v2 passes required checks and fresh 1024-game/opponent gates:
all wins versus v1/public/teammates/Skomuro/Arman, 1023 versus Deniz, 901/636
versus Mao85/89, zero versus Junghoon78. Native official RNG audits support
the improvement with the same unresolved counter. Tetsutani capacity and Lynn
terminal ports each match 8628 source actions; capacity beats Thomas 72.66%
in discovery, terminal adds no difference in 2304 games. Twelve latest global
courses extend the library to 102; all 73,338 source actions match. See
research/review_14.md for deep global invariants and original-goal coverage.
Next review 05:47. Return to composition/service/compiler dependencies.

## 2026-09-07 05:51 — review 15, scheduled 05:47

Marginal service search prunes all sixteen extra-care opportunities in four
families. Of 327 existing care visits, 70 can be removed without lost harvest;
ten complete reduced-hire days solve and pass full endpoints. The causal control
keeps all service and finds fifteen valid days with one fewer hire. A new C++
league combination loop retains five days, saving $487 in healthy games;
three rejected but promising alternatives are retained for broader testing.
Required checks and a new promotion gate are next. Global fault counting is
being completed after deliberate fixtures caught a comparison bug. Original
wording reread 05:48; see research/review_15.md. Next review 06:07.

## 2026-09-07 06:09 — review 16, scheduled 06:07

hire_day1_day9 wins960/1024fresh versusv2 and beats tested parent variants;
allpublic/teammates/Skomuro/Deniz,1023Arman,960Mao85,670Mao89,17Junghoon.
Native RNG audits support improvement. Named wrapper opening_router_v3 building.
Six-way causal audit attributes day1's large Junghoon gain to rebuilt routes;
accepted-market formatting alone neutral, hire deletion alone harmful.
Latest06:04leaderboard puts Junghoon first; thirty fresh player-games and one
new notebook downloading. See review16 for full invariants and priority review.
Original objective reread06:07; next06:27.

## 2026-09-07 06:31 — review17, scheduled06:27

v3 wrapper checks complete. Physical day-start guards permit seven additional
rebuilt days; guarded_hires_v3 wins860/1024fresh versusv3, preserves broad older
league strength, still loses Junghoon. Native RNG audits support gain. Thirty
new globally selected replay courses extend library to132; all94908actions
match. Binghua116 andJohn128/131 are new counters. FarmManager notebook and
immutable source audited, not yet ported. Generic compiler priority/care ablation
underway, preserving old behavior. See review17 for full independent global
invariants and original-objective gaps. Next06:47.

## 2026-09-07 06:49 — review18, scheduled06:47

Guarded schedules improve all three new replay counters on fresh1024games each;
named opening_router_v4 retained. Wrapper1024fields match underlying candidate,
generic/debug/thread16fields match, PASS/self pass. Healthy32paired profiles
preserve biology/output/sale volumes/land; mean cashgain173.44 primarily saves
175.38hirecost. PASS256J145940 versus145573parent. Still weak versusJunghoon
andBinghua. Generic priority/care/target-continuation ablations regress; movement
reversals are not the main problem. Warm compiler data now132courses with old72
facts exactly preserved. Semantic species-template and maintained-count module
experiments underway. See review18; original wording reread06:47. Next07:07.

## 2026-09-07 07:08/07:29 — reviews19/20

Maintained-count spans now compile the literal wheat2/20days example into
repeated rotations;32games realize40plant-days/48wheat, optional zero-minimum
labor saves60cash with equaloutput. Mixed farms still underproduce and can loseJ.
Species-template search produces a useful Junghoon78 goose-to-cow specialist:
653/1024fresh parent wins,margin954;163/256native. It countersv4/Mao but remains
weak against public/teammate/newJohn. Paired profiles isolate one changed animal,
real milk/egg output,100addedcost and rivalprice response. Cheap fixed-rival
estimates miss the gain. Finance7 public notebook port matches34512original
actions across4modes; full-game component ablations underway. Latest07:05global
fogflower/OceanMix replay cohort fully analyzed;144library courses now match all
103536original actions. Full invariants and objective review in review20.
Next07:47; nextfresh750000+, final900000+untouched.

## 2026-09-07 07:47 — review21

Finance7 source/deployment and broadleague checks complete; distinct Junghoon
counter but weakerv4/public/teammate. Mirror helps itsownbase; financing is not
universally beneficial. Latest144course library sourceparity and new12league
screens complete. New individual-animal composition search traces exact purchase/
pickup/placement identity andbaseline service, estimates60alternatives across
four runs, fulltests38changes pluspairedoutputcontrols. Allbaseline animalbiology
and64identitygames match. Fixedquote estimates miss endogenous price effects;
one apparent compositiongain ismostlyoutputcontrol. Improvewhole-market marginal
valuation, then expand toincumbent/shopbaselines. See review21. Next08:07.

## 2026-09-07 08:07 — review22

Conditional market replay preserves exact joint order slots and reproduces both
baseline cash balances. Multiple-baseline composition estimates now guide exact
single-animal compilation; 48 baseline biology/market checks pass. Binghua's
useful substitutions rank near the top across16 baselines. Two retained specialists
pass fresh parent gates: Junghoon wool timing968/1024; Binghua ticket9 composition
732/1024. Paired profiles isolate pure timing in the first and output/funding
changes in the second. New08:02 global cohort of18 games fully analyzed; executable
library still144. Three public notebooks downloaded, King RC4 audit pending.
See review22. Next08:27; next fresh800000+, final900000+ untouched.

## 2026-09-07 08:27 — review23

King RC4 public source audited and ported into an experimental C++ package.
First14,380 source actions match, including forced sheep/fertilizer/hiring and
continuation cases. Its Thomas tail is byte-identical to the previously verified
source; legacy anti-router and classifier layers are bypassed by the final entry.
Extended parity and generic/debug builds running; strength remains unmeasured.
Review23 compares the unchanged independent global cohort with v4 and retained
specialists, preserves the original composition/compiler priorities, and records
the next20-minute checkpoint at08:47. Fresh800000+, final900000+ untouched.

## 2026-09-07 08:47 — review24

King RC4 source/deployment/fresh checks complete. Latest18 replay courses extend
library to162; all116,478actions match. Justin150 wins fresh majorities versus
all8 tested current opponents, including805/1024v4, but King mean/tail remain
weak; old-agent league running. Named Justin/Atakan references fully checked.
Two reproducible C++ composition rounds start fromJustin150;32baseline checks
exact,28alternatives estimated,40changed/control full batches retained. No broad
promotion yet. Earliest full-output melon rotations materially improve cold
realization and mixed-farm J; service deletion helps some farms and regresses
others. Review24 preserves full global/Justin/v4 invariants and original gaps.
Next09:07; next fresh1,000,000+, final900000+ untouched.

## 2026-09-07 09:07 — review25 (recorded09:11)

Justin terminal variant passes broad/fresh/native/deployment checks; primary broad
reference updated. Paired causal tests separate recovered sales from sacrificed
fertilizer work. V30 solves22hire-reduction and13care-reduction alternatives with
full endpoint/cash checks; guarded combination next. Full global invariants in
review25. New notebook ANTIMODE under inspection. Nextreview09:27.

## 2026-09-07 09:27 — review26

GuardedJustin passesfresh967/1024parent and969/1024teammategames; primaryreference
updated. Exact32pairedgames save22hires/$1471withsame dailyper-tilebiology/service
andmarketflows; intradaylifecycle/stocktimingdiffers. Globalnew18cohort fullychecked,
library180courses/129420actions. BoatleeV29port11504sourceactions match; strength
pending. Fullglobalinvariants andoriginalpipelinegapreviewinreview26. Next09:47.


09:56 review27: direct observed milk/wool shop rule tested. Day3/day7 cow-to-sheep branches improve all five discovery matchups; day7 demand-weighted ticket8 margin +668..884. Later reverse substitutions weaker. Combining branches and checking guarded-day losses next. Main validated Justin source remains unchanged.

10:03: shop herd combinations checked across11 opponents at256games each. Conservative day7 pair improves all common-seed means and never lowers win utility; demand-weighted day7 pair wins150/256 against currentbest (+1540margin), stronger aggregateleague with one fewer v4win. Fresh1100000..1100511 both now running for both candidates and incumbent. Paired64profiles show real cow-to-sheep output changes and some lost labor savings, crop biology unchanged.

10:28: one officialsubmission56074695 COMPLETE; frozenmode3 C++3944/4096nativeRNGteammate and3980/4096independent,packed62/64withcash/actionparity,servervalidation1438actionsreproduced. Submissionartifactsin submissions/sep7-shop-herd-adaptive-v1. Userauthorizedexactlyone; no more planned. Rootcontinuesmaingoal. New15daybranchschedulevariantpassesbroad13x256andrequiredchecks,fresh1200000nowrunning.


## 10:51 checkpoint

Review29: originalgoal reread10:46; generalcow/sheep/goose/wait with future reconsideration is main priority. Newlocalbest shop_herd_guarded_001_best promoted:996/1024teammate; all12paired fresh matchups nondecrease. Evidence results/shop_herd_guarded_validation.json. In14/64changed profiles15fewerhires/$1322saved/13fewerfaults,biology64/64equal,trades62/64equal. Official56074695COMPLETE; no anotherupload. PublicTITAN/Market-v4ports checked but weak. All evaljobsfinished; parallelpublicagent completinglineage. Review29fullglobalinvariants; next11:10. Fresh1250000+,final900000untouched. Goaluntil00:47tomorrow.


## 11:13 checkpoint

Review30complete,originalgoalreread11:11. Generalanimal+waittypedpolicy/valuationimplemented;24checkedentryplans across2herdcontexts,days11/15/18/21all3species. Day12UNKNOWN. Fixed/adaptivefullgamescreen building(session86561). Currentbestunchanged; noadditionalupload. ClearAtakan3speciesbranchwithidenticalprefixandcrop-to-structure reuse found; donorartifacts beingretainedbyparallelagent. Freshmetadata1112started. Nextreview11:33.


## 11:35 checkpoint

Review31complete,originalgoalreread11:33,fullglobal1112invariantsversusnewadaptiveprofiles. Mainreferenceanimal_adaptive_r1_c0_b0:1015/1024teammate,all12meanmarginsimprove,Johnwinregressionexplicit. New11dayinvestment_guarded_001_bestcompiledbutunpromoted; secondcontext1015rebuildrunning8898. Genericfaa06ff632145f6a787e/debugd098d969d6873771cee7ready. Offlineanimalreturnsfix passesold16CSVregressionandguarded/adaptive16biologycases each. Thomasv5strongnewpublicport finishingparallel. Nextfresh1300000+,final900000unused; nextreview11:55. Goaluntil00:47tomorrow.

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

Review 32 completed 11:56; full 82-metric global comparison in research/review_32.md. Next review 12:16.

Review33 completed12:19: original objective reread, full82global metrics from new1212cohort, corrected early cow control384/384cash parity but no new early-choice improvement; Atakan demand/margin specialists validated; nonlinear future-shop/price error is next diagnostic priority. Details research/review_33.md. Next review12:39.

## 2026-09-07 12:45 UTC — Review34

See research/review_34.md for all82global/local metrics and original-goal coverage. Root early-animal and sampled-value screens produced no new incumbent; retain investment_context_guarded_001_best. Atakan sampled64 fresh specialist audit improves legacy47wins/3072, native checks pending. Prioritize productive crop fertilization with full supply/routing/harvest/sale dependency checks; preserve general animal selection including wait and larger composition edits. Next review13:05.

## 2026-09-07 13:02 UTC — Crop output and route closure

See results/fertilization_checkpoint_1302.json. All239source crop life harvest totals match the cheap biology model;414extra-fertilizer proposals generated in54–77µs. Single-day edits lose later optimized routes. Rebinding source routes restores labor; preserve original trade timing because sell-all altered pre-existing berry sales. Top corrected day20strawberry edit adds2sold berries in46/64activated games, mean cash+$56.78andmargin+$46.34with unchangedlabor. Wider league pending. Wheat extra harvest is currently discarded, requiring storage/deposit/sale rebuilding. Replay audit clarifies local strawberries already90.85%productive fertilizer coverage; wheat/carrot account for major within-crop gap. Explore a complete Mengfei wheat→tomato→carrot block next.
