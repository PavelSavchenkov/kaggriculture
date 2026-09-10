# Search coverage and what each test established

“Tested” means the method was implemented and run. It does not imply an improvement or a passed promotion gate. All run paths below are inside this experiment. The broader initial idea list remains in `IDEAS.md`.

| Idea | Implementation / evidence | Result and limit |
|---|---|---|
| Preserve the original strong layout | `search_course`, `search_lives`, bounded panels | Essential baseline and safe fallback |
| Nearest owned cells, animals first, row order | `cold_certificates_001`, `new_input_panel_001` | Simple proposals remain competitive; row order can fail |
| Closed-form weighted radial placement | `include/search.hpp`: `radial_frequency/actions/io/shadow` | Rearrangement solves the separable distance proxy; no general routing optimum or complete-course gain follows |
| Service frequency versus number of actions | `search_course`, initial search panels | Both implemented as separate weights; large rearrangements often fail certification |
| Input/output-trip weights | `Course::input_output`, radial rules | Useful proposal feature, no promoted universal ordering |
| Expensive-day hiring threshold weights | `Course::shadow`, radial rules | Uses marginal Fibonacci hire cost; does not capture shared-route feasibility |
| Cow/sheep swaps retaining both animals | `probe_swaps`, matched transfer and validation panels | A 199-coin development gain exists; transfer has substantial losses |
| Equal-radius swaps | Warm, constrained and lifetime searches | Radius alone misses route interactions; bounded certification remains necessary |
| Same-quadrant local search | `swap_search`, `search_lives` | Estimated gains are frequent; certified gains are much less reliable |
| Simulated annealing | `swap_search(...annealing=true)` | Explored beyond greedy improvement; no broad accepted gain |
| Donor species/service motifs | `donor_layout`, `portfolio_search_001` | Hungarian matching makes legal donor proposals; no extra certified general gain |
| Evolutionary donor combinations | `population_search`, population 24/four elites | Quadrant crossover and local mutation implemented; predicted improvements do not establish quality |
| Quadrant block search | `quadrant_population`, constrained panels | Implemented with global daily scoring; no independent-quadrant optimum claimed |
| Cross-quadrant future-lifetime placement | Dated search and lifetime probes | Ownership/continuity enforced; broader search often unresolved |
| Change the number of lives assigned to each quadrant | `probe_lives`, intervention 3125 | Full season saves 212 hiring coins but loses 731 cash in the original scenario |
| Whole crop-chain moves | `probe_lives`, intervention 1950 | Full-season hire bill 1,876 → 1,643 on one growing composition |
| Individual animal moves | `probe_lives` | One saves hiring; another improves the selected day but worsens season cost |
| Preserve future rotation slots | Lifetime occupancy and release checks | Exact legality feature; no teleporting active crops/animals |
| Dated decisions before placement | Warm day-7/day-8 checks; conditional driver | Valid pre-placement swap works; attempted movement after placement rejects |
| Demand-aware conditional swaps | `conditional_live_001`, `probe_live_001` | Interventions certified before fitting; small held-out examples, intervals include zero |
| Value changed delivery/overflow through finance | `export_calendar`, dawn/immediate sales panels | Same production can yield different retained stock and cash |
| Route geometry preserving all productive hours | `search_route_geometry` | Zero nontrivial candidates out of 900 per each of six sources; too restrictive |
| Retain source actors and hire timing | Route repair tools | Necessary for identity replay; regenerated hires initially broke controls |
| Manhattan repair of changed lifetime routes | `life_route_repair` | Did not repair the tested changed crop-chain days |
| Repair obsolete/missing preparation | `preparation_repair`, 13 focused checks | Completes some source plans that exact-key reuse misses; optional, strictly replayed |
| Absorb unexpected weeds into route slack | `repair_panel_004` | More complete recoveries; some cash losses remain |
| Idle-helper preclearing | `repair_panel_004`, mode 6 | 280/384 complete weed recoveries versus 242 queue-only; 48 clean controls preserved |
| Advance sales to repair cash | Modes 3–7 in repair panels | Some recoveries, severe downstream losses; no default cash repair |
| Guard future inventory before emergency sales | Next-day/full-plan/profit guard variants | Reduces some harms but does not remove them |
| Delay dawn orders to allow earlier hires | Two expensive-day point controls | Still UNKNOWN at lower workforce after 30 seconds; bounded negative evidence |
| Canonicalize identical life specifications | `check_life_equivalence` | 432 legacy assignments agree exactly; removes duplicate evaluations |
| Cache unchanged day estimates | `check_life_day_cache` | Exact scores, 23–47% measured time reduction across the listed panels |
| Pack strict-replay schedule banks | `pack_bank`, format benchmark | 36 complete engine/calendar checks, identical outcomes, much lower loading/compile time |
| Search under a total wall budget | `optimize_placement.py` | Safe complete-incumbent retention; latest quality comparisons do not beat fixed layout broadly |
| Allocate remaining time to fixed-layout refinement | Bounded version 3 | Prevents wasted idle budget; still loses to fixed-only on some real cases |
| Explicit repeated biological services | Version-four life format | Imports both distinct-family cases exactly; post-freeze coverage extension |
| Explicit expected future resource failure | `ResourceShortfall` | Reports day/item/deficit and retains incomplete status; does not repair stock |

## Considered but not solved here

- General intraday time windows for biological decay, harvest and output delivery. One source remains unsupported because its overripe crop loses yield within the day.
- A general mid-game remaining-season compiler CLI. The daily C++ boundary exists; broader history/suffix preservation does not.
- A learned general placement selector. The complete-course headroom and independent-family evidence are insufficient to justify one.
- Beam/tabu/large-neighborhood variants beyond the implemented population, swaps, crop chains and dated changes. More candidate volume is not the measured bottleneck in the failed bounded continuations.
- A proven additive quadrant decomposition. Shared workers, shed trips, resources and hiring thresholds prevent the simple decomposition from being exact.
- A universally safe cash-repair rule or adaptive financial-policy integration that replans all future trades after changed deliveries. Fixed commitments still fail in real continuations.
- A fixed-placement service-calendar optimizer. A final static inspection finds one and seven extra same-day fertilizer applications in the two repeated-service courses. Reapplying does not stack the biological bonus, but no removal was compiled; changed inventory and downstream effects still require a complete counterfactual. See `research/repeated_fertilize_followup.json`.
- SIMD, PGO and a replacement day solver. Profiling supported exact reuse and serialization work first; no evidence required these heavier changes.

The next algorithmic comparison should target the complete affected-day continuation, preserve a strong existing witness, and count all failed or timed-out plans. More attractive day estimates are insufficient on their own.
