# Integrated estimator, first calibration

Typed input: dated individual Life records, optional Support arrays, and explicit
EstimateOptions. New estimate.hpp combines biology, source/greedy layout,
approximate movement, pickups/deposits, workforce, land, seed/animal purchases
and financial schedules. economics.hpp values both players' fixed flows using
endogenous marginal prices and shop consumption. An opponent's flow schedule
is a scenario approximation, not its private state or its adaptive policy.

Service modes: productive, fully fertilized productive, and recorded day masks.
Recorded support borrows daily hiring and land from the replay. Estimated
support uses field work plus a movement and transport formula. Layout can be
recorded or newly assigned greedily. These are meaningful fidelity choices;
the source mode contains more information than composition counts alone.

No estimate is a certified upper bound or a feasible policy value. Output is
conditional on service and supply. Negative cash remains an explicit funding
witness; estimated route overload is uncertainty, not proof of infeasibility.
Birth/harvest/deposit timing uses simple distance formulas, input reserves are
one day, and purchases can occur before future scheduled own production would
make them unnecessary. The order estimate groups purchase types but does not
schedule around the ten-order cap. Most source programs have funding witnesses
despite being feasible in their recorded executions: improve timing and cash
coordination before using these witnesses to reject compositions.

Benchmark: 864 resource profiles (72 programs × 3 service modes × 2 support
modes × 2 layouts) across eight common shop/observed-opponent scenarios. First
one-sided evaluator: 39.6 microseconds per profile/scenario, including amortized
resource construction. Two-sided valuation: 45.4 microseconds, 6,912 evaluations
in 0.314 seconds. Five full 719-turn financial fixtures match both cash values
and our stock/market outcomes. These tests isolate the financial contract.

Exact calibration: all 72 programs compiled with productive or recorded service,
and all 72 exact recorded courses. Eight common discovery games each; no wins
for either greedy compiler. Recorded courses are verified against all 51,768
original actions. Same-source course 0 matches the earlier isolated teacher
over eight complete games. Source-model biological output for program 0 is
wheat 560, carrot 112, tomato 86, strawberry 313, melon 72, egg 86, milk 143,
wool 98 and fertilizer 319, closely matching the original recorded production.

Offline calibration in results/estimator_calibration.json deduplicates each
compiler's programs by actions over the full common scenario set. With source
support, productive cash estimates rank productive compiler cash at Spearman
0.763. Replacing source labor with our rough estimate reduces it to 0.122.
The labor module has not earned ownership of that decision.

For recorded complete courses, recorded services/support give cash Spearman
0.858, mean absolute cash error $4,389, and pairwise cash ordering 85.4%.
Valuing the opponent too raises margin correlation from 0.538 (ranking our cash)
to 0.881 (ranking predicted margin). Same model versus the source-service
greedy compiler has margin correlation 0.553 and overestimates cash by $17,668
on average, exposing substantial realization losses. Only four shop seeds are
represented in this first calibration; broader and held-out calibration remain
required. Similar traces are not independent strategic families.

Original one-sided CSV/calibration are retained with _v0 suffix. Drivers:
src/estimate_library.cpp and src/evaluate_library.cpp. Python scripts only format
source records and analyze outputs. Core estimation, compilation, game execution
and program iteration are C++. An automatic composition-mutation/branch search
and repeated league improvement loop are still required.
