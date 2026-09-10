# Strategy-search learnings

Initial evidence from the September 7 archive and root estimator:

- Whole-course placement has been weakly explored: early greedy distance/quadrant choices and later nearest-first quadrant additions.
- An owned-first layout funded earlier sheep in a weak farm, despite more movement. This is placement/finance evidence, not strong-policy proof.
- Six single wheat locations and two pairs found no improvement. Added production could be outweighed by discrete worker cost.
- Existing artificial day permutations show layout can change best-known labor, but cannot be applied independently to a live farm. Their biggest mean gain is dominated by one case.
- The $3,804 wheat saving came from scheduling on unchanged tiles; it is not placement gain.
- No new placement gain has been established in this experiment yet.

## 00:30 UTC evidence intake

Positive:

- All 43 downloaded games match the C++ engine through all 720 recorded states. Forty-eight selected player courses are available, eight from each of six current strong teams.
- Animals sit nearer shed access than crops in all six sampled teams. The mean radii are 1.71–2.91 for animals and 4.33–4.66 for crops.
- Broad crop/animal roles usually persist (86–95% of used tile histories), while species often change (only 21–33% retain one species). Stable zones and multi-crop tile chains are both reasonable proposal families.
- Nonlabor accepted market slots can be preserved explicitly when constructing the optional hiring menu.

Negative / limits:

- Thirty-seven of 48 courses have all daily physical contracts supported; 11 contain endpoint mismatches. Inspected cases discard stock in the source game, but retain it in the physical replay. They remain part of coverage accounting.
- Repeated or varying replay layouts alone cannot distinguish online placement from selection among stored plans. Seven of eight full work calendars repeat in each of two sampled teams; do not count those games as independent strategy families.
- Original physical day contracts omit some accepted economic orders. Using their apparent empty slots for hires can overwrite timed sales or round trips. The new adapter reserves slots from the executable source orders.

## 00:43 UTC first exact controls

Positive:

- A nine-second C++ search evaluated 2,848 consistent season layouts with peak RSS 9,336 KiB. Its best predicted bill fell from 12,552.9 to 9,547.6; this is an estimator result only.
- Full engine replay verified the original source and a re-solved unchanged layout through all 719 transitions. The latter saves 183 coins (12,691 to 12,508 hire bill), with identical production, sales and shops. This is workforce-search gain, not placement gain.
- Only 24–32% of productive worker-days cross quadrants in the six teams; 24–34% of field actions belong to those workers. Quadrants are plausible search blocks, with a material shared scheduling boundary.

Negative / limits:

- The best estimated layout certified only 12 of 30 days under two seconds per day and 0.2-second queries. Good estimates alone do not provide complete plans.
- Cow-versus-sheep distance differences by revealed demand are not consistent across teams. The samples also mix dates and compositions. Test controlled swaps for actual headroom before fitting any conditional rule.
- Candidate hire positions must preserve the economics of all accepted market slots. Full engine validation remains necessary even when daily physical replay passes.

## 01:10 UTC controlled results

Positive:

- One full-game verified placement gain: cow 25 / sheep 36 swap in DeeperNet saves 199 coins versus the equal-budget unchanged-layout control (3,326 to 3,127 hire bill). Both locations have radius 2; route neighbors matter beyond distance. Days 17, 25 and 29 save one worker each.
- All 384 untouched repair controls stay unchanged. Fixed-shop weed recovery improves from 222/384 with a worker queue to 242/384 after absorbing overdue PASS commands.
- Advancing planned sales can restore funding: fixed-shop complete cash-fault recovery improves from 8/384 to 125/384.

Negative / limits:

- Only three of ten selected placement interventions have complete certificates; two of those are worse than their controls. UNKNOWN is a search result, not infeasibility, but coverage is currently a major practical limitation.
- DeeperNet and mtmr_s1 strongly overlap with a public tape and must not count as independent successes.
- Naive sale advancement can harm incomplete plans: fixed-shop cash faults include 67 losses versus no repair, worst 21,569 coins. Restoring one purchase does not secure the remaining plan's funding.
- Native weed changes can alter future shop RNG through empty-tile counts. Keep native and fixed-shop results separate when attributing effects to repair or placement.

## 01:30 UTC speed and transfer

Positive:

- Immutable-course/day estimator memoization gave bit-identical predictions on 4,736 layouts, reducing measured evaluation time by 38.7% (14.97 to 9.18 seconds). This is not yet an end-to-end search-speed claim.
- The cow-25/sheep-36 motif and schedule bank produce fully replayed plans in all 16 related DeeperNet/mtmr_s1 games, for both original and swapped layouts. Every DeeperNet swapped bill is lower than its bank-only original-layout bill.
- The cheaper swapped schedules do not directly transfer back to the original discovery layout.

Negative / limits:

- Some swapped transfer cases spend extra solver time while the original finishes from the bank. Match that refinement budget before calling this wider placement-quality evidence.
- Donor species/calendar matching can violate timed deposit requirements. An inspected donor has proved missing required quantities on two days.
- Quadrant population search and global population search each win some predicted comparisons; there is no clear dominance or new certified win yet.

## 02:08 UTC controls, dated placement and coverage

Positive:

- Matched extra day-solver budgets preserve the cow-25/sheep-36 gain on all eight DeeperNet courses: seven save 199, one saves 144. The related mtmr_s1 results do not establish a general motif.
- The same motif can be chosen on day 7 before either animal is placed. The dated permutation passes 30 daily certificates and all 719 native/fixed-shop engine transitions at hire bill 3,127. Attempting to move an existing animal and assigning duplicate locations are rejected.
- A versioned source adapter records each item's shed discard and adds it only to the unbounded physical endpoint. All 48 courses now have 30 valid physical contracts, restoring the 11 previously unsupported courses. Actual source endpoints remain separate and full-game validation is still mandatory.
- Cash lookahead guards reduce worst fixed-shop loss versus queue-only execution from 21,569 to 3,762 on the same fault corpus.

Negative / limits:

- After matched refinement, mtmr_s1's swap wins once (+55), ties twice, and loses five times (55–377). Additional day solving explained several earlier apparent placement wins.
- Cash guards reduce complete fixed-shop recovery from 125/384 to 83/384 (next-day guard) or 85/384 (full-plan guard). They still lose final cash on 26 or 24 faults. A successful physical repair and an economically useful repair are different claims.
- Equal-radius population and four-move dated searches produce estimated improvements on six development courses. Exact constrained certification is running; those estimates are not new placement gains.
- Day-boundary permutations do not yet support moving only the next crop lifetime after an intraday harvest. General lifetime and cold-composition compilation remain unfinished.

## 02:49 UTC cold compositions and constrained-search failures

Positive:

- Individual crop/animal lifetimes now compile to daily physical contracts, with same-day harvest/replanting, derived weed clearing, compatible structure reuse and fixed land purchases. The compiler starts each day from the actual own farm.
- The first cold panel completes 44/46 courses across six compositions and two weed settings against live PASS. Both failures are the diagnostic row-order expanding farm's terminal day. The nearest and animals-first controls and all searched cold candidates complete.
- Crop-rotation placement saves two coins of hire cost (11 to 9) in both weed settings. This is a real but small absolute gain on a synthetic composition.
- A growing three-quadrant mixed farm completes with 1,876 hire cost and 63,853 final cash under animals-first placement on the initial native seed. This is compiler/pipeline coverage, not an agent-strength claim.
- Canonicalizing identical lifetime specifications gives exact predictions on 432 legal assignments across six cases. The revised search takes 4.38–16.14 seconds per case instead of 30, while its unrestricted local-search predicted cost is equal or lower in all six cases.
- Idle preclearing with existing planned DIG and hire-spawn guards leaves all clean controls intact and recovers 280/384 weed faults completely, versus 242 for queue/PASS absorption.

Negative / limits:

- None of the 18 larger constrained warm proposals has a complete certificate under the frozen budget. The six original-layout controls are all complete. Equal-radius and dated moves did not solve estimator-led search's certification problem.
- Cold estimated improvements often fail to improve certified cost. The expanding farm's estimated local improvement costs 1,903 versus 1,876 for animals-first. Geese cost 49 versus 46; several smaller changes tie. Keep the strong simple rules in the candidate portfolio.
- Search v2 changes traversal order as well as removing duplicates. One equal-radius mixed-animal prediction is slightly worse, though the unrestricted result improves. A no-quality-loss claim still needs full exact comparisons.
- Proactive clearing initially changed two untouched cases. Avoid clearing a weed already handled by a planned DIG and avoid moving a helper while a hire could spawn elsewhere. The corrected fixed panel passes all 48 course controls.
- Even corrected preclearing has four fixed-shop cash losses versus queue-only (worst 461), and negative mean native cash despite more complete recoveries. Cash forecast guards still have losses and are not promoted.

## Objective observation

With fixed successful sale quantities/times, fixed shops and a fixed opponent continuation, placement's monetary benefit comes from hire savings; output price alone cannot create an extra benefit if every obligation is already satisfied. Demand-aware placement needs either a route-cost interaction through service/delivery deadlines, funding risk, or a financial planner that can value changed delivery times. The financial-calendar adapter is being added to test the latter explicitly.

## 03:28 UTC peak interventions and causal controls

Positive:

- All 96 fresh-seed live-public-router courses complete, and their resource calendars agree with the full engine through 719 transitions. This covers two seats, six compositions, rule/searched layouts and dawn/immediate-goods finance.
- Strict schedule reuse compiles familiar rule layouts in roughly 0.5–0.8 seconds including loading, commonly with no new solver queries. The wheat bill falls from 33 to 14 through cross-day schedule reuse; this is a scheduling gain, not placement.
- The new lifetime probe set contains 3,212 legal, distinct interventions. Of 32 selected probes, six have a strict 13-worker schedule on expanding-farm day 20. The unchanged control remains UNKNOWN at 13 and 12 workers after 30 seconds each. Successes include a crop-chain move, two animal swaps, two animal moves and a quadrant-count change.
- The original and generated unchanged point contracts have identical nonlabor content on both peak days. Every full-course comparison receives the new feasible point schedules, so transferable route discoveries can strengthen its control too.
- A shared-shop evaluator now supplies only currently revealed shops to both live policies. Its first complete live-opponent course and financial replay agree through all 719 transitions.

Negative / limits:

- No selected probe has a 13-worker day-24 schedule within six seconds. None of these point successes is yet a whole-course placement gain.
- Several successful day-20 interventions have worse predicted costs, while the best predicted quadrant choices remain UNKNOWN. Estimator ranking alone is not a reliable certificate selector in this sample.
- Delaying dawn orders by one hour, allowing more early hires, still finds no 13-worker control on either peak after 30 seconds. This is only a bounded finance-order control, not a proof that timing cannot help.
- In the shared-shop smoke calendar, only 30 units reach the shed before market during the day versus 1,014 at day end. These routes give an immediate-sales policy little intraday supply to exploit.
- Reuse benchmarks must charge bank loading and prior certificate construction separately. A familiar-layout versus unseen-layout speed comparison is not an equal-budget placement-quality comparison.

## 04:02 UTC complete interventions, demand rules and reuse speed

Positive:

- All seven complete expanding-farm comparisons and their financial calendars verify. Against hire bill 1,876: crop-chain intervention 1950 costs 1,643; animal swaps 2717/2759 and quadrant intervention 3125 cost 1,664; animal move 2757 costs 1,698. These are development headroom results.
- All 188 fresh native/shared-shop live-public-router courses and calendars complete. Hiring differences persist in all four native continuations per candidate. Mean full-course process time is under one second with the expanded frozen bank; maximum worker RSS is 53,272 KiB.
- Two small rules were frozen before inspecting eight reserved shop sequences. Selecting swap 2717 on day 4 when milk demand is already positive yields 852.375 mean held-out own cash gain, versus 535.25 for always swapping. Selecting swap 2759 on day 10 with milk demand at least two yields 494.75, versus 482.5 for always swapping. These remain separate binary decisions on one composition.
- The C++ runtime executes those decisions at the actual observation time, forbids changing earlier lives, and exactly matches all sixteen selected-branch cash/bill/opponent outcomes. All sixteen calendars verify.
- All 48 new-input courses and calendars complete. Adding 0.1-second refinement to reused day schedules removes the one- and two-coin regressions from skipping refinement. Across twelve inputs it gives equal or lower bills individually, 1,806 versus 1,826 total, in 167.53 versus 334.07 process seconds.
- A 422,757-byte packed bank preserves all 390 checked lookup outcomes, including 99 returned strict-valid witnesses. Loading falls from 0.482 seconds to 0.0027. All six malformed-input checks reject and a packed round trip is byte-identical.
- In 36 paired full-game runs, the packed and text banks give identical bills and both players' cash. Mean process time falls from 0.6906 to 0.0570 seconds; medians are 0.7376 and 0.0524 seconds. The bank construction cost remains separate.

Negative / limits:

- Animal move 2727 improves the target peak but raises the total bill to 2,147. Quadrant intervention 3125 saves 212 hire coins but loses 731 final cash on the original shared-shop test. A peak gain and a hire gain each require their own whole-course checks.
- Shared-shop cash differences change sign, despite identical biological output and stable hiring differences. The delivered/discarded mix matters; prices do not justify a simple distance-only rule.
- The held-out early-swap rule still selects one losing outcome, and both eight-scenario bootstrap intervals include zero. No independent composition-family claim or universal demand rule is warranted. These swaps move a cow farther away, not closer.
- Lifetime-aware Manhattan repair has not repaired the tested changed crop-chain days. Regenerating hire times initially broke unchanged-course continuation; preserving the actual source hires restores all thirty identity days with no queries.
- A weaker two-second query allocation compiles the known crop-chain candidate at bill 2,918 without its discovered point witness. Better search allocation and retaining the original complete incumbent are essential; a successful earlier experiment is not automatically rediscovered under a smaller budget.
- On new inputs, estimated local search worsens several complete layouts, including wheat-9, wheat-17, strawberry rotation and the 32-wheat expanding farm. Keep simple rule assignments in the portfolio.

## 05:02 UTC bounded search, service calendars and route constraints

Positive:

- The lifetime day cache and one-time assignment validation preserve all 4,608 distinct-assignment scores across eighteen inputs. Total time falls from 10.05 to 5.33 seconds; the identical 3,266-probe growing-farm set takes 4.28 rather than 11.57 seconds.
- All 24 revised bounded-search runs retain a complete verified input or candidate. The maximum measured internal budget overrun is 0.0035 seconds. At both 90 and 180 seconds, search adds 1,803 cash on expanding-36 while fixed refinement adds zero; both add 650 on expanding-32 and one on the low-care plan. The other three cases tie. These are offline development scenarios, not an online policy generalization result.
- All 24 fertilized/sparse-service courses and financial calendars complete. On the eight unchanged rule assignments, frozen-bank reuse plus 0.1-second warm refinement preserves every bill and both players' cash; total compile time falls from 170.26 to 75.32 seconds, with total bill 388 in both modes. Bank construction remains separate.
- All sixteen reserved replay courses pass exact source parity, thirty daily contracts and 719-transition identity replay. The existing swap selectors are being evaluated without changes.

Negative / limits:

- A geometric screen based on every existing productive action's hour admits zero nontrivial single swaps out of 900 per course on all six development representatives. Preserving every source route for the whole season is too restrictive. The screen itself is fast (0.10–0.17 seconds including source loading), but it supplies no improved placement.
- Estimated local search improves none of the eight new service-calendar bills and worsens three: fertilized wheat-14 by four coins, aligned sparse tomatoes by eight, staggered sparse tomatoes by two.
- Team-level data reservation did not establish independent strategy families. Reserved teams match public tapes heavily; maximum cross-team full worker-turn matches are 98.75% for Himanshu, 94.02% for que la cuenten como quieran, 95.13% for Squirrel and 85.95% for デワンシュ. Keep these results as related-family transfer evidence. Final reserved teams remain unused.
- The revised search no longer finds the first panel's additional expanding-32 placement gain within 180 seconds: a late feasible peak proposal times out during full compilation. Initial refinement avoids the earlier 90-second regression, but search allocation still changes which improvements are discovered.

## 06:05 UTC realistic contracts and reserved swap results

Positive:

- Ordered daily service imports 47/48 development replay plans with exact biological obligations. One plan remains explicitly rejected for within-day overripe wheat decay. A known source assignment prevents failed greedy initialization from discarding feasible inputs.
- Preparation repair passes thirteen targeted checks, including ordered insertion/removal, false output rejection and the shortened final day. It completes realistic DeeperNet and mtmr_s1 plans where exact-key reuse fails. All completed real-input courses pass independent 719-transition financial replay.
- The fixed-finance cache agrees on all 1,536 distinct assignments from six real inputs. After the order-prefix correction, total checked evaluation time is 11.40 versus 8.71 seconds with caching; these process timings include the strengthened contract work.
- Five tiny budgets return the supplied complete incumbent or a verified improvement; zero budget makes no solver call. A corrupted executable input is rejected with no accepted result path. Results use atomic replacement. Near-zero budgets still pay 16–26 milliseconds of copying/setup internally in this test.
- Ordered sales/buys expose a precise contract correction: selling two fertilizer before buying one back requires two delivered units, despite a net withdrawal of one. The adapter now restores the maximum ordered stock deficit and matching canceled purchases. All 70,980 enumerated sequence/starting-stock checks pass. Full-course reruns are pending.

Negative / limits:

- On six real-input development representatives against a live public router, brief preparation repair completes four, full refinement completes five, and all six larger local placement proposals fail. Full refinement materially improves several bills: SpaTaro 2,960 to 2,413, Otter 12,691 to 12,453, DeeperNet 3,588 to 3,227 and mtmr_s1 3,596 to 3,361. Brief reuse is not a general quality-preserving replacement for full refinement.
- In the first 180-second bounded panel from the five fully refined real incumbents, placement search improves none; fixed-layout search saves another 288 on binghua. All ten runs preserve a complete verified incumbent. The placement queue exhausts in 59–83 seconds, so the next version spends the remaining budget on fixed-assignment refinement.
- Recorded-opponent reruns reproduce Mengfei's day-7/hour-6 failure. It is not explained by switching the opponent. Inspection identifies the lost sell-before-buy stock requirement above. Earlier complete full-game results remain valid; physical certificates alone had a weaker financial condition.
- Reserved cow/sheep selectors have poor full-game coverage: original-layout refinement 7/16, prediction-selected swaps 5/16, retained-route swaps 4/12 and uniform swaps 1/16. Against valid refined controls, prediction selection has one win/four losses (mean saving −219.4), retained selection one win/one tie/two losses (−5.25), and uniform selection one loss (−178). These are completion-selected subsets, not unconditional benefit estimates. Keep the verified original source when any new continuation fails.
- The original-layout reserved controls fail from five market-order and four inventory-endpoint violations despite all sixteen having complete physical day certificates. Team-level reservation also remains mostly related public-tape transfer. No universal species-distance, demand or placement-search promotion follows.

## 07:00 UTC frozen comparison, distinct-family extension and exact reuse

Positive:

- All twelve frozen final bounded runs preserve a complete independently verified incumbent. All six supported inputs tie between search and fixed-layout refinement at 180 seconds.
- The two distinct-family source courses now import exactly with version-four repeated services. Repeated fertilization consumes resources and must not be deduplicated. All 39 focused checks and 432 legacy assignment equivalence checks pass. This is a post-freeze extension; the original two unsupported cases remain in primary coverage counts.
- Final fixed-finance day-cache checks agree on all 1,536 distinct assignments from six supported inputs: 10.06486 to 6.86373 seconds, 31.8% less. The repeated-service extension agrees on 512 more: 2.85878 to 2.19956 seconds, 23.1% less.
- All 36 packed/text format speed runs now also pass an independent 719-transition financial check. The original paired bills and both players' cash are identical. Mean compile process time is 0.69060 versus 0.05702 seconds; medians 0.73756 versus 0.05240. Prior bank construction and the later independent check are separate from those timings.
- The recorded-rival adapter preserves raw invalid-item commands for the engine and their no-op effect for the financial ledger. The previously blocked corrected cooked calendar now verifies all 719 transitions. Own action validation is unchanged; the original frozen check remains recorded as a failure.
- An expected future stock shortage now writes an explicit incomplete summary. The saved Terry prefix completes 29 days with zero solver queries, then reports `insufficient_stock_day_29_item_1_missing_2`. This gives the outer planner a concrete deficit without accepting the prefix. All 70,981 ordered-stock checks pass.

Negative / limits:

- The frozen final method supports six of eight selected inputs after correcting one replay-encoding defect. Of 24 original-layout brief/full, recorded/live compilations, 16 complete in the engine and 15 pass the original calendar checker. The six outer endpoint exceptions, two market failures and one verifier failure are retained. A portfolio of complete live original-layout outputs supplies all six bounded inputs.
- The separate distinct-family bounded comparison loses to fixed-layout refinement in both cases. Fixed gains 650 and 89 cash; search gains 362 and zero. The 362 is unchanged-layout refinement. Search spends up to 105 seconds on a failing candidate compilation; another complete changed layout costs 199 more than its input. These are not placement wins.
- Both live continuations of the first repeated-service source fail with its fixed source trades, at day 5/hour 14 or day 19/hour 0. The recorded brief continuation completes while full refinement fails. More local refinement does not guarantee full-season funding or retained output.
- Final source-role fractions describe consistency within each season, averaged across two games per team. They must not be called cross-game placement agreement. About 94–95% of used tiles keep a crop/animal role, only 21–33% one species. Roughly 24–30% of productive worker-days serve multiple quadrants.
- The final name/family audit ties Blurry and Unknown Mother-Goose to the same team ID. Its worker commands are distinct from public tapes; Terry, cooked and pensukesan are heavily related. These team counts do not justify an independent-family confidence interval or the declared general placement-quality promotion.

## 07:30 UTC exact loading improvement and deeper negative placement check

Positive:

- The text loader now invokes the biological oracle once per identical complete lifetime specification. Input order, independent lifetime values and all validity checks remain. Direct fresh-oracle comparison agrees on every field of 15,715 lifetimes and 471,450 daily records across 75 plans; only 5,430 distinct oracle calls are needed. All 39 repeated-service/legacy rejection checks still pass.
- Eight real inputs take 190.371 milliseconds total to load biological plans before reuse and 81.725 milliseconds after, averaging five loads per input. This is a setup-only profile, not a whole compiler speed claim.
- A randomized three-variant panel performs 120 complete real courses and independent financial checks. Every paired executable action and outcome is identical. Combining the reused lifetime loader with packed banks reduces mean compilation-plus-verification time from 0.13190 to 0.10655 seconds and compilation CPU from 4.11 to 3.16 seconds over forty runs per variant.
- A build dependency audit finds only persistent root `agents`, `day_solver`, `fast_day_solver_estimator` and `fast_game_engine` resources. It finds no build dependency on another experiment and no broken checked document links. A compact source archive and runnable rotation example are being checked in a fresh staged build.

Negative / limits:

- Small per-course banks save much less time than the earlier 269-entry bank. In the first real format-only panel, total mean time falls about 16%; the later combined change saves 19.2% including the independent checker. Do not round that into the provisional 20% whole-wall target. Compilation CPU saves 23.1%, but that CPU column excludes the separate checker.
- The five-cap real development curve shows a nonuniform quality tradeoff. Caps 2 and 9 seconds each complete five of six, with common bills 27,041 versus 25,853. The two-second cap uses about 60% less total CPU, but its bill is 4.6% higher. Excluding binghua leaves a 0.6% bill increase; this exclusion is descriptive, not a rule selection. Smaller caps complete only four.
- A separate post-freeze 30-second diagnostic deepens the first two actually queried unresolved proposals per available case. Thirteen of fourteen candidate points remain UNKNOWN; one becomes feasible, and its unchanged-layout control is feasible too. Six other control points remain UNKNOWN. The sole candidate's full course verifies but costs 2,671, versus 2,404 for the unchanged layout with the same new witnesses. Larger query budgets do not rescue the placement claim in this sample.
- The newly feasible point finishes in about five seconds even though its earlier smaller-budget call was UNKNOWN. Solver budget can change search allocation as well as elapsed work; these are not monotone time-to-solution proofs.
- The cross-game comparison is also distinct from within-season persistence. The final Unknown Mother-Goose pair has 66.4% exact-species agreement over the active union and 96.6% crop/animal agreement where both are occupied. This shows variation while retaining broad roles, but does not identify its online placement mechanism.

## 07:58 UTC delivery checks and the cost of building a bank

Positive:

- The source archive builds in a fresh staged directory using only the four declared persistent root dependencies. Its rotation example completes thirty days with bill 14 and cash 14,246; the independent financial replay checks all 719 transitions. The zero- and five-second optimizer calls preserve it, and the five-second call rechecks the input.
- The delivery audit passes 340 hash checks, including all frozen final tools/launchers, all packed and staged source files, 61 current core files, result sources, the initial review and root runtime libraries. A separate dependency/link audit finds no other-experiment build dependency or broken checked document link.
- Three packing repeats on each of four banks (9, 30, 269 and 1,212 entries) produce twelve byte-identical existing banks. Mean packing plus strict lookup validation takes 0.028, 0.047, 0.661 and 2.290 seconds. Dividing this by measured per-load savings gives approximate payback after three, three, two and three loads respectively.

Negative / limits:

- A bank newly packed for just one evaluation does not recover its packing/validation cost in this check. Discovering the source schedules is still excluded and must be charged separately. Payback divides external packing wall time by internal load savings; it is an approximate load-only calculation, not a complete-search gate result.
- On the six frozen final searches, proposal generation uses about 0.5% of tool time and changed/unchanged point queries about 97%. On the two distinct-family extension searches, changed full-course compilation uses about 83%. More or cheaper proposals alone do not address either observed bottleneck.
- A deterministic node-budget reproduction of complete placement search was not implemented. Frozen seeds/binaries and wall/query budgets make the experiments inspectable, but do not make time-limited solver discoveries monotone or identical across machines.
- The final gate review retains exact reuse and the explicit lifetime/calendar interface. General placement quality, general repair and an end-to-end 20% search-CPU claim remain unestablished. The exact source package is a component handoff, not a submitted agent or persistent-package promotion.

## 08:06 UTC a small service-calendar follow-up, outside this placement contract

The two repeated-service inputs contain one and seven extra same-day fertilizations respectively. The engine sets the fertilizer expiry to the maximum of its current value and day + 2; reapplying on the same crop/day does not stack the biological bonus. `research/repeated_fertilize_followup.json` records all seven affected life/day sequences and source hashes. This gives a concrete candidate for a later fixed-placement service simplification study.

No changed course was compiled for this idea. Removing these operations changes required inputs, carried inventory and potentially overflow or later action timing; the current placement contract preserves those services. Eight redundant biological applications across two dependent courses are too little evidence to justify a broad service-calendar gain. Any later test should first remove them in a complete matched continuation, then investigate larger calendar changes only if useful headroom appears.

## Final eight-hour decision

The research window is complete. Retain the exact reuse and dated lifetime/calendar component in this experiment. General placement quality and repair gates did not pass. The clean source build and example passed; RESULTS.md, GATE_DECISION.md and artifacts/DELIVERY_AUDIT.json contain the final decision and verification scope. No unresolved run is being counted as a success, and no agent or persistent package was promoted.
