# Original objective coverage

Reread `research/user_objective.md` at every 20-minute review. The original
wording and later user clarifications remain authoritative. This table prevents
the working design from quietly dropping an idea. Update status from evidence,
and explain every deliberate change or deferral.

| Original idea or intuition | Concrete interpretation | Current evidence / next work |
| --- | --- | --- |
| Product, count, day start, day end | Search dated crop/animal lifecycles; make day indexing explicit | 132 replay compositions,32585 individual lives; CountSpan maintains counts across repeated rotations. Two wheat for20days exactly realizes40plant-days/48output; mixed contracts still have execution gaps |
| Composition generally enough to judge strength | Test ranking accuracy before solving exact movements | First recorded-service/support forecast ranks complete-course margin at Spearman 0.881; own labor estimate weak; broader calibration needed |
| Composition versus composition OR actual agent | Support modeled opponent production/market flows and exact-agent evaluation | C++ arena plus two-sided fixed rival-flow scenarios work; adaptive modeled compositions still pending |
| Simulate day 1 to 30 approximately | Track full-season dated biology and economics with cheaper movement assumptions | Integrated estimate.hpp, 45.4 us/profile/scenario; timing/funding/route approximations explicitly documented |
| Greedy placement, with improvable placement logic | Try fast layouts informed by replays; preserve alternatives | Source and greedy layout alternatives implemented; greedy realization tested in cold/maintained farms but remains weak |
| Top players place dynamically, sometimes behind crops | Do not force animals into a reserved inner ring or forbid crossing crops | Compiler uses source mixed tiles and legal traversal through crops and locked land; alternate layouts still need ablation |
| Water, collect fertilizer, feed and care for productive output | Use top service practices as a starting point and optimize exceptions | Fresh audit shows conditional feed/care omissions and deliberate-looking animal exits; examine economics before generalizing |
| Estimate workforce quickly | Count required work, approximate movement, pickups, deposits and hire timing | V30 workforce/day reconstruction improves v4. Own labor estimate remains weak; allowing zero hires preserves small wheat output and saves60, but worsens mixed-farm lower-tail J |
| Wheat and fertilizer quantities and purchases | Model carried/shed inputs, own output, bought inputs, dated needs and cash | Exact replay trade reconstruction validated |
| Earliest sales and products brought to shed | Estimate production-ready, collection and deposit times separately | Exact local warehouse events and trade flows exported; first conditional financial evaluator tested; approximate scheduling pending |
| Fixed composition, optimize tiles and daily schedules | Separate economic proposal from realization; allow tile swaps and route alternatives | V30 reconstructs 21 days exactly; one compiled earlier sale improves 95.46% versus its parent. Marginal service search now runs |
| Adjust sales/purchases within the day from opponent and actual cash | Replan market timing from observed conditions without breaking service obligations | Bounded day-0 finance components and exact day-18 sale optimization improve incumbent; Kaito flow/sale repair and public capacity guards ported; adaptive own optimizer remains pending |
| Nothing versus cow versus sheep should be nearly instantaneous | Marginal lifecycle evaluation with input, labor, output, capital and opportunity costs | Single-animal conditional market estimates27–30us per baseline/proposal;16-baseline batches5–13ms for12–24changes. Rank calibration and full-game controls recorded |
| Outer search over compositions | Keep dated production intent at the outer level; execution belongs to modules | Typed Proposal mutations, cold rebuilds and four-generation estimate/compile/exact search implemented; first 583 proposals yield no improvement |
| Quick feasibility and approximate best practices | Return separate economic, labor, cash and compiler failure diagnostics | Biology tested; financial evaluator reports funding, supply and slot witnesses; layout/worker feasibility pending |
| Deeper minimax or another search via cheaper estimates | Try deeper search only after evaluator quality/speed measured | Method deliberately open; not silently limited to greedy tuning |
| Reuse day schedule templates when helpful | Treat borrowed exact days and reusable service patterns as compiler components | 4320 exact day templates across144courses; V30 modified days and physical start guards retained in v4. Semantic species-template compiler now tested with explicit feasibility gaps |
| Branch based on shops | Select/rebuild composition suffixes when shops become observable | Must preserve non-anticipativity; no hidden future-shop access |
| Branch based on opponent when helpful | Use public production, prices and inferred flows to choose continuations | No opponent-private state in API |
| Actual self-play and growing league loop | Search counterstrategies, retain diverse previous versions, retest the wider league | Composition estimate/compile loop and four-opponent exact day-combination loop run;264species-template cells tested againstthreeopponents. Repeated adaptive multiopponent composition loop still pending |
| Best teammates, pulled Kaggle C++ agents, earlier iterations | Cover all three opponent groups, with provenance and measured strength | All 11 official ports, 7 archived packages, 3 six-day variants, Kaito v58 and new capacity/terminal ports tested; exact source parity and broad comparisons retained |
| Good openings and good initial proposals, not only random | Warm start from current top compositions/day schedules, plus independent cold starts | Fresh replay library downloaded; source lineage required |
| Many shop scenarios during approximate play | Evaluate common sampled continuations and preserve unused audits | C++ full-season common shop sampling and native official RNG audits implemented; broader multi-opponent approximate scenarios still needed; final seeds unused |
| Estimated versus complete strategy improves estimator AND compiler | Diagnose ranking error and execution loss independently; calibrate both | Ranking calibration .881 with source support, weak own labor model; day-0 ablation isolates capital-to-production cascade; first exact schedule edit retained |
| Pull latest top replays and keep returning to them | Repeated refresh and mechanism study throughout the session | 162 analyzed/executable courses through08:02; all116,478sourceactions match. Justin150 becomes a strong broad reference |
| Final agent beats teammates, Kaggle agents and itself | Promote through full exact games; report gaps when superiority is unproven | v4 wins860/1024fresh vs v3, allpublic/teammates/threeoldercounters,958Mao85/675Mao89/14Junghoon. Newcountergate401Binghua/503John128/570John131; all slightly improvev3, important gaps remain |
| Optimize slow C++ and rethink bad logic or pipelines | Measure end-to-end cost; prune impossible routes/branches and redesign weak stages | CPU C++ default; no demonstrated GPU need |
| Review every 20 minutes and pivot/reprioritize | Review ideas, original wording, global invariants, experiment gates and bottlenecks | Twenty-four reviews through08:47; original attachment reread08:45. Next09:07UTC |
| Borrow anything useful, keep component/idea lineage | Allow direct schedule/placement/composition/branch reuse; distinguish copied, adapted and inferred logic | User clarification recorded in LINEAGE.md |
| Productive service may work almost always and admit local exceptions | Strong default in estimator/compiler; locally optimize dated exceptions in either estimated or exact play | User clarification: do not reject a useful default because it is not universal |
| Alter architecture, component placement or detail whenever evidence helps | Let measured agent quality, calibration, realization and throughput change the design | User explicitly authorizes evidence-backed changes without further approval |
| Search new public notebooks and investigate older pulled agents deeply | Inspect actual source, deduplicate route families, port and test promising agents | 18 old-port comparisons and full Thomas/sixday/Kaito/capacity/terminal source audits; licensed FarmManager source inspected, not yet ported.07:05query finds no changed/new notebooks; remaining audits retained |

No row is satisfied merely because a document mentions it. Mark it implemented
only after runnable code or an actual measured analysis supports the claim.

## 09:27 update

Strongestvalidatedreference:justin_guarded_hires_001_best, fresh969/1024teammate,
967/1024parent; fullgatesandcausalworkforcesavings recorded. Library180courses,
newglobal18fullyanalyzed. BoatleeV29activepolicyportpasses11504sourceactions.
Workforcecompiler is useful on fixed complete contracts, while stock-safe outer
familychanges, adaptive composition suffixes and generic construction remainopen.
Every new outer edit must invalidate/rebuild affected dayplans and address traces.
Review26recordsallglobalmilestones/service/flows/timing/labor/land/fertilizer/faults.


10:09: observed-shop composition branches now improve exact fresh league results. Two day7 purchases are selected from current milk/wool demand, with actual output -54milk/+48wool when both switch. Full adaptive composition construction and opponent-aware herd choices remain incomplete. Branch scheduling rebuild is next; day solver no longer treated as substitute for strategic adaptation.


## 11:27 update

Mainvalidatedsearchreference: runs/animal_investments_001/proposals/animal_adaptive_r1_c0_b0. Usergeneralanimalideaimplementedasgoose/cow/sheep/waitwithfuturepurchase,recedinghorizonobservedshops/market/publicrivalherdvaluation. Oneadditionalday11investment pluspreviousday7shopchoices; arbitrarymany-animalconstructionstillopen. Fresh1250000..1250511both:1015/1024teammatewins, meanmarginall12opponentsimproves. Equalgroupwinutility85.27%→88.00%; Johnwins925→908despitebettermean/tail. Native256andPASSJimprove; allrequiredchecks pass. See results/animal_investment_validation.json. Waitingoperationalincludinglaterday21buy,butbestsettingdoesnotwaitinthissample. CPUestimate2.05us/alternative, marginrank.60–.64. Most9-productmismatchesare+2strawberryfromrestoredfertinputs; targetanimalbiologyalmostexact. Labourguardslostafternewspecies;11newV30daysnowchecked,combinationsearchnext. Review30next11:33; freshglobal1112analyzed18games,newThomas93.8notebookfullportinparallel. ExplicitAtakan3wayportfolioandplacementdonorsinresearch/animal_decision_replays. NoadditionalKaggleupload.


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

Review33 completed12:19: original objective reread, full82global metrics from new1212cohort, corrected early cow control384/384cash parity but no new early-choice improvement; Atakan demand/margin specialists validated; nonlinear future-shop/price error is next diagnostic priority. Details research/review_33.md. Next review12:39.

## 2026-09-07 12:45 UTC — Review34

See research/review_34.md for all82global/local metrics and original-goal coverage. Root early-animal and sampled-value screens produced no new incumbent; retain investment_context_guarded_001_best. Atakan sampled64 fresh specialist audit improves legacy47wins/3072, native checks pending. Prioritize productive crop fertilization with full supply/routing/harvest/sale dependency checks; preserve general animal selection including wait and larger composition edits. Next review13:05.

## 2026-09-07 13:02 UTC — Crop output and route closure

See results/fertilization_checkpoint_1302.json. All239source crop life harvest totals match the cheap biology model;414extra-fertilizer proposals generated in54–77µs. Single-day edits lose later optimized routes. Rebinding source routes restores labor; preserve original trade timing because sell-all altered pre-existing berry sales. Top corrected day20strawberry edit adds2sold berries in46/64activated games, mean cash+$56.78andmargin+$46.34with unchangedlabor. Wider league pending. Wheat extra harvest is currently discarded, requiring storage/deposit/sale rebuilding. Replay audit clarifies local strawberries already90.85%productive fertilizer coverage; wheat/carrot account for major within-crop gap. Explore a complete Mengfei wheat→tomato→carrot block next.
