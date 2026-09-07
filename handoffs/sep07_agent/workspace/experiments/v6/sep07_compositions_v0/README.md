# Composition search

Work window: 2026-09-07 00:47 UTC to 2026-09-08 00:47 UTC.

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

Current broad search starting point (11:52):
`candidates/investment_context_guarded_001_best/`.
General goose/cow/sheep/wait valuation at one additional investment, plus earlier
shop-dependent purchases. Twenty-three worker days cover the two late-sheep
herd contexts. Fresh 1,024 games per opponent: 1,008 teammate wins, 767 King,
798 new public v5. All 14 paired matchups improve mean margin; none loses win
utility or any individual game's margin against the same opponent. Existing
equal-group utility rises 87.40% to 88.44%. Exact checks and limitations:
`results/investment_context_validation.json`. This local improvement follows
the single authorized official submission; no further upload has been made.

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
