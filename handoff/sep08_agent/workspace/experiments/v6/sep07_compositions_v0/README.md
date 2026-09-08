# Composition search

Work window: 2026-09-07 00:47 UTC to 2026-09-10 00:47 UTC, extended by the user.

Initial 24-hour checkpoint and verified evidence: `research/final_result.md` and
`research/final_audit.json`. These preserve the first window's results; the
extended goal remains active. Reproduction: `research/reproduction_entry.md`.

Build a strong C++ agent by searching dated crop and animal compositions,
estimating their economics cheaply, compiling promising compositions into
worker schedules, and improving against a growing opponent league. Learn from
current top-player replays throughout the run. Superiority requires exact-game
evidence; estimated profit is a search signal.

- `OBJECTIVE.md`: scope, objective, search and evaluation protocol.
- `OBJECTIVE_COVERAGE.md`: original ideas, implementation status and evidence.
- `DESIGN.md`: current module ownership and implementation choices.
- `LINEAGE.md`: component origins, modifications and supporting evidence.
- `IDEAS_LEDGER.md`: hypotheses, results, and priorities.
- `PROFILING_LEDGER.md`: implementation and diagnostic lessons.
- `PROGRESS.md`: checkpoints and 20-minute reviews.
- `research/user_objective.md`: unchanged user objective.

All generated files and experimental agents stay here. Persistent dependencies
are the repository game engine, local agent API, and optionally `day_solver/`.
Opponent sources are copied into `league/` or referenced from official `agents/`.
Python is limited to data retrieval, data analysis, and offline model training;
agents, planning, estimation, search, and exact evaluation use C++.

Run Python, Kaggle, builds and package commands through
`conda run -n kaggriculture`. Do not use Git.

Current broad search starting point (00:51 UTC):
`runs/observed_sale_lead_004/proposals/observed_sale_lead_start_216/`.
Keep the inherited adaptive farm and advance eligible non-input product sales
after step216. A copied policy predicts the next sale from current public/own
state; immutable calendars share storage. The initial funding phase is preserved
after an earlier-sale prototype accidentally improved a rival's farm financing.
Fresh65536 games against32 opponents, native/PASS5632,
1024 operational games and64 frozen rebuilt full
records pass all declared gates. Read `CURRENT_REFERENCE.json` and
`results/observed_sale_lead_start216_validation.json` for metrics, source lineage and limitations.

Extended-window compiler research: `runs/compiler_labor_sep08_001/RESULTS.md`,
`runs/compiler_route_sep08_001/RESULTS.md`,
`runs/compiler_placement_sep08_001/RESULTS.md`, and
`runs/cold_day_tasks_sep08_001/RESULTS.md`. Placement can change early funding
and animal birth dates. Directly generated first-day task schedules now execute
in full games, but their gains are small and the cold policies remain weak.
The next compiler work targets later daily tasks, funding and service losses.

Previous reference (22:49 UTC):
`runs/rival_wool_context_003/proposals/rival_wool_context_v3/`;
`results/rival_wool_context_v3_validation.json` retains its evidence.

Previous reference (21:49 UTC):
`runs/wool_family_context_v2_001/proposals/wool_family_context_v2/`;
`results/wool_family_context_v2_validation.json` retains its evidence.

Previous reference (20:22 UTC):
`runs/late_portfolio_001/proposals/late_value_s32_t0_r05/`;
`results/late_portfolio_validation.json` retains its evidence.

Previous reference (18:42 UTC):
`runs/opening_market_search_001/proposals/opening_q32_b13_v1/`;
`results/opening_market_validation.json` retains its evidence.
The committed catalog reference remains `agents/external/bohann_opening_v1/`.

Previous reference (17:20 UTC):
`runs/fresh_courses_1642/proposals/bohann_opening_v1/`.

The current adaptive crop/animal policy with the first two market sequences
borrowed from Bohann Wang, episode 106497007 seat 0, submission 56071218.
The later donor composition was tested separately and rejected. The compact
component matches all 768 full records of its library-based control.

Fresh tests cover 1,024 games per opponent across 19 opponents. It wins
969 games and loses 55 directly against the previous mix, with +$34.76 mean
margin. Against King RC4 it wins 1,006 and loses 18; paired mean margin rises
$16,707.93. The established four-group utility rises from 93.587% to 94.999%;
the 95% seed-cluster gain interval is +1.170 to +1.666 percentage points.

This is aggregate improvement with explicit tradeoffs: V5 loses two fresh
wins and $9.28 mean margin, some other means/tails fall, and native public_router
loses one win in 256 games. Generic, pair, debug, serial, self-play, PASS and
native checks pass. Gross wheat trades rise through a round trip, not extra
production. The opening can alter opponent funding and later market flows.
Full evidence: `results/bohann_opening_validation.json`; frozen C++ inputs are
in `runs/bohann_opening_validation_001/frozen/FROZEN.json`. No new Kaggle package
or submission has been made for this local agent.

Previous broad search starting point (16:01 UTC):
`runs/crop_mix_001/proposals/crop_mix_t2_wheat/`.

At day 12, choose the complete three-tile tomato course when at least two
observed shops consume tomatoes and its farm-state guard matches. Otherwise,
use the productive one-tile wheat course. Both preserve the existing optional
berry fertilizer decision on day 20.

Fresh tests cover 1,024 games per opponent across 18 opponents. The established
four-group score rises from 91.079% for the tomato reference to 93.197% for the
mixture; the seed-cluster 95% interval for the gain is +1.784 to +2.468 percentage
points. All paired mean margins improve, and no opponent's win utility falls
relative to the tomato reference. Additional direct tests use 4,096 games each:
1,801 wins, 2,010 ties and 285 losses against tomato (+$67.45 mean margin);
874 wins, 2,800 ties and 422 losses against wheat (+$161.25 mean margin).

The broader group advantage over wheat alone is uncertain: +0.130 percentage
points, with a 95% interval of −0.069 to +0.334. Some old-version win rates are
slightly worse than wheat alone. Retain both components. All 18,432 fresh full
game records exactly match the selected complete component. Debug, generic,
pair, serial, native, PASS and self-play checks pass. Evidence and limitations:
`results/crop_mix_validation.json`. This local variant has not been packaged or
submitted to Kaggle.

Previous broad search starting point (15:41 UTC):
`runs/crop_rotation_berry_gate_001/proposals/crop_rotation_t2_berry/`.

The tomato course replaces 36 wheat with 24 tomatoes across three tiles while
preserving all other output. On its own fresh seed pool, it beat the previous
crop reference 199 wins to 67 losses, with 758 ties, and improved every paired
mean margin across 17 opponents. Its four-group score rose from 90.277% to
91.294%. Small lower-tail regressions are reported in
`results/crop_rotation_berry_validation.json`.

The full guard audit covers 17,408 games and 12,516,352 semantic actions. Of
2,551 selected courses, 2,540 use all 17 compiled days. Eleven fall back on day
28 because a weed appears at the planned coop site; intended crop output is
preserved. The guards remain intact. Exact witnesses are in
`runs/crop_rotation_guard_audit_001/FAILURE_DIAGNOSTICS.json`.

Previous broad search starting point (14:24):
`runs/crop_value_001/proposals/crop_value_m2_t4/`.
The previous reference plus one productive strawberry fertilization when at least
four already observed shops consume strawberries. Fresh1,024games per15opponents:
all paired mean margins and win utilities nondecrease; equal-group utility
88.395%→90.240% (seed-cluster95% gain interval+1.46..2.26percentagepoints).
Directparent278wins664ties82losses,meanmargin+$62.23. Causal64publicgames change
only two additional strawberries in15activatedgames, with unchangedlabor,faults
anddiscards. Native and requiredoperationalchecks pass. Direct-version lowertail
margin declines about$5; full limits in `results/crop_value_validation.json`.
The user-requested second submission is exactly the parent below:56078898,
COMPLETE, stored under `submissions/sep7-investment-context-guarded-v1/`.

Previous broad search starting point (11:52):
`candidates/investment_context_guarded_001_best/`.
General goose/cow/sheep/wait valuation at one additional investment, plus earlier
shop-dependent purchases. Twenty-three worker days cover the two late-sheep
herd contexts. Fresh 1,024 games per opponent: 1,008 teammate wins, 767 King,
798 new public v5. All 14 paired matchups improve mean margin; none loses win
utility or any individual game's margin against the same opponent. Existing
equal-group utility rises 87.40% to 88.44%. Exact checks and limitations:
`results/investment_context_validation.json`. This exact agent was submitted on explicit request as56078898, COMPLETE;
archive and validation artifacts are in submissions/sep7-investment-context-guarded-v1.

Previous broad search starting point (11:27):
`runs/animal_investments_001/proposals/animal_adaptive_r1_c0_b0/`.
General three-species valuation with operational waiting and later entry.
The strongest tested setting buys immediately. Fresh 1,024 games per opponent:
1,015 teammate wins and improved mean margin across 12 opponents on its own
paired seed pool; John strict wins regressed despite better mean and tail.
Evidence: `results/animal_investment_validation.json`.

Previous search starting point (10:50):
`candidates/shop_herd_guarded_001_best/`.
Fresh1024games/opponent:996teammate wins; all12common-seed matchups improve in mean margin and win utility over the submitted parent. Fifteen new guarded worker schedules restore labor efficiency after the sheep branch. Full evidence in `results/shop_herd_guarded_validation.json`.

Previous reference, submitted once as56074695 (COMPLETE):
`runs/shop_herd_combinations_001/proposals/shop_herd_s6_m3_g1/`.
Justin150's course with attributed terminal recovery, locally rebuilt worker days,
and the user's observed milk/wool shop-demand rule for two day7 purchases.
Fresh1,024 games per11 opponents:599 previousbest wins (+$1,582 mean margin),
972 teammate,725 King,924 v4,891 public,884 Binghua,858 Jun and864 John.
Native/deployment/self/PASS checks pass. Full evidence and tail limitations:
`results/shop_herd_validation.json`. The conservative mode2 variant is also retained.
Changed herds still lose some old labor savings; their day rebuilding is ongoing.
Earlier courses, versions and specialists remain in the league. The full
composition-search goal continues.

Reproduce a C++ composition-improvement round with
`scripts/run_ticket_experiment.py`; see `research/ticket_search_runner.md`.
Read the latest checkpoint at the end of `NEXT.md` before resuming work.

Build arena: `conda run -n kaggriculture python scripts/build_arena.py` from this
directory. The command prints the cached binary path. Example arguments:
`--a public_router --b teammate_shoprouter --games 128 --seed-start 1000
--threads 4 --validate --output results/comparison.json`.

Source parity: `scripts/verify_public_router.py` and `scripts/verify_teammate.py`.
Replay proposals: `research/compositions.json` and `research/compositions.txt`.
Exact days: `research/day_templates/`. Initial C++ biology: `include/biology.hpp`;
micro-game validation: `tests/biology.cpp`; profile driver: `src/biology_profile.cpp`.


## 11:27 update

Mainvalidatedsearchreference: runs/animal_investments_001/proposals/animal_adaptive_r1_c0_b0. Usergeneralanimalideaimplementedasgoose/cow/sheep/waitwithfuturepurchase,recedinghorizonobservedshops/market/publicrivalherdvaluation. Oneadditionalday11investment pluspreviousday7shopchoices; arbitrarymany-animalconstructionstillopen. Fresh1250000..1250511both:1015/1024teammatewins, meanmarginall12opponentsimproves. Equalgroupwinutility85.27%→88.00%; Johnwins925→908despitebettermean/tail. Native256andPASSJimprove; allrequiredchecks pass. See results/animal_investment_validation.json. Waitingoperationalincludinglaterday21buy,butbestsettingdoesnotwaitinthissample. CPUestimate2.05us/alternative, marginrank.60–.64. Most9-productmismatchesare+2strawberryfromrestoredfertinputs; targetanimalbiologyalmostexact. Labourguardslostafternewspecies;11newV30daysnowchecked,combinationsearchnext. Review30next11:33; freshglobal1112analyzed18games,newThomas93.8notebookfullportinparallel. ExplicitAtakan3wayportfolioandplacementdonorsinresearch/animal_decision_replays. NoadditionalKaggleupload.
