# Agent development: what worked and what to optimize next

Evidence snapshot: 8 September 2026, 16:42 UTC. This report distinguishes completed experiments, the submitted artifact, and proposed work. Monetary values are in-game cash; pp means percentage points of win score, with a tie counted as half a win.

## Main conclusion

**Your ideas worked in important parts.** Dated compositions are a useful search representation; adapting production to observed shops improves actual games; exact schedules turn some improvements into profit; and comparing estimates with realized play has identified correctable errors. The strongest result is much more than cheaper labor on the teammate's agent.

**The general version is still unfinished.** Our strongest agent combines proven replay-derived farm plans, locally rebuilt schedules, shop/opponent branches, and market execution. We have not yet demonstrated a general, reliable path from an arbitrary new composition through cheap valuation and placement to a strong executable agent. That remains the main strategic engineering gap.

| Current artifact or result | Evidence | Interpretation |
| --- | --- | --- |
| Submitted composition agent (q24 + animal groups) | Kaggle 56101451: COMPLETE; validation episode 106828090 | The requested single upload is finished. |
| Exact frozen C++ vs teammate | 4,040 wins / 4,096 games; 98.63%; mean margin +$11,793 | Strong measured head-to-head advantage on native game randomness, both seats. |
| Exact frozen C++ vs previous submission | 3,883 wins / 4,096 games; 94.80%; mean margin +$3,431 | A substantial advance over investment_context_guarded_001_best. |
| Actual Python package | 132 full official-environment games; 94,908 C++ action matches; 19,413 additional rare-branch matches | Packaging preserves the tested policy on these checks. |
| Actual server validation | Both DONE; 1,438 actions and both rewards reproduced exactly; no agent errors | The server ran an operational artifact. This is not a leaderboard-strength estimate. |

The **132,864-game comparison and guard/tail review are complete**. Submitted `animal_repair_q24_premium_m2` raises eight-opponent win score from 78.96% to 90.45% on independent-shop games and from 76.27% to 87.89% on native-RNG games. The native margin-gain interval still crosses zero, so the required promotion gate fails. Investigating the execution faults does not waive that numeric gate. The accepted development reference is still empty_sale_slots_m2. [S1-S3]

The most productive loop has been: **borrow or propose a coherent farm → estimate its complete marginal economics → compile all affected days → replay against a diverse league → explain the difference between predicted and realized results.** Preserve strong schedules throughout that loop.

Cheap valuation is promising but only partly general: successful fast selectors often consume trade/labor summaries of **already compiled** alternatives. They do not yet prove that arbitrary unscheduled compositions can be ranked equally well. [S4-S8]

<!-- page -->

## Ranked drivers of strength

This is an evidence-weighted judgment about practical contribution, not an additive attribution model. The experiments below use different opponent panels and seed sets. Their gains cannot be summed, and a large parent-counter gain can be small elsewhere.

| Rank and driver | Best evidence | What to retain |
| --- | --- | --- |
| 1. Strong complete farm plans, with selective replay/notebook borrowing | Warm-start descendants dominate the cold branch. The selected cold agent still lost 0-256 to both the main reference and teammate. | Borrow whole useful calendars and schedules; copy individual components when the donor's whole agent is weaker. This is foundational, without a clean isolated effect size. |
| 2. Market execution and opening interactions | Sale leading from step 216 gained +3.986 pp on its paired 32-opponent panel. Removing empty sale slots added +0.769 pp. Current q24/animal/premium gained +11.499 pp on the completed eight-opponent independent-shop panel. | Deposit-aware early sales, correct order slots, and bounded opening changes have produced the largest recent measured win gains. |
| 3. Shop-dependent production and crop calendars | Conditional berry fertilizer +1.845 pp; wheat-to-tomato branch +1.017 pp; productive wheat continuation +2.118 pp on their respective panels. | Choose what to produce from shops already seen, then rebuild the actual crop and feed schedule. These are real production changes. |
| 4. Exact scheduling and workforce choice | One controlled schedule saved 22 hires/$1,471 with unchanged output and trading. New cow courses save $411 in each of two contexts; broad discovery adds $13-$21 mean margin, no extra wins. | Minimize paid workforce while preserving complete physical and economic obligations. Reuse certified schedules. |
| 5. Whole-herd valuation and useful investment alternatives | Legal sampled late-investment selector gained +0.167 pp in broad confirmation. New animal groups added roughly $165 mean neutral margin, but almost no extra wins. | Value a new animal with its effects on existing herd revenue, labor, feed, crops displaced, and rival prices; retain wait. |
| 6. Public-opponent-dependent branching | Rival-wool v3 gained +0.285 pp across 28 opponents, entirely from better results against V5/2; other 27 result sets equaled its parent. | Opponent adaptation is real but narrow. Require a causal explanation and broader safeguards before extending it. |

The q24 result needs special care: on the new independent-shop panel, King win score falls 5.664 pp and mean margin falls $9,998 relative to the parent, although the candidate still wins 955/1,024 games. Primary-group margin gains $168 (95% interval $75 to $265); native-group margin gains $226 but its interval is -$5.89 to $465.50. This misses the declared positive-lower-bound requirement; it does not show a negative point estimate. [S2-S4]

**Biggest underperformer so far:** general construction from scratch. Its importance to the final goal is high, but its contribution to our strongest current agent is low. Do not confuse future potential with demonstrated benefit.

<!-- page -->

## What works best, and what does not

**Works: selective execution components.** V25 uses Yusuke's same four farm tapes. It wins 255/256 directly against Yusuke; neutral-seven win score is 85.27%, versus 91.18% for our submission. A separate 9,216-game, eight-mask ablation attributes +$1,906 neutral mean margin to earlier sales, with unchanged production and labor. The win gain is +2.46 pp, but its 95% interval includes zero; Junghoon loses 3.125 pp. Weed repair and final delivery add only about $8 and $22 respectively in the full policy. [S12]

**Works: economically meaningful branches with complete execution.** The tomato branch trades 36 wheat for 24 tomatoes in activated games; productive wheat adds six wheat; the berry service branch adds two strawberries. Each is useful because we also supply purchases, fertilizer, feed, deposits, sales, and later schedules. A counts-only substitution is not the finished change. [S3]

**Works: diagnosing forecast errors.** In an Atakan three-course test, actual future shops reduced mean branch regret from $3,249 to $475; actual flows and shops reduced it to $15.44. Those are offline oracle tests. A legal sampled-shop version gained 47 wins in 3,072 fresh games and $658 mean margin in that specialist family. [S5]

**Works: service as a strong default with profitable exceptions.** Two early cow cares created bonuses beyond the first-yield cap. Removing them alone saved $0. Workforce search plus retaining both original and new schedules saves $411 in each of two worlds, without losing production. In 13,824 discovery games, mean margin improves $21 with independent shops and $13 with native shops; primary win counts and investment choices stay unchanged. This is a small execution gain, not new adaptivity. [S6]

| Approach that disappointed | Evidence or mechanism | Better response |
| --- | --- | --- |
| Greedy construction judged mainly by legality | The initial 583 proposals / 36 exact candidates found no improvement; production and timing were lost despite legal actions. | Learn service, funding and route templates; track realized composition and failed obligations. |
| Unconditional extra animals | A previously tested extra goose produced 28 extra eggs and 16 fertilizer, but needed 8-9 extra hires costing $898-$987 across the remaining season. | Compare goose/cow/sheep/wait with displaced crops and discrete labor cost. This was an addition, unlike the separate profitable earlier-goose course. |
| Maximizing own cash or output alone | One crop forecast variant gained $15.18 own cash but lost $19.17 relative margin. Shared prices can help the opponent more. | Evaluate own cash, rival cash, win score and margin separately. |
| Over-optimizing a weak choice set | Hindsight could improve one existing two-leaf wheat/tomato selector by only $4.60 mean margin on its discovery panel. | Add stronger complete alternatives before refining that selector. |
| Treating a timeout as infeasibility | A repeated search lost known cheap schedules and turned +$178 into -$144 in one context. | Preserve and revalidate incumbents. Root day_solver correctly returned UNKNOWN. |
| Treating a notebook title or replay as proof of strength | Nusrati's notebook was byte-identical to Yusuke. Yusuke won 1,024/1,024 against the old main agent yet lost 6.795 pp on a neutral comparison. | Verify source identity, broad strength and component value. |

<!-- page -->

## What separates us from the very top

**We do not yet have a measured win rate against the complete current policies of every leaderboard leader.** A replay shows one realized game; even an exact reconstruction does not recover all the donor's decisions. Local league win rates and a just-validated submission cannot be converted into an honest leaderboard-rank claim.

At the 15:11 UTC leaderboard snapshot, SpaTaro led at 2,931.6, followed by Otter Vibe at 2,901.3 and Mengfei Li at 2,886.3. Our new submission has just completed validation; a meaningful mature rating comparison is not yet available in this report. [S9]

The clearest engineering gaps are:

1. **A limited set of strong farms.** We can choose several useful continuations, but cannot freely rebuild herd size, species, crops, land, and placement together at multiple dates. The weak cold branch shows how much the inherited complete farm contributes.
2. **Execution that is too dependent on expected states.** A weed on one wheat tile can invalidate several later day guards. Service may continue, but crop cycles and intended cash can be lost. More general continuation repair is unfinished.
3. **Incomplete forecasts of the opponent and future demand.** Current-herd projections omit some future growth, crops, fertilizer flows and trade decisions. The right response is a distribution over plausible futures, not pretending future shops are known.
4. **Vulnerability to opponent cycles.** q24 counters Yusuke but weakens the King matchup. A strategy that wins more often on one panel can have worse losses or margins elsewhere.

The latest top-player replay sample also suggests where to investigate. These are **unmatched** cohorts: 72 player records from 55 recent top-player replays versus 256 native games of accepted empty_sale_slots_m2, not the newly submitted agent. [S9-S10]

| Behavioral measure | Top-player sample | Accepted local sample |
| --- | --- | --- |
| Tomato harvested per game | 42.60 | 4.22 |
| Carrot harvested per game | 118.32 | 79.36 |
| Strawberry harvested per game | 229.69 | 249.02 |
| Average sale hour, recorded metric | 7.82 | 10.28 |
| Cow care fraction | 0.816 | 0.946 |
| Mean hire cost | $6,231 | $3,946 |

These figures do **not** prove that we should copy average crop counts, spend more on workers, or skip care at the same rate. They support testing additional crop families, earlier deliveries and economically justified service exceptions. The top cohort's larger labor bill may fund a better farm; lower labor cost alone is not the objective.

<!-- page -->

## Main bottlenecks in strategy search

| Rank | Bottleneck and evidence | What would improve it |
| --- | --- | --- |
| 1 | Multi-day compilation and repair. A mixed farm now passes all 30 endpoints, after repairs to day 9 funding and an inherited day 10 goose purchase. Physical routing alone had accepted a plan for an animal never bought. | Rebuild economic dependencies as well as routes; preserve valid days; return the first failed requirement instead of a generic failed candidate. |
| 2 | Too few strong, distinct alternatives. A polished two-leaf selector had only $4.60 hindsight headroom. Cold farms remain far behind. | Mine complete top-player families; search insertion/deletion, earlier investment, multiple tiles and dates, rotations, and new openings. Measure best new family found per compute budget. |
| 3 | Wrong marginal economics from shops, rival supply and labor. In one early-cow case, the labor-adjusted estimate predicted +$3,092 margin; exact play gave -$857. | Improve conditional distributions and rival-growth/flow models. Use forecast-versus-realized error and decision regret on many contexts, not one favorable example. |
| 4 | Difficult schedules and discrete workforce thresholds. Extra visits do not translate smoothly into cost. A 30-second attempt can return UNKNOWN for a previously solved day. | Keep incumbents, lower bounds and reusable route hints; search workforce explicitly; benchmark timeout coverage and p95 latency. |
| 5 | Validation cost and avoidable experiment overhead. The completed broad study has 132,864 games; a past repair study used 245,760. Tiny effects can consume many hours. | Use staged screens, paired seeds, frozen-result reuse, fewer redundant rebuilds and predeclared confidence targets. Complete the required package checks without letting optional research audits delay an authorized upload. |

**The main limit is not GPU availability.** Microsecond biology and sub-millisecond bounded valuation are already plausible on CPU. The expensive parts are finding useful alternatives, compiling them faithfully and obtaining enough exact evidence. We have no demonstrated GPU workload that improves this loop; the unrelated RTX 5090 training has stayed intact. [S5-S8, S11]

The early-cow error diagnosis is informative but not universal. Substituting actual future shops moved the estimate to -$1,125 margin; substituting actual own and rival daily flows moved it to -$1,072, versus -$857 exact. Those unavailable values locate errors offline. Their effects depend on substitution order and are not an additive causal attribution. A different earlier-goose course was already within about $82 of actual own gain after measured labor. [S7]

The completed mixed course adds 36 eggs and 44 wool but $1,563 labor cost, and displaces crops. Our cash falls $4,923; the rival's falls $4,956, leaving only +$33 margin in that one world. More production is not enough. Optimize league win score; use cash, margin, tails and component metrics to explain it. [S7]

<!-- page -->

## Separable tasks: how independent are they?

Yes. Several are excellent isolated optimization problems once their contracts are fixed. Others can be tested separately but need whole-agent confirmation because their outputs change opponents, prices, or future feasibility.

| Task | Isolation potential | Recommended role |
| --- | --- | --- |
| 1. Biology and service calendar | Very high | Exact foundation and cheap pruning |
| 2. Market and opponent-flow scenarios | High for arithmetic; medium/low for predicting behavior | Quantify uncertain future prices and rival effects |
| 3. Marginal composition scorer | Medium | Rank proposals without worker-level simulation |
| 4. Tile placement | Medium/high for a fixed dated composition | Make good economics physically affordable |
| 5. Fast workforce/cost estimator | High as a predictor; not a feasibility proof | Avoid compiling obviously poor proposals |
| 6. Exact day solver | Very high for its physical contract | Certify hourly service and deliveries |
| 7. Workforce and service-pattern search | High for a fixed day/course | Convert fewer tasks into actual saved money |
| 8. Funding, inventory and intraday trades | High with fixed flows; medium when joint | Prevent partial purchases and monetize production |
| 9. Incremental multi-day compiler | High for local edits; medium for arbitrary farms | Preserve compatible suffixes and expose exact failures |
| 10. Runtime state repair | High for bounded faults | Recover from weeds and missed actions |
| 11. Observation-driven branch selector | High with a fixed certified library; medium for general decisions | Choose species/count/date/wait and opponent responses |
| 12. Proposal generation / outer search | Medium/low in isolation | Escape one farm family and find stronger candidates |
| 13. League evaluator and diagnostics | Very high for correctness and speed | Make all claims cheaper and more trustworthy |
| 14. Deployment compiler | Very high for equivalence | Submit the same behavior we tested |

Start with **9 + 7 + 5**, supported by exact biology and the existing day solver: better compilation, workforce decisions and labor estimates make more of the composition space usable. In parallel, improve **2** and expand **12** from actual top-player courses. These are recommended work priorities, not assignments already given to other agents.

The following pages specify inputs, outputs, objectives, independent benchmarks and how each task contributes to the search. All playable decisions use only own/private-allowed state and public observations; actual future shops or opponent-private state remain offline diagnostic data only.

<!-- page -->

## Tasks 1-3: model what a composition earns

### 1. Exact biology and service-calendar model

**Inputs:** crop/animal species, placement and removal dates, age, held yield, water/fertilizer/feed/care state, and candidate service events. **Outputs:** dated harvestable output, wheat/fertilizer consumption, care banks, cap losses, escape and service-driven weed events, and required service windows. Random weed arrival needs a separate risk model.

**Objective:** zero disagreement with isolated engine transitions; then minimize batch latency. Test event order, immature end-of-game tails, output caps and last-useful-service dates, not just total yield. **Isolation: very high.** Dynamics are deterministic when service is fixed. Existing microsecond models and the cow-care-cap check show this is tractable. Its value is removing impossible or dominated proposals before routing, and generating the day solver's real work requirements.

### 2. Public-market and opponent-flow scenario model

**Inputs:** observed shops and market stocks/prices, both public farms and their histories, own planned flows, and assumptions or sampled beliefs about future shops, rival planting, services and sales. **Outputs:** scenarios for future quantities, price paths, rival receipts and uncertainty, with the assumptions retained.

**Objective:** exact shared-price arithmetic given fixed flows; calibrated forecasts and low downstream branch regret when flows are unknown. Track residuals by product, date and opponent, and distinguish model error from irreducible future-shop uncertainty. **Isolation: mixed.** Arithmetic is highly testable; future opponent behavior is only partly predictable and responds to our policy. This module prevents apparently profitable additions from helping the opponent more. Hindsight substitutions are diagnostic tests, never runtime inputs.

### 3. Fast marginal composition scorer

**Inputs:** dated composition alternatives including wait, current farm and capital, approximate placement/service/labor, and shared market scenarios from task 2. **Outputs:** estimated own cash, rival cash, margin and risk per alternative, plus feasibility flags and the main cost/output terms.

**Objective:** rank the best exact alternatives correctly under a time budget; minimize selection regret against fully realized alternatives. Revenue MAE and rank correlation are supporting metrics. **Isolation: medium.** A fixed library gives a clean supervised comparison; new farms depend on uncertain labor and execution. Benchmark both compiled-library choices and genuinely unscheduled proposals. The latter must not quietly receive exact future trade schedules. This is the gate that makes a wide outer search affordable.

<!-- page -->

## Tasks 4-6: turn dated work into routes

### 4. Tile placement for a fixed composition

**Inputs:** current grid and ownership, dated crop/animal lifetimes, service/output calendars, shed access, occupied tiles, inherited route templates and immutable constraints. **Outputs:** tile assignments and a transition plan, with travel/load estimates and a feasible or unresolved status.

**Objective:** minimize realized worker cost and missed delivery penalties while preserving the composition's economic obligations. Compare placements by downstream compile success and exact profit, not Manhattan distance alone. **Isolation: medium/high.** Fixed calendars are a bounded spatial search; earlier/later land acquisition and crop removal create coupling. Local swaps, clustered animal service and reusable motifs are promising. Full optimality is unlikely to be cheap, but useful improvements can be evaluated independently on frozen farms.

### 5. Cheap workforce and labor-cost estimator

**Inputs:** daily tasks and their windows, layout, worker start positions, carried stock/capacity, purchase times and required deliveries. **Outputs:** workforce lower bound, likely feasible count, expected hire cost, uncertainty and a reason when a day looks difficult.

**Objective:** low regret in composition ranking, especially avoiding expensive underestimates near another-hire thresholds; maintain high recall of promising feasible proposals at low latency. **Isolation: high as a predictor.** Use exact certified schedules as upper bounds, mathematical lower bounds when available, and retain UNKNOWN as censored search evidence. A timeout is not an infeasible label. The estimator must learn discrete worker costs rather than a smooth cost per visit. Its value is reducing wasted exact solves.

### 6. Exact day solver

**Inputs:** the complete start grid/state, explicit required work and service windows, fixed workforce and hires, purchase/land events, inventories, delivery or sale-availability deadlines, and required terminal tiles/stocks. **Outputs:** hourly worker actions with a strict replay certificate, or UNKNOWN plus timing/diagnostics. A before/after grid alone is insufficient: some services and temporary stock movements leave no unique endpoint signature.

**Objective:** maximize certified coverage on unseen representative feasible problems under a fixed budget, then reduce median/p95 latency. With workforce fixed, it is a feasibility scheduler; an outer optimizer chooses cheaper workforce. **Isolation: very high.** Root day_solver already implements this contract. It does not certify full cash, shed-capacity economics, price changes or opponent responses. The caller must replay those. No root bug was found in the recent timeout incident: the caller failed to retain earlier solutions. [S6, S11]

**Existing baseline:** V30 solved 323/323 exposed development days at 243 ms median and 423 ms mean; 242 previously slow days took 1.135 s median and 2.340 s mean. These are different cohorts, and fresh unseen V30 coverage is not established. Optimize unseen coverage and difficult cases, while preserving strict replay acceptance. [S11]

<!-- page -->

## Tasks 7-9: preserve money and the continuation

### 7. Workforce and service-pattern optimizer

**Inputs:** a day's or course's required output/end state, legal optional service patterns, hiring prices/times, and a verified incumbent schedule. **Outputs:** a cheaper certified workforce/service plan, or the still-valid incumbent when search cannot improve it.

**Objective:** maximize realized cash or margin improvement at equal required outcomes; charge actual hire costs and any altered inputs/production. **Isolation: high for bounded edits.** Enumerate cheaper workforce and biology-proven service exceptions, invoking task 6 where useful. All schedules must meet the current contract. Zero-new-search replay should preserve valid incumbents and reject invalid cached schedules. Two $411 cow-course savings now yield small positive league margin in discovery, with unchanged primary wins.

### 8. Funding, inventory and intraday transaction planner

**Inputs:** dated output and input needs, worker pickup/deposit availability, cash, shed and carried inventories, capacity, order slots, prices, current shops and opponent-flow scenarios. **Outputs:** executable purchase, hire, land and sale quantities/times, with a sequential cash/stock ledger and failed obligations if no funded plan is found.

**Objective:** maximize realized marginal profit or win surrogate while meeting prerequisites, storage limits and market-order priority. **Isolation: high with fixed worker flows; medium jointly.** Requested quantities may execute only partly. The mixed course needed replacement wheat on day 9 and a split goose purchase on day 10. Moving a sale can change its priority relative to the rival. Exact fixed-scenario replay is a good independent benchmark; uncertainty still requires full policy testing. This task turns theoretical yield into spendable money.

### 9. Incremental multi-day composition compiler

**Inputs:** current observed state, a dated composition edit, existing certified continuation, financial plan and reusable day/route templates. **Outputs:** a complete guarded continuation with precise entry/exit contracts and a dependency manifest, or the earliest explicit unmet requirement. Preserve old valid alternatives.

**Objective:** increase exact full-course compile success and realized value per CPU second; minimize unnecessary re-solves without accepting stale states. **Isolation: high for a fixed input corpus, medium for arbitrary farms.** Track every dependent seed, feed, fertilizer, output, sale and later guard. Recompile only the affected closure, then verify the entire remaining game. This is the highest-priority engineering bottleneck because it determines whether new composition ideas become usable agents. A single successful day is not a successful continuation.

<!-- page -->

## Tasks 10-12: make the policy adaptive and broaden the search

### 10. Bounded runtime repair

**Inputs:** actual own/public state, expected day contract, planned action and remaining obligations, with a strict decision-time budget. **Outputs:** a minimal repair, a compatible alternate day program, or a safe continuation with its expected lost obligations explicitly recorded.

**Objective:** recover realized cash/output and future contract validity with bounded time and no invalid API actions. **Isolation: high for defined disturbances** such as one weed, delayed planting, a missing input or worker; much lower for an arbitrary broken farm. Maintain an injected-fault corpus and compare against complete games. The tile 38 weed family is a concrete target: repair all compatible states. V25's component ablation now separates weed recovery from larger sale-timing gains; a transplant still needs testing inside our policy.

### 11. Observation-driven composition and investment selector

**Inputs:** current own/public state and history, a library of compatible complete continuations, and value distributions from task 3. Alternatives include species, count, dates, retaining crops and waiting. **Outputs:** the chosen branch plus its entry requirements and future decision points.

**Objective:** improve out-of-sample paired league win score with declared margin/tail constraints; reduce exact hindsight regret within the available library. **Isolation: high for a fixed library; medium for general decisions.** Verify common prefixes and input-state compatibility so a decision is actually reachable. Use only shops seen, prices and observed rival behavior. Do not optimize a selector past the available alternatives' small headroom. Its usefulness grows when task 12 produces stronger complete families.

### 12. Proposal generation and outer composition search

**Inputs:** current best agents, replay/notebook evidence with provenance, economic residuals, league weaknesses, and a compute budget. **Outputs:** diverse dated compositions and conditional edits, with cheap rankings, compile results, and reasons for rejection.

**Objective:** strongest newly validated complete agent found per budget, with coverage of genuinely different farms and matchups. **Isolation: medium/low.** Novelty or estimator score alone is not success; proposal value depends on compiler and league quality. Use family insertion/deletion, multiple investment slots, earlier starts, mixed farms, different crop calendars and cold construction. Retain a cold-search budget even while warm starts pay better. A clean independent test freezes the estimator/compiler/evaluation distribution and compares search methods on fresh rounds.

<!-- page -->

## Tasks 13-14: reliable evidence and deployable behavior

### 13. League evaluator, profiling and error attribution

**Inputs:** frozen C++ agents, public/replay-derived opponents and prior versions, seeds, seat assignments, budgets and a preregistered comparison protocol. **Outputs:** paired game records, wins/ties, own and rival cash, margins, lower tails, action/production/flow profiles, confidence intervals, and the earliest execution differences.

**Objective:** exact deterministic results and useful diagnostic coverage per wall-clock second. Statistical precision must match the decision being made. **Isolation: very high for correctness/throughput.** Compare generic and typed runners, thread counts, debug legality, native RNG and official environment behavior. Cache only complete results with matching source/config hashes. Track unused scenarios and avoid pooling unlike panels. This module distinguishes a real strategy gain from an opponent-specific counter, changed scenario, accounting error or packaging defect.

### 14. Deployment compiler and exact package verification

**Inputs:** the selected immutable C++ policy and its lineage, plus the allowed deployment interface. **Outputs:** self-contained submission artifact, source/data hashes, reproducible build, and equivalence/latency evidence for the actual packaged code.

**Objective:** preserve decisions and episode independence exactly while minimizing import/decision latency and artifact size. **Isolation: very high.** It is a correctness problem before it is a speed problem. Test source-only rebuild, both seats, separate instances, PASS/self games, relevant rare branches and the official runtime. Finally reproduce the server validation replay. The new submission did exactly that: 1,438 server actions and both rewards matched. This protects the value created by every strategy experiment. [S1]

## Interfaces that prevent wasted work

Every saved problem should identify the state, assumptions, mandatory obligations, objective and source hashes. Every result should say **certified feasible**, **invalid**, or **UNKNOWN**, with the verified schedule and residual obligations where applicable. Do not collapse these into a single numeric score.

For a new composition, retain both requested and realized lifetimes, output, purchases and sales. For a saved schedule, retain its exact entry and exit contract. For a predictor, retain the uncertainty and the exact alternative it is compared with. This lets different subtasks improve without silently changing what another component assumes.

<!-- page -->

## Concrete catch-up plan

| Order | Work | Evidence needed to keep it |
| --- | --- | --- |
| 1 | Use the completed broad review and V25 component ablation to choose useful transplants; preserve the King regression and failed native margin gate. | Re-test components inside our actual policy. Donor gains and submission success alone do not pass research gates. |
| 2 | Generalize the proven local fixes: tile 38 repair, retained workforce schedules, and mixed-course purchase funding. | All affected days and the full 719-transition game meet their contracts; gains survive different shops and rivals. |
| 3 | Expand the useful farm library from recent top-player courses: earlier/multiple animals, larger cow/sheep/goose groups, additional crop rotations and placements. | Complete funded alternatives with actual marginal labor, inputs, output and sale timing. Keep viable wait/crop alternatives. |
| 4 | Improve rival-growth and market-flow scenarios, then the cheap ranker. | Lower decision regret on unseen contexts and more exact good candidates reached per compute budget; no oracle leakage. |
| 5 | Search multi-stage branches and independent farms against a growing league. | Common-seed, both-seat gains against strong public agents and prior versions, with declared tail constraints and unused final scenarios. |

**What would count as catching up?** Sustained improvement on an independent strong-opponent population, demonstrated robustness to the public counters we know, an operational submitted artifact, and enough official evaluation to compare its mature rating with the live leaders. We cannot yet claim all of that, and there is no justified fixed-date promise of first place.

**What to stop doing:** interpreting a bigger herd as automatically profitable; treating all feed/care as mandatory; accepting output gains without realized sales; using timeouts as negative feasibility labels; tuning a nearly exhausted branch selector; repeatedly testing tiny changes without a decision-sized budget; and delaying an explicitly requested upload after its required operational checks have passed.

The original direction remains sound: search what the farm contains over time, estimate it cheaply, spend exact scheduling effort on promising choices, and improve both models and execution from their disagreements. The next advance depends most on making substantially different good farms executable and broadening the current policy.

<!-- page -->

## Evidence and reproduction

The source Markdown and PDF builder are in `work/agent_strategy_report_sep08/`. `BUILD.json` records the PDF hash, source hash and build command. The previous root PDF is preserved. Full paths below are relative to the repository root; abbreviated `runs/` and `research/` paths are relative to `experiments/v6/sep07_compositions_v0/`. This report adds no Git operation or submission.

**S1 - Exact submitted artifact and server evidence.** `submissions/sep8-composition-adaptive-v1/`: README.md, SELECTION.json, FROZEN_CPP_VALIDATION.json, packed_v1_validation.json, branch_validation.json, REBUILD.json, SUBMISSION.json, kaggle_validation/audit.json. Kaggle submission: https://www.kaggle.com/competitions/kaggriculture/submissions?dialog=episodes-submission-56101451

**S2 - Recent composition/market factor tests.** `runs/animal_repair_sep08_001/`: RESULTS.json, BROAD_RESULTS.json, FINAL_BROAD_REVIEW.json, BROAD_TRADEOFFS.csv, GUARD_REVIEW.json, broad_2360000/PROTOCOL.json. The final review has 159 paired comparison rows. Also `runs/animal_premium_sep08_001/FRESH_RESULTS.md`.

**S3 - Accepted mainline lineage and individual promotion reports.** `experiments/v6/sep07_compositions_v0/research/post_submission_lineage.md`. This is a historical snapshot: its old “last submitted” label is superseded by S1. It links the paired validation reports for all ten descendants.

**S4 - Public counter and cold-search limits.** `experiments/v6/sep07_compositions_v0/runs/yusuke_port_sep08_001/FRESH_RESULTS.md`, `runs/opening_funding_sep08_001/FRESH_RESULTS.md`, and `runs/early_melon_sep08_001/FRESH_RESULTS.md`.

**S5 - Estimator diagnosis and legal sampled-shop improvement.** `experiments/v6/sep07_compositions_v0/runs/atakan_oracle_ablation_001/README.md`, `runs/atakan_sampled_shops_001/README.md`.

**S6 - Care/workforce and caller fix.** `runs/animal_service_cost_sep08_001/`: LINEAGE.json, INCUMBENT_FIX.md and incumbent_zero_budget_v2/RESULTS.json. Final $411 certificates: `runs/animal_service_policy_sep08_001/retained_parent/RESULTS.json`. League discovery: `runs/animal_service_policy_sep08_002/RESULTS.json`; no promotion. Earlier gains: `handoffs/sep07_agent/docs/learnings.md`.

**S7 - Forecast errors and mixed-farm execution.** `runs/animal_forecast_error_sep08_001/`: README.md, RESULTS.csv, PROTOCOL.json. Completed mixed course: `runs/mixed_funding_sep08_001/`: README.md, RESULTS.json, AUDIT_PROTOCOL.json, day10/SUMMARY.csv and audit/MATCHED_RESULT.json.

**S8 - Selector headroom and historical learnings.** `experiments/v6/sep07_compositions_v0/runs/crop_choice_estimator_001/README.md`; `handoffs/sep07_agent/docs/learnings.md`; prior report source `work/agent_development_pdf/summary.md`. Historical numbers retain their original scope.

**S9 - Current top-player evidence.** `experiments/v6/sep07_compositions_v0/research/refresh_sep08_1508/`: timestamped leaderboard CSV, top_replay_selection.csv, REFRESH.json and invariant_means.json. Public leaderboard: https://www.kaggle.com/competitions/kaggriculture/leaderboard

**S10 - Explicit local/global behavioral comparison.** `experiments/v6/sep07_compositions_v0/research/review_109.md` and review_110.md. These contain all 82 metrics and the unmatched-cohort warning.

**S11 - Actual day-solver contract.** `day_solver/README.md`, `day_solver/include/day_solver/scheduler.hpp`, and `day_solver/docs/integration.md`. Physical schedule acceptance does not certify the complete economic game.

**S12 - Public source inspection and component ablation.** `research/refresh_sep08_1508/NOTEBOOK_AUDIT.md`; `runs/ahmed_v25_sep08_001/`: LINEAGE.json, SOURCE_PARITY.json, OPERATIONAL_CHECKS.json and DISCOVERY_SUMMARY.json. V25 matched 10,793 source actions and passed 24 operational games; initial discovery used 8,448 games. Separate eight-mask study: `runs/v25_components_sep08_001/`: OPERATIONAL_CHECKS.json (84 games), RESULTS.json, EFFECTS.csv and discovery_2400000/PROTOCOL.json (9,216 games).
