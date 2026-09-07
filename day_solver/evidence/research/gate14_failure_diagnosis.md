# Gate14 failures

Gate14 tested the frozen V25 solver on1340unique unseen days from60unused
episodes of Crop Dusta submission56023097. It closed1338strict schedules and
2UNKNOWNs. There were no invalid returned schedules or extraction/conversion
failures. The failures are106126272_p0_d16 and106139776_p1_d16, with127and121farm
actions respectively and13workers each. Both consumed their900s budget.

The previously selected V20 also returnsUNKNOWN on both with the same900s
budget. These gaps predate the latest optimization. V25 remains unselected.
Neither this comparison nor any later diagnostic rerun changes the first-attempt
gate results. Both inputs are now development data.

No original route is used as a solver input. The original actions were read only
after the gate closed, for a separate inventory trace and inspection of worker
assignments. Each witness splits7tiles between workers, often harvesting first
and planting, feeding or caring later. The original routes remain separate from
all generated hints used in the experiments below.

What the controlled checks show:
- Extra constructor seeds alone do not solve either case:16quick-only attempts
  with8offsets all endUNKNOWN.
- OUR shared-resource constructor's soft coarse optima have no tile-order
  violations. Remaining penalties primarily concern melon returns, with smaller
  strawberry deficits. Early milk alone is not established as the cause.
- The existing availability neighborhood moves consecutive tasks of a tile as
  one block. It cannot detach a harvest from subsequent planting/watering in
  that block. This is a search restriction, not a game invariant.
- Insertion considers only the two positions with least estimated route length.
  An urgent harvest inserted first can be excluded by that rule.

New C++ split-prefix/suffix moves were tested alone and together with the original
whole-block moves. Neither experiment produced a strict schedule. The combined
search improved the second case's remaining coarse quantity penalty to6, but a
lower penalty is not a valid day schedule. All attempted candidates and negative
results are retained. Route-front insertion and delivery-lateness ranking also failed to produce a
strict schedule. These are internal repair tests, not fresh validation.

Evidence:
- work/crop_dusta_gate14_reserved/final_summary.json
- work/crop_dusta_gate14_diagnosis/initial_failure_analysis.json
- work/crop_dusta_gate14_diagnosis/selected_v20_v1/closure.json
- work/crop_dusta_gate14_diagnosis/proposal_diagnostics_v1/
- work/native_quick_portfolio_v28/seed_probe_v1/closure.json
- work/native_split_candidates/repairs_v1/closure.json
- work/native_split_candidates/combined_v1/closure.json


The frozen Gate9 Python reference also returned UNKNOWN on both cases at its
original 900-second budget with eight solver threads, running two days
concurrently. No original route or witness was supplied. Selected C++ V20,
frozen C++ V25, and the earlier Python pipeline all miss these two inputs.
This establishes an older coverage gap; it does not identify a unique missing
constraint or prove infeasibility. The original witnesses remain feasible.
See work/crop_dusta_gate14_diagnosis/frozen_python_v1/closure.json.


## Owner changes and related tile work

An exact model with optional owner preferences solved the second case from OUR
repaired proposal in 50.138 seconds. It changed seven task owners and preserved
11 of 13 complete route-owner assignments. The important exchange included a
harvest and subsequent planting/watering on related tiles. Preferences were
soft, task domains remained unpruned, and search stopped on the first schedule.
Three other 180-second preference trials found no schedule; stronger presolve
in four 60-second trials did not help.

Selecting a deficient-output route, then freeing owners of all work on tiles
that route touches, solved the second case in 7.720 seconds from the repaired
proposal and 29.506 seconds from the initial proposal. Strict replay and an
independent 24-hour inventory ledger passed both schedules. The initial proposal
was constructed from public v3 only. The component test nevertheless reused the
stored generated proposal, so it is not yet a cold end-to-end result.

For the first case, larger related-tile components with four to six workers
returned UNKNOWN at 60 seconds. Two smaller groups were restricted-INFEASIBLE.
Repeated tile closure can produce identical worker groups, which should be
deduplicated before exact solving. None of these results proves that the whole
day is infeasible.

Evidence: owner_preferences_v1, tile_closure_release_v1 and
 tile_closure_expansion_v1 under work/crop_dusta_gate14_diagnosis/.
The cold native implementation and its retained trials are under
work/native_tile_closure/.


Cold native_tile_closure testing reproduced the constructor proposals exactly,
but the parallel coarse solver chose different worker assignments. Both exposed
days then returned UNKNOWN. This demonstrates sensitivity to the selected coarse
assignment; the earlier stored-proposal rescue has not become a cold solver fix.
Two empty-work synthetic rejections were separately fixed and all nine synthetic
controls now pass. Adding optional coarse task-hour hints, with verified model
and activation controls, did not rescue the new cold proposals at 45 seconds.
Keeping workers interchangeable within their coarse spawn/release groups also
failed in these four component trials. No fresh cohort has been opened.
