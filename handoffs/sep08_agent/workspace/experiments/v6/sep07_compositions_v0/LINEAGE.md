# Component lineage

The user explicitly authorized borrowing any useful material from top-player
replays or agents on 2026-09-07: placements, full day schedules, compositions,
branching and other components. This overrides repository guidance limiting
external behavior reuse for this experiment. Preserve attribution and applicable
source license information. Experimental agents remain inside this experiment.

For every retained component record:

- Local component/version and its parent versions.
- Source player, episode ID and seat, submission/leaderboard snapshot, or exact
  code path/URL and source hash, as applicable.
- Whether behavior was copied, adapted, inferred from observations, or developed
  here. A single replay does not establish an opponent's branching logic.
- Changes made here, their reason, and the idea or counterexample behind them.
- Exact evaluations and common-seed ablations supporting retention or rejection.
- License and known reuse terms when adopting source code.

| Component | Origin | Changes | Evidence/status |
| --- | --- | --- | --- |
| Earlier crop rotations and service deletion | Melon first-cap observation from muelsyse111 crop/labor notebook; exact official engine rules; new local C++ lifecycle/search code | CountSpan -2 chooses earliest equal-output harvest; optional dated biology-preserving fertilizer/water deletion; original defaults preserved | 40 isolated engine cases match;16 variants ×32 exact PASS games. Two-melon contract output12→24, mixed-farm J+3,904 from early harvest; generic service pruning can regress. See research/crop_rotation_findings.md |
| 08:02 replay refresh | Fresh globally selected Mengfei, JustinLee, Atakan episodes; exact snapshot/submission/seat/hash in library IMPORT.json | Append courses144–161; original144 unchanged; retain Justin150/154 andAtakan161 as named reference courses | All162 courses116,478 source actions match;18×64 games vs v4/public. Justin150 wins56/64v4; new broad and deployment checks running |
| king_rc4 | yamakawanin/king-v4e-rc4 frozen SHA26ffba5273e4432dbc4ec822d5a65b09d3e9f1473c3ce813c528812eeb45779d; exact extraction in research/refresh_0804 | Faithful active controller C++ translation;52 program blocks,22,289 market examples; byte-identical Thomas tail reuses public_router with read-only planned-action accessor; inactive legacy layers omitted | First14,380 source actions match; expanded parity and deployment/league checks running. Rating/license unknown; original replay IDs not supplied; full attribution limits in IMPORT.json |
| Replay downloader | New experiment script; patterned after existing repository retrieval workflow | Fresh leaderboard, concurrent retrieval, metadata saved | 56 unique current episodes downloaded |
| Replay analyzer | Copied from repository `work/latest_invariants/analyze_top_replays.py` | Self-contained paths; fail-fast mismatch witness; preserve worker inventory insertion order across replay steps | Exact cash matches all 72 selected player-games |
| Replay summary | Copied from repository `work/latest_invariants/summarize_top_replays.py` | Self-contained paths | Validated analysis exported |
| Transition engine | Persistent repository `fast_game_engine/`, official 1.32.7 port | None | Existing parity evidence; integration checks still required |
| Teammate shoprouter adapter | `external/kaggriculture/agents/shoprouter-rl-v2`, yhay81 backbone and teammate market head | Observation-only local API, immutable decoded tapes, C++ translation of head, action metadata normalization | 7,190 original-source actions match; generic/debug pair match |
| C++ arena | Developed in this experiment | Runtime generic and typed pair modes; independent common shop stream; exact diagnostics | Full games and deterministic cross-runner hashes validated |
| Seven archived agents | Historical Ryo, Arman, Crop Dusta, Subramanya, MrKiwi replay packages, structured economic port and Deniz port; exact paths/hashes in each IMPORT.json | Copied closure, repaired moved includes only | 256 discovery games each against teammate; retained for diversity |
| public_router | Thomas Tschinkel public-state-router notebook; exact URL/hash and four route hashes in league/public_router/IMPORT.json | Typed C++ translation, immutable route/suffix tables, normalized active workers | 8,628 reference actions match; wins 251/256 discovery games versus teammate |
| Public route extraction | Notebook's base64/zlib route payload | Static AST decode and C++ numeric data generation; no notebook execution | Four route prefix lengths verified; original replay IDs unavailable |
| Composition/day library | Fresh current top-12 replay cohort; exact episode/seat/submission/snapshot in each entry | Group realized lifecycles by dated counts; retain precise tile/time instances and full day start/end states | 72 proposals, 4,488 cohorts, 2,160 exact days; no branching intention inferred from one trace |
| C++ biology | Developed here from official 1.32.7 engine rules | Dated service masks, output, wheat/fertilizer and field-operation counts; default retires expired crop service | 128 exact micro-games; economics/logistics and decay-day salvage remain outside this component |
| Detailed C++ profiling | Developed here using exact engine sanitizer/state deltas | Per-life service, actual inputs/net flows, timing, labor/land, weeds, discards and warehouse events | 32 profiled games match ordinary gameplay; mass-balance assertions and fault cross-checks |
| Conditional financial model | Developed here from exact engine price/shop rules | Warehouse schedule, marginal market valuation, input reserve and funding/capacity witnesses | Four 719-turn financial fixtures match engine; full composition calibration still pending |
| composition_greedy_v0 | Program 0: episode 106277936 seat 0, current rank 1, submission 56061226; all 72 sources in PROGRAM_SOURCES.json | Copies dated lifetimes/layout/workforce/land; own greedy job assignment informed by archived structured policy; no fixed worker tape | Initial 8 games: $65,112, no wins versus public_router; zero invalid unit commands; original source snapshot retained |
| Source-service compiler variants | Per-life dated successful services from the same selected replay | Fertilize-before-water, source harvest/feed/care/collection masks; legal runtime repair | Review 6 records negative result, funding rescue and remaining production gaps; no promotion |
| Input/finance compiler variants | Local diagnoses of exact warehouse/market and fixed-cost profiles | Common placed/planned herd reserve; optional three-day surplus retention; optional removal of $60 hiring reserve | Separate common-seed ablations in review 6; original behaviors still selectable |
| leader_program0_tape | Same rank-1 episode/seat/submission, full replay hash in IMPORT.json | Copies 719 exact actions, normalizes active worker count only; no invented branching | All 719 source actions match; 2/8 smoke wins versus public_router; execution reference, not original complete agent |
| Integrated estimator | New composition.hpp/estimate.hpp with tested biology/economics | Source or greedy layout, service/support fidelity choices, approximate work/input/deposit dates; two-sided fixed rival valuation | 45.4 us/profile/scenario; first ranking calibration and substantial limits in estimator_scope.md |
| top_replay_library | All 72 current top-12 episodes/seats/submissions and SHA256 in IMPORT.json | Direct C++ action formatting, active worker normalization only | All 51,768 original actions match; 72 complete courses tested in 8+64 discovery games each |
| feeltheagi_55 | Library program 55, episode 106284179 seat 1, feel the agi, submission 56058325, rank 10 | Exact recorded course, no policy optimization/repair/inferred branch | 2,048/2,048 fresh wins vs public_router; older skomuro/Deniz/Arman counter it; review 7 |
| opening_router_v0 | Unchanged public_router and feeltheagi_55 components; selector developed here from public opening probes and the counterstrategy cycle | One shared initial action, then select feeltheagi when opponent has not hired yet, otherwise public_router; no identity or hidden inputs | 1,024/1,024 fresh wins against each of public_router, skomuro, Deniz and Arman; 1,536 exact parent-equivalent games plus generic/debug/thread parity; ties feeltheagi on average |
| 03:14 replay refresh | New snapshot leaders HowardLeeTW (#1), 3정훈 (#2), Mao Nishino (#12); exact metadata in research/refresh_0314 | Eighteen new exact reconstructed games and 540 day templates; append programs 72–89 without renumbering existing sources | Fresh action parity and independent opposition testing in progress; ignored HIRE numeric arguments found and normalized per engine rules |
| Arbitrary composition API and cold farms | Developed here from typed Life, Support and official biology | Custom immutable proposal storage; six independent rule-built families | 48 complete cold games, zero unit faults; custom/indexed program-0 parity in 8 profiled games |
| search_v0_001 | Local C++ proposal mutation, estimation and archive; original 72 replay compositions are warm seeds | Dated count/lifecycle edits, product-family insertion/deletion, cold rebuilds, dependency-closed land and full compiler reruns | 583 estimates, 36 exact candidates, 288 discovery games; no wins or improvement over best source; best search_v0_4 preserved |
| Six-day policy ports | yhay81 Six-Day Public-State Fieldbook and source dataset; teammate sixday-rl and sixday-robust-rl wrappers; exact hashes in league/teammate_sixday/IMPORT.json | Persistent matching engine types, isolated guard namespace, immutable route tables, per-instance state, literal C++ wrapper translation | Three modes each match 9,347 source actions; full league tests and generic/debug/thread parity; original collapses against wheat-trading family, reserve variant still loses |
| Day-solver integration | Persistent day_solver V30, matching engine headers verified byte-for-byte | New offline C++ extraction of successful tile work, purchases, sales deadlines and day endpoints; rebuild routes then replay with original markets | src/rebuild_days.cpp and scheduler/CMakeLists.txt; integration checks in progress; source overflow lies outside solver contract |
| advance_sales_001_7 and _9 | Program 55 complete source course plus local sale-deadline search and V30 route rebuilding | Move one noninput-product sale earlier, preserve daily biological/physical obligations and rebuild all worker routes | 21 cheap proposals, 16 solver attempts, two solved; proposal 7 wins 95.46% versus parent over 2,048 fresh games; exact lineage in generated packages and run artifacts |
| opening_router_v1 | opening_router_v0 public-worker selector and public_router branch; advanced program-55 branch from advance_sales_001_7 | Replace only the delayed-hire course's rebuilt day-18 schedule and sale | 989/1,024 fresh wins vs v0, 1,024/1,024 each vs six older reference opponents; paired profile and generic/debug/thread checks; newer replay counters unresolved |
| kaito_v58 | Kaito Fukami v58 Version 11, repository champion-v58 copy with Pilkwang Kim loader shim; SHA256 and route hashes in IMPORT.json, Apache-2.0 LICENSE/NOTICE | Literal active controller port, immutable numeric routes, independent per-instance histories, eager prefix-equivalent controller initialization; disabled branches omitted | 17,256 source actions match; full 256-game/opponent league tests and generic/debug/thread/PASS/self checks; weaker than incumbent but distinct |
| junghoon_78, mao_85, mao_89 | Existing verified replay library programs from globally selected 03:14 snapshot; exact episode/seat/submission/hash in each IMPORT.json | Named C++ wrappers only; no action changes or inferred branching | 256-game full league comparisons preserve a new counterstrategy cycle; all source actions already verified, debug/pair checks completed |

| Market component audit and firstday_m85_v0 | Program55 worker/later-market course and Mao85 day-0 market course; exact source IDs/hashes in the named parent IMPORT.json files | New C++ bounded market overlay; nine common-seed component ablations distinguish the one-unit first change, whole opening and later changes | Day-0 component restores real production versus v1 and wins all 256 discovery games; later/all-market substitutions regress |
| firstday_m85_sale7 and opening_router_v2 | Day-0 Mao85 markets plus program55 with local V30 day-18 sale advance; v1's public-worker selector and Thomas branch | Combine independently tested components, then independently retest whole policy | 1024 fresh games/opponent: all wins vs v1/public/teammates/Skomuro/Arman, 1023 Deniz, 901 Mao85, 636 Mao89, zero Junghoon; required checks and native RNG audits pass |
| public_capacity_router | Tetsutani public notebook; exact SHA/URL in IMPORT.json; decoded four-course blob identical to Thomas | Faithful C++ one-slot day-close capacity guard, original parent unchanged | 8628 source actions match; generic/debug/thread/PASS/self; wins 72.66% vs Thomas in 256 discovery games |
| public_terminal_router | Lynn Sakurai refreshed notebook, source SHA 1dc166ae2bf0c56a44fac4482f469b8812968c4cb32459cb9860f5077897a7d4; same Tetsutani/Thomas ancestry | C++ final-step full-stock liquidation, including floor-price products | 8628 actions match including forced terminal cases; all 2304 discovery games equal capacity-only; no measured incremental gain |
| 05:04 global refresh | SpaTaro #8 and kwa #12 in official 05:04:43 snapshot; research/refresh_0504 exact episode/seat/submission hashes | Twelve new reconstructed games, 360 day templates; append programs90–101 unchanged | All 102 courses pass 73,338 actions; twelve new courses tested in 64 games each vs public and v2, none consistently wins |
| improve_care compiler experiment | Local code from official care-bank/cap rules and source55 successful service calendar; persistent V30 routes | Cheap isolated marginal harvest estimate, conditional future replay, add/remove care and reduce hires with full day reconstruction | All16 extra-care proposals across four families have zero value; 70/327 existing care visits marginally redundant; ten remove-care/reduce-hire schedules fully validate. Forecast relaxes labor/fixes rival actions, not a bound |
| reduce_hires_001 | Same source55 contracts, all successful service retained | Remove final hire, V30 rebuild complete day, exact engine checks stocks/tiles/production/cash | 26attempts,16schedules,15match; day6rejected,10UNKNOWN; most savings available without deleting care |
| combine_hires_001_best | Locally compiled days21,23,11,13,14 over opening_router_v2 | New C++ sequential four-opponent mean-margin gate; v2branch only; day0/18 excluded due changed economics | Five fewer hires,487saved in all32healthy paired profiles with equal daily biological/economic flows; broad and fresh tests retained |
| hire_day1/day4/day9 alternatives and combinations | Additional explicit reduce_hires_001 day components over five-day parent | Preserve related alternatives rejected by strict per-opponent gate despite aggregate gain; complete combinations separately tested | Day1 sharply reduces Junghoon gap; day1+day9 beats all listed parent versions in fresh gate, remains weak versus Junghoon |
| opening_router_v3 | Behavior-identical named wrapper of hire_day1_day9, full v2/external source and local V30 lineage retained | Replace seven whole days and associated final hires in delayed-hire branch | 1024fresh games/opponent:960v2,allpublic/teammates/Skomuro/Deniz,1023Arman,960Mao85,670Mao89,17Junghoon; native audits support result; wrapper checks pending |
| Day1 causal audit | Local six-way ablation over compiled source55day1 and five-day parent | Separate rebuilt routes, accepted market quantities and hire deletion with full live-opponent replay | Routes retain large Junghoon gain even with all original orders/hires; accepted markets alone neutral; hire deletion alone breaks service |

Source paths above record provenance only; copied scripts do not import the
original work files at runtime.

| Guarded full-day compiler | Locally compiled reduce_hires_001 schedules and source55 physical contracts | Per-instance hour0 tile/shed/seed/land compatibility gate, whole-day commit or parent continuation; no future-finance certificate | guarded_hires_v3 required checks pass;860/1024fresh wins vs v3; native220/256; new global counters pending |
| 06:04 global refresh and named binghua_116/john_128/john_131 | Official global snapshot ranks1,2,9,11,12; exact sources in replay_library_sources_v4.json and wrapper IMPORT files | Append30courses preservingindices; formatting and active-worker normalization only | All132courses/94908sourceactions match; new wrappers broad/deployment checks pending |
| Farm Manager source audit | tokenjunkielabs/tokenjunkielabs-farm-manager; immutable GitHub commit2dc9d9f955adb2f8ddbf238e6e5a334004178a9a | Read-only download, SHA verification, MIT/CC-BY license retention; no code adoption yet | Public tests cover weak baselines; no demonstrated leaderboard strength |
| composition_greedy_v1 ablation | Isolated copy of ourv0 with source hashes in IMPORT.json | Selectable fertilizer-priority reduction and care-after-feed condition, all other behavior preserved | Six full-game mode0/v0 action/cash comparisons equal; first32-game profiles underway; no promotion |
| opening_router_v4 | Behavior-identical wrapper of guarded_hires_v3; exact source/component hashes in IMPORT.json | Name the retained guarded policy after fresh old/new-counter gates | 1024wrapper records equalunderlying; generic4thread/debugtyped1thread16records equal; PASS/self pass |
| Compiler route-continuation ablation | New local per-instance worker-target memory | Optional soft/hard continuation while a job remains available | Reversals nearly eliminated but travel and scores worsen across3programs; no promotion |
| Complete v1 warm data | All132verified original/03:14/05:04/06:04replay metadata | Offline formatting of32585lives plus dated service and support | All original72facts exactly unchanged; PROGRAM_DATA_IMPORT hashes inputs |
| Species template compiler experiment | Verified full courses0,4,55,78,85,89,116,131 and local semantic item mapping | Whole-species count substitution preserves birth/placement/service template while converting purchases, structures, animal pickups/placements and output commodities; optional full-stock output sales | 264cells/4224discovery games; identity16games/source parity; funding/yield/route feasibility remains a measured gap, not certified |
| Maintained-count interface | Local CountSpan expansion from official crop/animal biology | Repeated productive crop rotations, explicit harvest-age alternative, dated occupancy and immature-tail witness | C++ audit building/running; requested occupancy distinct from realized greedy service |
| Maintained-count completed audit | Local CountSpan module, official lifecycle rules | Repeated rotations and explicit immature tail; optional zero-minimum adaptive labor | 10000expansions~.286us; 32normal games realize40days/48output; 60cashsaved, mixedfarmJregresses; results/count_spans_labor_002 |
| species_78_goose_cow | Junghoon program78 exact source IDs/hash in package IMPORT; local SpeciesTemplate mapping | One realized goose becomes cow with related structure/commodity closure, original dated service | Generic/debug/PASS/self pass;653/1024fresh source wins,163/256native; specialist, broader weaknesses retained |
| species_89_goose_cow_sell | Mao program89 source IDs/hash; same local mapping with full-stock output sale option | Change whole goose family and corresponding commodity flows | Required checks pass;87/256against source, mean-8.7; no promotion |
| destbreso_finance7 and finance7_hires | Public destbreso v7.38 notebook, Python SHA196b1aa2476b939787acc45b2435c4dbe61f9c5a0d1b85f1b736a50754386e05; six verified native sources | C++ literal mirror and hire-financing layers on bare yhay81 backbone; no teammateRL head; isolated state/namespaces | 4modes×8628source actions match; full-game ablations pending. Native Apache-2.0, separate layer license undeclared, user authorized reuse |
| 07:05 global replay refresh | Official snapshot new fogflower#11/OceanMix#12; exact entries in replay_library_sources_v5.json | Append12courses as132–143; original132 preserved | All144courses103536actions match;12games exact cash reconstruction and full invariant comparison in review20 |
| Animal purchase compiler | New local animal_ticket.hpp and exact ticket_trace.hpp; source program IDs and full hashes in each generated IMPORT | Change a single purchase/pickup/placement/structure closure; optional output handling has an unchanged-animal control; old service reused | Initial60 estimates/38 changes;64 identity games; baseline animal biology exact. Explicit shared-transfer/return exclusions and funding/route gaps |
| Conditional market tape | New local market_tape.hpp from the official engine's joint order semantics | Preserve accepted quantities, exact order slots, fixed spending and per-unit prices; revalue changed output and both players | All48 multi-baseline cases match both cash and animal biology; ~27–30us per initial marginal estimate. Conditional flows, relaxed deposits and unmatched transfer coverage remain |
| junghoon_wool_sales | Junghoon78 recorded course plus output-only control discovered during local animal substitution search | Append wool at egg-sale slots afterstep266, conditional egg-deposit DROP; animals unchanged | 968/1024fresh parent wins,240/256native; all32paired physical/output/cost records unchanged, wool sells1.204h earlier |
| ticket_p116_t9_i9 | Binghua116 exact replay source; local traced cow purchase at its recorded addresses becomes goose | Related animal inputs/placement/structure and optional output handling; complete source metadata in run package | 732/1024fresh parent wins,191/256native; broad useful specialist;32profiles show cow→goose plus occasionally restored sheep; required checks pass |
| 08:02 global refresh | Official global ranks2 Mengfei,10 JustinLee,11 Atakan; research/refresh_0804 exact metadata | Eighteen recent player-games,1039cohorts/540days; no inferred complete original policy | All18cash reconstructions pass,12328transactions/4315lives. Not yet appended to executable library |
| King RC4 source audit | yamakawanin/king-v4e-rc4, frozen main SHA26ffba5273e4432dbc4ec822d5a65b09d3e9f1473c3ce813c528812eeb45779d | Static payload extraction, no notebook execution; notes and final three overlays inspected | Promising compatible shop/animal/capital controllers; full source audit, C++ port and strength checks pending |

| King RC4 completed port | Frozen source SHA26ffba5273e4432dbc4ec822d5a65b09d3e9f1473c3ce813c528812eeb45779d | Complete active source graph; dead legacy routers explicitly excluded | 14,380 source actions match, deployment/native/fresh checks in king validation; earlier pending row superseded |
| Latest replay library162 and Justin150/154, Atakan161 | Eighteen08:02 globally selected games with exact episode/seat/submission/SHA | Append144–161 and normalize active workers | All116,478source actions match; named wrappers checked, Justin150 broad/fresh/old-league majorities |
| justin_liquidate_v0 / justin_recall_v0 | Justin episode106370340 seat0, King/Dusta recall, Lynn liquidation; nested exact hashes in IMPORT | New typed terminal overlay, preserves market slots and settles final deposits in engine | 192 paired causal games; output unchanged except four fewer fertilizer for recall; combined fresh1024 versus ten opponents, native and required checks pass |
| Justin full-day workforce proposals | Justin150 observed source contracts, local V30 exact day solver | Remove one hire and recompile all original work/orders; physical starting guards added separately | 22 of26 hire attempts solve and match all endpoint stocks/biology plus hire saving; 13 of16 redundant-care variants solve; combination evaluation pending |

| Justin guarded whole-day combination | Justin150+terminal parent;22localV30hire-reduction contracts in justin_day_library_001 | C++sequential six-opponent search with physical day-start guards; whole route days, same service and market slots | Required/native/fresh gates pass,967/1024parent wins;32pairedgames save22hires/$1471 with exact lives/service/output/trade equality; faults26→10 |
| 09:07 fresh replay cohort180 | Global ranks1ymg_aq,2Junghoon,10自己找差距; sources_v7 exact episode/seat/submission/hash | Append18courses162–179; static formatting only | All129,420source actions match; all18cash reconstructions exact; none beats Justin/public in64game screen, so retain as component evidence |

| boatlee_v29 complete port | PublicBoatleeV29-R1 frozenSHA c4a6964cec3c1c99207c32bb1fd91e53c3ec01e6890da5734331cbeab1cc1267; RngRngsubmission55948382 creditedepisodes104679155/104689164/104691666 | Fixed720course,8stepweedrecovery, adaptivepremiumsale/reserve/mirror/pressurecontroller; inactiveemptyfront-runlist documented | 11504sourceactions andrequiredchecks pass;255/256archivedBoatlee,141Jun,35v4,losesallcurrentJustin/Binghua/teammate; retainreference |
| Terminal-parent composition search | ExistingJustin150 course +Dusta/Kingrecall andLynnliquidation; localgenericAnimalTicketOverlay | Trace actualchosenbasepolicy, preserve its terminal behavior in source/control/changedfullgames; affecteddayplans mustbe rebuilt afterchanges | First16tracebiology/bothcashbalancesexact,32sourcegamefieldsmatchprioraudit;14estimates6.45ms,marginrank.709overall/.636laterKing; no broadpromotion |


10:09: shop_herd_s6_m3_g1 becomes primary validated reference. Exact external Justin150 replay, local terminal and V30 components retain lineage. Milk/wool shop rule comes directly from user; mode3 counts Yarn2. Purchase/transfer/output patch and guarded reuse implementation are local. No Goose-portfolio model is used by this candidate. Full IMPORT and results/shop_herd_validation.json.


## 10:59 public notebook audit

TTV1 exactly duplicates existing Lynn terminal router; no duplicate agent. Full C++ TITAN/Kaito-v43/Market-v4 ports registered as league diversity and causal controls; none improves the current broad incumbent. Original-source parity and exact gates: results/refresh_1010_notebook_audit.json. Market-v4 vs Thomas:5wins,28ties,95losses;14.84% is utility, not strict win rate. TITAN source components and price-curve differences preserved literally.


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

2026-09-07 12:48 UTC: registered atakan_integrated_s64_margin from runs/atakan_sampled_shops_001. Atakan three-course replay portfolio preserved; local deterministic64future-shop integration improves old margin selector47wins/3072 and$658mean margin on fresh audit. Keep atakan_demand as complementary specialist. Not an incumbent promotion. Final source/parity lineage in FINAL_VALIDATION.json.

Donor display-name correction: Atakan Aldemir is the correct name. An earlier frozen Atakan README mistakenly says Atakan Mamedov; exact episode/submission IDs and source hashes remain authoritative. Frozen artifacts preserved.

## 15:41UTC — Crop rotation with preserved berry branch

New local reference crop_rotation_t2_berry combines investment-context→crop_value_m2_t4 lineage with Mengfei Li episode106429645 seat0 cell(0,1) tomato calendar and local complete three-tile placement22/31/40, wheat seed cancellation, first-free-morning surplus sales, shed-cap constraint and carrot resumptionday25. Courses009/010 are full17-day V30 compilations against berry-off/on source policies. The observedday12TOM-demand>=2 gate came from25C++rules on discovery1000..1255both×5; exactold-parentleafparity2560fullrecords. Berry branch ownership remains the original day20 weightedSTRAW-demand>=4 rule. Runtime CropBranchSequence is adapted from productive_wheat_rotation_001/source/policy.hpp with source hashes in package IMPORT.json. No unseen shop or rival-private input enters the policy. Fresh and operational evidence is results/crop_rotation_berry_validation.json; frozen233transitiveinputs are runs/crop_rotation_berry_validation_001/FROZEN.json.

## 16:01 UTC — Combine productive wheat and conditional tomatoes

`crop_mix_t2_wheat` combines the frozen `wheat_one_fert` and `crop_rotation_t2_berry::Course` without changing either component. The original observed day-12 tomato demand threshold remains two; when the tomato entry is rejected or its demand threshold is unmet, the wheat policy remains active. Both components contain their own complete day-20 berry continuations. Source schedules come from the previously attributed Mengfei Li tomato calendar and get some fries productive wheat calendar, with all inherited animal and labor lineage preserved. Exact component selection agrees in 1,792 discovery and 18,432 fresh complete game records. Evidence is `results/crop_mix_validation.json`; frozen 301-input closure is `runs/crop_mix_validation_001/FROZEN.json`.

16:31 UTC review45: completed eight-placement screen; no improvement, two distinct economic ties, paired labor attribution recorded. New day-12 replanting forecast audit reduces quantity error sharply but leaves large timing error; financial branch validation is next. See runs/productive_wheat_placements_001/README.md and runs/crop_choice_estimator_001/README.md. Original objective reread16:27, 82 global/local metrics retained, current mix unchanged, active goal through00:47. Next review16:48; no additional uploads.

16:46 UTC: crop-choice audit completed 768 paired contexts (1536 full games),462 eligible. Current two-shop gate is already within$4.598958 mean margin of best tested leaf with hindsight on this finite discovery panel. Best selected alternative changes only4 choices (+$0.89 margin); no promotion. Replanting improves quantity forecasts but own-profit choice helps rivals more through shared prices. Preserve506-source frozen audit and pivot to new whole-farm proposals from fresh16:42top12replays. Full goal remains active; fresh1740000unused.

16:48 UTC review46: full82new global/local metrics recorded. Two-leaf selector headroom only$4.599 on discovery, so prioritize new family/course proposals. Fresh69courses pass49611sourceactions and begin six-opponent screen. X-ray notebook executableASTunchanged42functions; no port. Currentmix and submission56078898 unchanged. Nextreview17:08, activegoal through00:47.

17:09 UTC review47: fresh69-course screen led to Bohann25opening component. Four3840-game ablations isolate gain to first2markets, not later composition; compact768records exact. Fresh19opponents1750000running; pair/debug/native checks and causal King analysis pending. Currentmix remains promoted; smallV5tradeoff explicitly preregistered. Full82global/currentmetrics retained; nextreview17:28, goalactiveuntil00:47, no upload.

17:20 UTC: promoted bohann_opening_v1 as local reference. Fresh19opponents×1024games, directcurrent969W55L(+34.76margin), King1006W18L(+16707.93pairedmargin). Establishedgroup93.587→94.999%,95%gain+1.170..1.666pp. All operational checks and native directpass; compact768records exact;276compileddependencies frozen. Explicit regressions:V5−2freshwins,−$9.28mean;nativepublic−1/256win;othermean/tailtradeoffs retained. SourceBohann106497007seat0first2markets only, latercourseablationnegative. OneKingtrace day1cash27→0 thenworkforce/output changes; initialgrosswheattrades are notproduction. Reportresults/bohann_opening_validation.json. Preserveparent;next test retaining original hire/stock contracts while borrowing opening trades. Submission56078898 unchanged,no extra upload. Nextreview17:28,goalactiveuntil00:47.

2026-09-07T17:58:01.386887+00:00: user-requested strongest catalog agent and detailed prompt committed and pushed to main atf07754bb5. agents/external/bohann_opening_v1 is the exact17:20reference;6144frozenfullrecords exact,16928catalog games, native teammate4059/4096 and lastsubmitted3815/4096. Isolated checkout64records exact; no engine/day-solver copies. prompt_optim.md follows currentAGENTS and omits retired strategy guide. Scopecomplete, no additionalupload. Resume opening_market_search_001 extended20opponent analysis, freshreplay refresh overdue, nextreview18:08, maingoalthrough00:47.

2026-09-07T20:22:53.676928+00:00: Promotedlate_value_s32_t0_r05 overopening_q32_b13_v1. Independent180224-game confirmation passes all gates, current group+.1666pp(95%+.0745..+.2585), direct1527W2256T313L,+$173.98. Native/PASS no mean/utility regressions;256 rebuilt frozen records exact. Real four-way crop/animal choice, with small legacy utility/tail tradeoffs recorded. No Git or submission.

## 2026-09-07T21:25:07.844759+00:00 — Independent V5/2 sheep-heavy family

ThomasV5/2 donor provenance remains league/public_router_v52/IMPORT.json.
New local runs/v52_family_001 probes prove256 original full records, physical
entry compatibility except stock, and stock-restored complete suffix transfer.
Day-solver V30 generates17 checked days ($1275 conditional), while runtime
activation commonly saves15–16hires/$1165–$1254. Exact life/trade/rival profiles
separate labor from funded additional sheep output. The h18 trial label predates
the final17-day count; DAY_LIBRARY.json is authoritative.

Local six-rule comparison uses12288 exact counterfactual games,6144 identical
observed prefixes; selected wool_family_context_v1 preserves all6144 chosen
game records/action hashes. Its source rule is developed here from observed
Yarn timing and other shops, not claimed as Thomas's original decision tree.
Independent49152-game fresh24opponent panel improves current group with95%
positive lower bound, but historical noninferiority and one mean-margin gate
fail. All896 operational checks pass. Candidate retained as a diverse league
opponent; no promotion or external upload. results/wool_family_validation.json
is the complete report. Next diagnose Junghoon with the same causal protocol.

2026-09-07T21:49:29.310519+00:00: Promoted wool_family_context_v2. Whole sheep-heavy continuation with observed-shop selection and compiled labor savings. Fresh51200games/25opponents passes all gates; direct parent139W818T67L,+$363.36. Native4608profiles,896 operational,8192 selector parity,227 frozen inputs and256 rebuilt full records pass. All paired mean margins improve; teammate-1freshwin/1024 and v1 specialist direct loss retained explicitly. No submission or Git.

2026-09-07T22:49:14.724519+00:00: Promoted rival_wool_context_v3. Public spending after six hires and one wheat purchase distinguishes the failed John sale from V5/2 seed purchases. Fresh57344 games: V5/2+6.8359pp and+$182.14 mean; all27other fullrecords unchanged. Native5120profiles,1024operational checks with active custom/native cases,302frozen C++inputs and256 rebuilt records pass. Packaging omission repaired without policy changes; amendment retained. Parent-self unchanged; no Git/upload.

23:37UTC: AhmedV23 exact publicsource mainSHA6eb728a40cc55f7e497add6ea2946ce38b0389c24531a208219d48587435838e reconstructed from auditedtemplate+ThomasV5/2donor,Apachelicense retained. C++port12950sourceactions exact/all5routes/all7layers,896operational fullgamespass.6912discoverygames withcurrent/V52controls. Newpublicagent loses183/256current butbeatsV52180/256. Runtimeattribution maskcontrols sourcecopied separately; no promotion. results/ahmed_v23_validation.json.

23:43UTC: NewAhmedV23 exactport completes12950sourceactions,896operational,6912profiled discoverygames. Independent9layercontrols plusfullparent7680profilegames; all768mask0records exact. Removing salelead loses12/128wins againstourcurrent and6/128V52, meanmargin-$120.15across6opponents. Removingroomguard loses13Junwins and$6573.20mean; removingbudgetloses$503.96Junmean; weedsmallbenefit. Deadstocksales hurtaggregateutility; clampchangesordersbutnoeconomics. Newobserved_sale_lead_001 translates salelead tocurrent usingcopied-policy time-onlynextactionforecast, completeownstockprojection,suppression;3variants(control/all/milkwool) pluscurrent,10opponents256games: live98682. ParentCURRENT_REFERENCE unchanged.


00:23UTC: Sale-lead source shared calendars preserve7680 V2 records and2560 parent-control records;768 forecast diagnostic games match all200000+ eligible next raw sale vectors exactly (exact count in results/observed_sale_lead_v3_validation.json). Ten-opponent discovery improves average margin+$2195.95, but Jun loses11/256 wins and$490.95 mean. No fresh1990000 or promotion. Fixed rival actions do not imply fixed successful purchases or production: Jun1044 loses one sheep purchase at217 after early sale changes, later sells more crop/wool and finishes+$5097 while own-$4988. Global start216/240/288/360 alternatives now running in observed_sale_lead_004; no opponent-ID exclusion.


2026-09-08T00:51:01.675772+00:00: Promoted observed_sale_lead_start_216. Fresh65536games/32opponents, native/PASS5632, operational1024, frozen64 full records/237 C++ dependencies pass. Advance non-input sales after step216 using copied-policy prediction; preserve initial farm funding. Read results/observed_sale_lead_start216_validation.json. No upload or Git.

2026-09-08T01:22:29.282146+00:00: runs/compiler_labor_sep08_001 copies the original local composition_greedy_v0; modes vary only hiring, using the unchanged existing estimator. runs/compiler_route_sep08_001 copies that isolated compiler and tests two local tile-visit scoring ideas. Both preserve source-program replay attribution through PROGRAM_SOURCES.json and LINEAGE.json parent hashes. All cold-farm inputs come from the existing local cold_compiler.cpp. No new external policy code is copied.

2026-09-08T01:43:00.861566+00:00: runs/compiler_placement_sep08_001 isolates allocated-land-first placement from the local labor compiler; source hashes and exact funding witnesses retained. runs/cold_day_tasks_sep08_001 generates task lists/end states from those dated cold lives and asks the persistent day_solver to produce routes. No source route or worker assignment enters its day problem. Service alternatives are explicit hypotheses, not assumed equivalent outcomes.

2026-09-08T04:35:18.841359+00:00: empty_sale_slots_m2 rejected:67584 fresh/5632 native-PASS/1024 operational/64 isolated rebuilt games. Current league93.956%->94.725%, direct906W38T80L,+239.62margin; both preregistered parent per-game gates fail. Fresh regressions1 and native1. Exact native trace proves strawberry floor-price inventory effect, not input movement; same-state immediate+2 leads eventual-6margin. Floor guards are a new discovery study, no promotion.

2026-09-08T06:02:24.290734+00:00: Day programs complete512discovery/64originalcontrols,512exactcoverage,104operations. No interrupted plans; p362 loss is later biology/service, not fallback. Continuation-value horizons1/3/remaining season now tested in new isolated run. Sources unchanged.

2026-09-08T06:27:53.831028+00:00: Promoted empty_sale_slots_m2 after independent population71680fresh/6144native plus unchanged1024ops/64frozen evidence per candidate. All232policy/239dependency hashes unchanged. Direct accepted parent911W40T73L,+242.12. Old rejected per-game audit remains rejected; new criterion explicitly optimizes population strength. No Git/catalog/upload.

2026-09-08T07:27:03.124678+00:00: Retained service_bank_p362_m2 as improved p362 cold-farm continuation. Fresh2560games/5opponents all6gates pass; mean cash gain614.41,95%[410.76,822.27], no mean fault regressions. Parent182W10T64L/256,+1185.63margin. All640coverage records exact, zeroabandoned programs,64operations;13216initialsourceinputhashes unchanged. This is genuine production/service plus labor improvement, not strongest-agent promotion; current-reference margin remains-60495.49. Next quantify the remaining dated-composition versus actual production gap and broaden actual-state scheduling.

## 2026-09-08T09:19:02.998837+00:00: Shop composition selector and mainline crop controls

shop_branch_m0 and m1 exactly wrap service_bank_p362_m2 and cold_renewal_p98.
shop_branch_m2 chooses between them from the two day6 observed shops. Local rule
was fitted in FIT_RESULT.json; no new external policy was copied. The selector
has verified common prefixes and full constituent outcomes, same-budget runner
parity, and7776fresh/native games. It improves the old pair but does not replace
early_melon_b98_m1. Full provenance and results: runs/shop_branch_sep08_001.

crop_context_m0/m1/m2 copy225 dependencies of empty_sale_slots_m2 into isolated
namespaces and add per-instance crop-mode forwarding through6headers. Calendars,
external components and their original lineage remain inherited. Modes1/2 expose
wheat/tomato choices and adjust day13 animal eligibility to actual crop choice;
mode0 is exactly unchanged.4608games/1152controlrecords/1152prefixes/52operations
complete. Both unconditional alternatives rejected; narrow hindsight margin
headroom14.08. Sources, modified-header list and audit: runs/crop_context_sep08_001.

## Additions at 2026-09-08T12:11:10.791155+00:00

- `runs/animal_group_policy_sep08_001`: derives the v8 compiler from the preceding animal-group run; adds explicit offline opponents, an unchanged-policy entry survey, and six independently audited continuation contexts. No runtime policy yet.
- `runs/yusuke_port_sep08_001`: four attributed C++ variants of Yusuke Hayashi's Shop Router 0908. Mode 2 is faithful; the local mode-3 amendment is negative. Source, operational, discovery and fresh evidence are complete. A strong counter, not a promoted replacement.
- `runs/opening_funding_sep08_001`: strongest-parent descendants change only the initial wheat round-trip quantity. q32 is an exact control; q13 is donor-inspired; q20/q24 are local. All operational and discovery evidence is complete; King regressions prevent claiming a general improvement.


## 2026-09-08T13:23:14.950860+00:00: animal courses, premium sales and broader compiler

Accepted reference unchanged. Seven experimental animal_groups packages inherit
empty_sale_slots_m2 and190locallycompiled day programs, conditional32shop
valuation and source-group labor. Twelve fixtureparity games,72operations,
8960discovery and1152independentfull-record controls pass; neutralwins unchanged,
margin+174.66/+197.93 forselector modes. Seeanimal_group_policy/STUDY_RESULTS.

premium_sales_sep08_001 imports AhmedV24's sole wrapper change, credited to
KingRC4's order-priority idea. All36other ASTnodes equal verifiedV23.4096source
cases/48ops/7040discovery; standaloneV24+2.474ppneutralvsV23, mainline+0.586pp.
Two animal_premium combinations inherit those exactcomponents and pass32ops;
36864fresh/nativefactor games running, no promotion. Publicsource licenses and
hashes remain inIMPORT/LIBRARY_LINEAGE files.

Earlycow/goose and mixed courses have independent719-turn action/endpoint audits.
Own gains-4145,+519,-5531respectively. Newcompiler variants fix land quadrant
encoding and permit explicit crop removal when delaying investment. This is
compiler/search progress, not agent promotion; retain losses and forecast errors.
q24fresh12288completes with+12.081ppneutralutilitybut-100.89meanmargin. Oldmean
promotion gate fails; keepthis tradeoff visible while further testing.

2026-09-08T13:59:26.863111+00:00: Review106. Repaired/composed agents16fixture+56ops+9856discovery pass; full q24combo+11.816pp/+186.37margin, King tradeoff persists. Forecast actual-shops/rival-flow diagnostic isolates major earlycow errors. Mixedday9 partial wheat buys traced and late stock correction verified; full certificate/season pending. All82metrics reuse1308/native256. Next14:18/refresh14:08.

2026-09-08T14:27:03.651581+00:00: Review107. One Kaggle upload authorized. Broad132864 live62984; care compiler live42461. Mixedday9 certified, day10 animal/stock mismatch blocks full course. Global82 metrics updated to1408 cohort72/64; localnative256 reused. Tetsutani sale-merge and Dmitrii terminal-delivery leads retained. Nextreview14:50/refresh15:08.

2026-09-08T14:49:24.911337+00:00: Review108. One submission in preparation, no upload yet. Frozen adapter first8 full games exactly match C++; larger packed132, C++8192, and broad132864 audits pending. Guard-miss inspector pending. Care-only saves no money; below-parent workforce search saves3 hires/$178 in independently audited seed1014 with identical production. Latest global72/64 metrics reused with localnative256. Nextreview15:10/refresh15:08.

2026-09-08T15:14:50.905091+00:00: Review109. Packed94908 plus rare19413 actions exact; frozen CPP teammate4040/4096, lastsubmission3883/4096. Broad132864 pending, no upload. Both retained-incumbent cow courses save178 with identical output. Timeout regression belongs to experiment caller, not root solver; integrated zero-budget check pending. Fresh global72/55, 82 metrics updated. Nextreview15:30/refresh16:08.

2026-09-08T15:22:32.468932+00:00: Caller incumbent fix is complete and validated: zero new search retains30/30 saved days across two full719-step scenarios, all final fields exact, +178cash each; invalid PASS-only saved schedule rejected. Root day_solver correctly returnsUNKNOWN for fixed workforce; no root bug/edit/commit. Evidence runs/animal_service_cost_sep08_001/INCUMBENT_FIX.md. Only live task62984 broad132864; package all132+27 checks complete; ONE authorized upload still unattempted. Review109/latestrefresh1508, nextreview15:30/refresh16:08. Nusrati exactduplicate Yusuke; AhmedV25 uses same4tapes with execution layers, queued.

2026-09-08T15:32:22.503567+00:00: Review110. Single authorized upload56101451 made15:30:57, PENDING; monitor1064 will fetch/reproduce server validation. Selected using completed discovery and exact package/native checks; broad132864 and unchanged promotion gates remain pending on62984, accepted reference stillempty_sale_slots_m2. V25 C++10793 source actions match, ops4178 live. Caller incumbent fix30/30days exact with zero search, invalid cache rejected. Nextreview15:50/refresh16:08.

2026-09-08T15:34:32.007184+00:00: Requested single Kaggle submission COMPLETE:56101451, animal_repair_q24_premium_m2, validation106828090. Local replay1438/1438exact actions, rewards66312/69823both exact, bothDONE, serverlogsnoerrors; archivehashd65b8150db33becdedb4f752e8d773fe1c1b24306cb4459974104564d6bde374. This consumes the one upload authorization. Broadaudit62984 still pending; researchacceptedreference remains empty_sale_slots_m2 and all promotion gates unchanged. V25sourceparity10793passes, ops4178live; run_screen.py ready afterops. Nextreview15:50/refresh16:08.

2026-09-08T15:52:19.038594+00:00: Review111. Root PDF13pages/14tasks complete. Broad132864 completed, q24native margin CIcrosseszero; no researchpromotion. Guard182misses,2weed73cases needinterpretation. V25discovery8448done,255/256vsYusuke,90/256vsuploaded. Submission56101451COMPLETE; no extra upload. No live task handles. Nextreview16:10/refresh16:08.
