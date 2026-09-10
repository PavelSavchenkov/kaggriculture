# Using the placement component in composition search

The reusable unit is a dated farm plan and its legal lifetime assignment. The current implementation supplies C++ compilation, proposal generation, estimation caches, strict schedule reuse and full-game checks. It does not establish that the new placement search generally beats stronger fixed-layout scheduling. Keep the exact reuse improvements and the placement-quality claim separate.

## Boundaries between the existing components

1. The composition search supplies species, counts, birth/stop dates, service requirements, owned land and dated land purchases. Different choices make different `PlacementProgram` inputs.
2. Placement supplies legal cells for whole lifetimes. Existing occupants remain fixed; a later crop can use a different cell after the earlier life releases its cell. Preparation work follows the resulting assignment.
3. The fast day estimator orders candidate assignments and workforce queries. Its score is a prediction, not permission to execute a schedule.
4. The day solver supplies routes for an exact daily contract. A returned strict schedule is a witness; no returned schedule means UNKNOWN.
5. The financial planner supplies dated purchases/withdrawals or evaluates the delivered-resource calendar. Preserve real market slots and successful quantities. An ordered sell/buy sequence may need more stock than its net withdrawal.
6. The full engine checks the complete continuation, including capacity, money, realized production, actual orders and the opponent. Keep a complete incumbent when any candidate fails or times out.

`compile_cold` demonstrates this complete path from a new day-zero game. `export_calendar` independently checks all 719 real transitions and exports resource movements, sales, delivered output and discarded output. Equal biological output does not imply equal stock retained for sale.

## Practical entry points

Build with the commands in `README.md`. Text plan formats and header signatures are in `API.md`.

- For a new composition, construct `PlacementProgram`, compile its `LifeSpec` records, and supply an assignment. Keep source/nearest/animals-first assignments in the candidate set. A failed greedy assignment is not an infeasibility proof.
- For a replay-derived fixed plan, run `import_lives SOURCE_COURSE NEW_OUTPUT`. Only use `INPUT.plan` when `biological_contract_verified` is true. Diagnostic or rejected outputs are not valid inputs. Trades and preparation timing are separate from the imported biological obligations.
- For estimates, create one `LifeEvaluator` for one immutable program and optional fixed-finance array. Reuse it across assignment proposals. Changing the biological or financial program requires a new evaluator. The whole-program estimator starts from day zero with a weed-free unbounded projection; it is not an arbitrary-current-state evaluator.
- For an actual dawn state, `compile_life_day` derives the exact daily work and checks biological continuity. `apply_fixed_finance` can replace its simple baseline finance. `ScheduleBank::find` replays every candidate against that new problem; `solve_contract` can refine a returned witness.
- For bounded offline improvement, give `tools/optimize_placement.py` a complete `compile_cold` course. It retains the input, rechecks it, shares useful new witnesses with the unchanged layout and writes the best complete result atomically. Compare `--mode search` with `--mode fixed` at the same total budget. After the finite placement queue is exhausted, search uses the remaining budget to refine its best complete assignment.

The supplied whole-course CLI has no general mid-game resume command. A caller can use the daily C++ interface at an actual dawn; the caller must preserve history and certify the entire remaining suffix. The dated warm and conditional two-assignment tools cover narrower implemented cases.

## Reuse and compute budgets

Use one mutable estimator/bank per worker. The measured panels use four single-threaded workers and two build jobs. Whole-assignment and day caches are bounded. Pack a reusable schedule bank once, and charge that construction separately from later lookup/compile time. The packed representation is a serialization optimization; strict replay remains mandatory.

The text loader also compiles each identical lifetime specification once per load. This is exact oracle reuse; it preserves input order and independent lifetime values. The direct check covers 15,715 lifetimes from 75 plans, with every daily state and work action unchanged. In a paired eight-input real-course benchmark, combining this change with packed banks reduces compile-plus-independent-verification time by about 19%, with every action and outcome identical.

Brief refinement of a reused schedule is a useful speed option, not a universal quality-preserving default. It matched full refinement on several new synthetic/service inputs but worsened real replay-derived bills and sometimes lost completion. Preserve a stronger complete incumbent and give difficult days more time when the caller's budget permits.

The process wrapper includes setup/copy time in its elapsed measurement. Near-zero budgets can expire during setup before any solver call. In that case it returns the supplied incumbent as not reverified in this run. A demonstrably invalid input is rejected and has no accepted result path.

## Rules supported by the evidence

Keep animals near shed access as a simple starting proposal, and explicitly preserve crop rotations and later lifetimes. Local route neighbors and simultaneous service can matter even when two cells have equal radius. A fixed universal cow-before-sheep order is not supported.

Use quadrants as proposal/cache blocks while evaluating shared workforce and resources globally. Around a quarter to a third of productive work in the inspected sources belongs to workers serving multiple quadrants. A quadrant-count intervention can save hiring yet reduce final cash through delivery/overflow.

Demand-aware rules must first have a demonstrated same-composition intervention. Two small conditional swap examples use only shops revealed when the relevant animals are still unplaced. Their held-out evidence is confined to one composition and a small scenario set, and their intervals include zero. They are examples, not promoted runtime rules.

Preparation/weed repair can remove obsolete builds or insert clearing into spare route time, with strict replay. Emergency sales restored some cash faults but still caused large downstream losses. No general cash-shortage repair policy is promoted. Return failed obligations and keep the planned financial commitments explicit.

## Remaining limitations

- Version four preserves repeated biological services, with at most eight ordered operations per life/day. Both repeated-fertilization final inputs now import exactly in a separately labeled extension. Within-day overripe decay still needs a richer timing contract; the rejected development case remains in coverage counts.
- Fixed replay trades are conditional commitments. A different opponent or delivery schedule can make them unfunded. Recorded future actions belong only to offline evaluation, not a deployable information set.
- The search does not prove optimality. Several predicted gains never produce a complete certificate. The best known placement depends on query allocation and available route witnesses.
- The final name audit identifies Blurry and Unknown Mother-Goose as the same team ID, 16730612. Its two command traces differ substantially from the public-tape family. Terry, cooked and pensukesan are heavily related to public tapes; they are not three independent strategy families.

Expected resource deficits now return an incomplete `SUMMARY.json`, for example `insufficient_stock_day_29_item_1_missing_2`. That recorded course replays its first 29 days with no solver queries and then reports the two-carrot shortfall. An incomplete prefix must not replace a complete incumbent.

Use `LEARNINGS.md` for the full positive/negative ledger and `reviews/` for the recorded changes in priorities. The final frozen protocol and binaries are in `runs/final_panel_001/`.
