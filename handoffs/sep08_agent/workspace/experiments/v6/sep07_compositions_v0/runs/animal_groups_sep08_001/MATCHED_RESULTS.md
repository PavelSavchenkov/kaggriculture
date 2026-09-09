# Complete matched animal continuations

All six selected cow/sheep continuations execute719 transitions against the live
public router, validating every action and every daily farm/inventory endpoint.
The source is the current strongest empty_sale_slots_m2. These remain offline
fixed-calendar counterfactuals in two exposed worlds, not deployable league wins.

| Candidate | Own cash gain | Margin gain | Extra hire cost | Cheap own estimate | Estimate less actual labor | Residual |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| sheep_3 | +981 | +747 | 377 | 1353.31 | 976.31 | +4.69 |
| sheep_12 | +1125 | +891 | 233 | 1353.31 | 1120.31 | +4.69 |
| sheep_13 | +892 | +658 | 466 | 1353.31 | 887.31 | +4.69 |
| sheep_triple | +3258 | +2711 | 877 | 3893.22 | 3016.22 | +241.78 |
| cow_9 | +2110 | +2331 | 233 | 2347.12 | 2114.12 | -4.12 |
| cow_triple | +4774 | +5396 | 1542 | 6330.16 | 4788.16 | -14.16 |

Across these six cases, own-cash mean absolute error falls from
581.74 to
45.69 after charging measured labor.
This supports the cheap-economics hypothesis in this small matched study. It
does not establish a general labor predictor or accuracy on unseen farms.
Margin error after labor remains301.82;
the live rival's cash response differs although all rival production stays equal.

The single cow adds15milk and13fertilizer, displacing10wheat and1carrot. Three
cows add45milk/39fertilizer, displacing31wheat/2carrots. Each sheep adds10wool
and8fertilizer, displacing8wheat. The sheep group triples these quantities.
Three sheep together cost877 extra labor versus
1076 summed across the three separate changes;
own gain3258 exceeds their summed2998.
Shared labor and market interactions both matter.

## What made the compiler finish

1. Keep full animal lifetimes, including already planned future animals, when
   calculating displaced output, feed, fertilizer, work and canceled purchases.
2. Place extra animal orders only at a funded hour; physical stock arrival alone
   does not prove the order can be paid.
3. Reduce lost output carried into later days. Cancel early sales against missing
   opening stock before assigning new output to a later sale. Cow day28 then
   solves in0.34..0.36seconds with no extra worker.
4. Preserve actual untouched empty/weed tiles. Copying the original trajectory's
   weeds silently required random weeds to appear/disappear without work.
   A constructed courier schedule exposed the two invalid passive targets.
5. Fix unchanged original jobs to their worker/hour, leaving changed jobs free.
   After correcting passive targets, all six final days solve in0.10..0.16seconds
   with2extra workers. Five also solve with1extra worker. Cow triple still uses2.
   Every selected physical schedule then passes the complete live-engine audit.

The original final-day control solves in0.072seconds with fixed jobs, while the
same solver without assignments remainsUNKNOWN after30seconds. The false passive
targets made restricted models immediately infeasible. Earlier unrestricted
timeouts were therefore not evidence that these compositions needed more labor.

## Reproduction and next work

prepare_lifecycles.py and estimate_v2.cpp define the corrected economic screen.
run_later_groups.py, run_terminal.py and run_suffix.py retain all prior attempts.
prepare_market_repair.py and prepare_passive_tiles.py preserve the two contract
corrections. run_fixed_hints.py records the failed/control comparison;
run_passive_hints.py records corrected0/1/2-worker physical solves.
run_final_audits.py executes those schedules against the live opponent;
matched_h1 and matched_h2 retain complete actions, guards, problems and daily
cash/labor profiles. Protocols contain exact commands and source/binary hashes.
All Python, builds and binaries run through the kaggriculture conda environment;
solver-linked binaries also use the persistent day_solver/with_runtime.sh.

Compiler_v7 includes passive-state repair; compiler_v8 additionally tries a
bounded fixed-job repair before unrestricted solving. Its full integration test
is separate from these already completed suffix audits.

Next build full observation-driven continuations with the parent's tomato,
animal-family and day20berry choices preserved; retain current market adaptation
and compare against parent and diverse opponents on matching fresh scenarios.
No source-season seed, hidden inventory or future shop path may reach the policy.
Broader cold farms and mixed cow/sheep/goose/wait choices remain in scope.
