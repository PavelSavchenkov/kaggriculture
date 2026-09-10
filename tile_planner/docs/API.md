# Current C++ interfaces and file contract

## Lifetime plan

`PlacementProgram` in `include/cold_cases.hpp` holds `lives` and thirty land-purchase counts. `LifeSpec` in `include/lifetimes.hpp` names an item, birth day, stop day, six service-day masks and allowed cells. Cell `10*y+x` has coordinates `(x,y)`; days are zero-based.

The normal stop removes a surviving crop before that day's service. `clear_after_service` permits service followed by clearing on the stop day. Animals cannot be dug up; natural escape under the supplied feed calendar is represented by the biological oracle. `release_day` is derived, and same-day old-life release/new-life birth is legal.

The text formats are backward compatible:

```text
PLACEMENT_LIVES1 count
30 land-purchase counts
item begin stop water feed care collect harvest fertilize allowed_100_bits
... one record per life
```

Version 2 appends a `0|1` clear-after-service field to every life record. Version 3 appends that field followed by thirty service-order integers. Each nonzero order packs four-bit action codes from the low nibble upward: water 1, feed 2, care 3, fertilizer collection 4, harvest 5, fertilize 6. Zero uses the normal order. An explicit order must include the required valid service operations exactly once. This supports, for example, watering before fertilizing without changing the implied yield.

Version 4 appends a `0|1` repeated-service flag after the thirty order integers. A flagged life may repeat a valid operation in its explicit sequence, preserving every input consumption and effect. Each daily packed sequence holds at most eight operations. Required operation types still cannot be omitted. Earlier versions retain the no-repeat check. This coverage extension was added after the frozen final test rejected two repeated-fertilization inputs; its results are reported separately.

Writers use the oldest format that expresses the supplied plan. `load_placement_program` validates and compiles the biological program. `save_placement_program`, `read_life_assignment` and `save_life_assignment` provide matching file operations. An assignment file contains one cell integer per life, in input order.

Within one text-plan load, identical complete `LifeSpec` values share one oracle compilation. Returned lifetime records remain independent values in the original input order. The cache includes allowed cells, ordered/repeated service flags and every other specification field, and ends when loading returns. It does not persist across changing programs or replace legality checks.

`compile_life(spec)` uses a single-tile official-engine oracle with injected supplies. Its result includes each day's starting state, ordered work, post-work state and next dawn state. It proves biology only. Late harvests whose yield decays within the day still need an explicit intraday timing contract and are rejected by the replay importer when quantities differ.

## Placement and daily compilation

- `legal_lives(lives, assignment)` checks allowed cells and ordered interval occupancy.
- `try_assign_lives(lives, animals_first, nearest)` returns a greedy assignment or no result. `assign_lives` requires that construction to succeed. Neither proves that other assignments are impossible.
- `compile_life_day(lives, assignment, day, actual_farm, land_purchases, validate_assignment)` derives the day's biological work, clearing, compatible structure reuse and a simple financial baseline. Normal callers leave validation enabled. The whole-course driver validates once and then keeps actual biological-state checks enabled on every day.
- `apply_fixed_finance(contract, source_day, day)` replaces the simple financial baseline with the supplied dated commitments. It recalculates physical resource endpoints from the actual start and required work. Same-hour round trips stay in executable orders. The physical adapter preserves the maximum sell-minus-buy prefix of the actual order sequence, restoring canceled purchases as needed; net withdrawal alone is insufficient for a sale preceding a buyback.
  A negative required endpoint throws `ResourceShortfall` with day, item and missing quantity. The whole-course driver catches this expected failure and writes an incomplete summary; unexpected input/program errors still fail fast.
- `LifeEvaluator(lives, land, collapse_identical, reuse_days, revalidate, optional_fixed_finance_days)` predicts the complete program. The default uses a fresh day-zero farm and an unbounded weed-free projection; it is not a schedule or cash certificate. The optional finance array and program must remain immutable and outlive the evaluator.
- `search_lives` explores legal relocations/swaps and preserves its best predicted assignment. The caller must still perform exact full-course checks. `probe_lives` also constructs crop-chain, animal and jointly evaluated quadrant-count interventions.

The daily compiler accepts the actual dawn farm. The supplied whole-course CLI starts a new game at day zero. General mid-game replanning is not exposed as a complete CLI: the existing dated warm interface and the conditional two-assignment driver cover narrower cases. Established entities must keep their cells.

## Certificates and reuse

`solve_contract(day, day_number, day_seconds, query_seconds, optional_incumbent)` returns a strict schedule/problem pair when found. Missing schedules mean UNKNOWN. A timeout does not prove infeasibility. Physical certificates do not establish cash or capacity feasibility in the full game.

`ScheduleBank` supports `load`, `add`, `save_packed`, and `find(day, day_number, maximum_workers, repair_preparation)`. Keys filter candidates using physical cells and ordered work. Resource amounts and biological ages are checked by strict replay before a schedule is returned. Hiring cannot overwrite fixed nonlabor orders. The optional preparation pass examines at most 64 candidate schedules, removes obsolete digging/building and inserts missing preparation into spare route time. Terminal field work must finish by hour 22.

Packed banks use `TPLBANK1`. `pack_bank` checks text/packed lookup equivalence on a supplied validation manifest. Prior bank construction and loading are separate costs in experiments.

The estimate cache is scoped to one immutable program/day and includes every farm field read by the compiler plus active life locations. Identical life specifications are canonicalized. These caches never substitute for a physical or whole-game certificate. Use one mutable evaluator/bank per worker or externally synchronize access.

## Full-game output and acceptance

The C++ driver runs both players through the real engine, requires every own requested action/order to succeed, and checks cumulative biological output each day. It records the actual continuation for the independent financial replay. Delivered and discarded quantities belong in the output passed to the sales planner; equal biological production does not guarantee equal retained stock.

The calendar exporter uses a separate reader for recorded rival actions. Invalid rival item commands remain unchanged in the full engine and are normalized to their known no-op effect only in the financial ledger. Own certificate files retain strict validation. The exporter records how many such rival commands it handled.

`compile_cold --opponent recorded --opponent_trace trace.txt` evaluates against the original recorded opponent actions. The supplied seed and weed rate must match that trace. This is a conditional evaluation context, not information available to an online placement policy. The normal `pass` and `public_router` modes run live policies. A supplied `--finance_source` remains a fixed commitment calendar, so changing the opponent can make that calendar unfunded even when its biology is unchanged.

`optimize_placement.py` retains the complete input, freezes its C++ tools, shares newly found witnesses with the unchanged-layout control, and accepts only complete financially verified candidates. It uses only certificates supplied with the input or found within that run. Its score is conditional on the supplied evaluation scenario. Runtime demand rules use only the current observation and cannot change earlier lives.

The optimizer writes `RESULT.json` by atomic replacement. A budget too small to recheck the input returns the supplied incumbent with `input_reverified: false`; it does not invent a new certificate. An actual failed input recheck returns a nonzero process status and `status: invalid_incumbent`, with no accepted path. Setup/copy time is included in reported internal elapsed time and can exceed a near-zero budget before any solver call.
