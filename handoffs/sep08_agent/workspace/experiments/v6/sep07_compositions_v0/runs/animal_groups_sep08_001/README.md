# Animal groups in the strongest farm

Current results: [INTEGRATED_RESULTS.md](INTEGRATED_RESULTS.md) records six complete
cow/sheep counterfactuals on the strongest parent. All daily endpoints and719
real transitions pass. After measuring labor, own-value error falls from494.90
to45.69 across these six exposed cases. They are not deployable league agents.
The corrected281256-proposal screen is in CORRECTED_RESULTS.md; the initial
screen below omitted some already planned animals and is retained as history.

This offline study starts from `empty_sale_slots_m2`, preserving its actual
selected crop and animal calendar when proposing changes. It enumerates one,
two or three animal additions on released one-shot crop tiles, including mixed
species. Entry dates range from day8 through day22 when enough time remains for
production. The compiler also accepts groups with different entry dates.

The C++ estimator subtracts retired crop lifetimes, adds animal biology and feed,
and values the changed whole-farm market flow under32 shop continuations sampled
from shops already observed. It records operations and original-route visit
deficits separately. Added labor is not priced until schedules are solved. A
visit deficit is not a proof that another worker is necessary.

Source calendars are extracted retrospectively from complete games. Future
source branch choices can therefore be present in the baseline calendar. These
scores cannot be used directly as a deployable observation-only selector.
The first control independently replays the unmodified current agent, requiring
both full action hashes, money, production, sales and discards to match, plus
exact dated crop biology. All real games end after719 transitions.

`prepare.py` adapts the older single-animal extractor and compiler. `LINEAGE.json`
records original hashes and changes. The new compiler preserves original work
before each animal entry and retains the root day solver's physical certificate
and full-game endpoint check with a live opponent. It restores gross market
orders after the physical solver's netted inventory calculation.

The compiler produces matched-scenario fixtures, not a new agent. A reusable
policy must preserve selected future farm branches and the inherited market
adaptivity before league validation. No old physically guarded cross-scenario
wrapper is reused to claim playing strength.

Reproduce in a new output directory, using the kaggriculture conda environment:

1. On a clean copy, run `prepare.py` once.
2. Configure and build this directory with CMake, Release mode.
3. Run `run_estimation.py --seed-start 1000 --seeds 2 --output estimates_1000`.
4. Give the compiler an output path, source seed, a text file containing
   `cell entry_day animal_item` rows, and a per-day solver time budget in seconds.
   Launch solver-linked binaries through the persistent `day_solver/with_runtime.sh`.

`run_estimation.py` saves the exact command, binary hash and actual C++ dependency
hashes. Failed attempts and UNKNOWN solves remain recorded. None is relabeled
as an economic loss or a proof of physical infeasibility.


The completed screen contains281256 proposals over18scenarios, with36full
source/control games and14.5815seconds of estimator work. Read
ESTIMATION_RESULTS.md and ESTIMATION_ANALYSIS.json. This is not a win-rate result.

Six initial fixtures and subsequent30-second pair attempts did not complete a
season. FUNDING_FINDING.md records the exact406-cash/two-cow purchase failure
and its funded-order correction. The new purchase timing passes day8 at the
original workforce. Both cow and goose pairs still time out on day10.

SOURCE_PHYSICAL_AUDIT.json now verifies42 unmodified source day contracts with
requirements, invariants and empty error lists. The first control incorrectly
included explicit sales in a physical schedule where cumulative stock demands
already apply those sales; its diagnostic errors remain preserved.

The current CMakeLists builds all prepared sources directly. For generator
lineage, prepare.py, prepare_diagnostics.py, prepare_funded.py and prepare_hints.py
show the sequential derivation; regeneration requires a clean isolated copy
without their generated files. Existing prepared sources need no regeneration.
Run source_control_v2 through with_runtime.sh into a new output directory.
run_case.py records each additional compiler attempt and freezes C++ dependencies.
run_hints.py compares the root light-exact solver with and without soft hints
from unchanged source tile tasks, with30seconds and4solver threads in both modes.
The hint experiment is offline scheduling, with no agent API or promotion change.

LIFECYCLE_CORRECTION.json invalidates initial economic scores where the parent
already plans an animal on a released crop tile. The original compiler duplicated
build/place work in those cases. Corrected season_v2/estimate_v2 include those
dated animal lifetimes, opportunity costs and purchase cancellation. All4242
crop and317animal lifetimes match the engine in18 source scenarios.

Compiler_v5 carries displaced crop inventory forward. Compiler_v6 cancels early
sales against missing opening stock. Compiler_v7 carries actual untouched weed
states; the former targets wrongly copied weeds from the original trajectory.
Compiler_v8 adds bounded fixed-job repair before unrestricted day solving.
The standalone physical/complete-game proof is in passive_fixed_30s,
passive_fixed_h1_30s, matched_h1 and matched_h2. integrated_30s passes all six
cases through the normal compiler with unchanged source hashes. integrated_audit
independently validates all their actions and daily endpoints in719-turn games.
The integrated compiler finds cheaper final routes for cow9, cowtriple and
sheep13. All earlier failures and the earlier MATCHED report remain available.

HINT_RESULTS.json records unsuccessful soft hints. fixed_hints_30s proves the
original final day solves in0.072s with fixed assignments versusUNKNOWN30s
without them; corrected changed final days solve in0.10..0.16s with2extra
workers. Fixed-job failures do not prove unrestricted infeasibility.
CARE_EQUIVALENCE.json verifies a separate two-care omission for new cows; that
omission has not been integrated into the accepted agent or these courses.
