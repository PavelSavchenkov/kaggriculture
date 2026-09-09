# Four-way late investment portfolio

Promoted agent: proposals/late_value_s32_t0_r05. Validation entry point:
../../results/late_portfolio_validation.json. The prior q32 reference, committed
Bohann catalog agent and last submitted investment agent remain separately
recorded in ../../CURRENT_REFERENCE.json.

At day13, compare retaining the existing wheat/carrot cycles on tile(1,3)
with goose, cow or sheep through the remaining season. Preserve the parent's
tomato intention: if day12 had at least two tomato-consuming shops, this slot
does not replace that future. Each animal course also needs its exact physical
entry guard. At day20, choose the berry continuation from then-observed shops.

The C++ estimator compares complete intended daily buys/sales and includes
animal, seed and compiled hire costs. It models the whole remaining own farm
and the opponent's public herd effect on shared prices. Thirty-two conditional
shop samples estimate unknown future demand; each sample uses its corresponding
future berry continuation. Select expected margin minus half a sampled standard
deviation, or retain crops if no estimate exceeds zero. No environment seed,
future observations or opponent-private stock reaches the policy.

All calendars are immutable and shared. Each agent owns one parent controller
and its own selection state. The selected schedules run behind checked daily
guards; no day solver runs inside agent inference. DATA_LINEAGE.json records
244 raw guard/action inputs. Before day20, a cheaper route from either leaf can
be reused only when the full physical day problems match byte-for-byte. This
reuses cow's on-day18 route and saves$144 in its off continuation as well.

The original3-second compiler overstated labor cost. The checked longer-search
routes are in ../late_animal_schedule_001: goose saves8/$898 worker-days per
activated game; sheep and on-cow need no additional hires. Off-goose still has
one extra$89 hire, and off-cow retains unresolved days. Those are achievable
costs, not minimum proofs. Current compiled HIRE orders all have quantity1;
the root engine executes one hire per market-order slot regardless of quantity.

On8192 diagnostic counterfactual games, goose predictions correlate.90–.92
with realized margin and have$70–$88 mean absolute error. Cow correlation is
.85–.86; sheep is less reliable at.47–.56. All32-scenario choices take median
.25–.34ms. Intraday liquidity, detailed rival crop sales and uncertain future
demand remain model limitations; exact execution is still necessary.

The C++ probe plays all four alternatives on identical observed prefixes.
analyze_counterfactuals.py compares prediction errors and42 simple selection
settings from those exact outcomes. The actual chosen C++ policy reproduces
all2048 selected game records. This permits cheap rule comparisons without
replaying every candidate rule, while retaining an executable-policy parity
requirement. The diagnostic1790000 seeds are never called fresh validation.

The first45056 fresh games left the current-group confidence bound unresolved.
The frozen policy then passed an independent180224-game confirmation: direct
parent1527wins,2256ties,313losses,mean+$173.98. Current grouped utility rises
.9451294→.9467957,95%gain[+.0007446,+.0025849]. Every paired mean improves;
some legacy crop agents lose a small number of wins, explicitly reported.
Native/PASS,896 operational checks and256 rebuilt frozen native records pass.

Reproduction starts with prepare_data.py and prepare_packages.py, then the
experiment's scripts/build_arena.py. build_probe.py/run_counterfactuals.py
create the counterfactual evidence; run_initial.py and check_selector_parity.py
check the executable policies. ../late_portfolio_validation_001 and
../late_portfolio_confirm_001 hold frozen specifications, commands, full games
and analysis. The final half-standard-deviation wrapper is a recorded parameter
choice in its IMPORT.json. Run all Python/build commands through
conda run -n kaggriculture. Generation scripts preserve existing outputs by
failing rather than overwriting them; use a new output/run for another trial.

This is one additional slot and date. Larger joint herd changes, new placement
and cold starts remain part of the main goal. The new Thomas V5/2 sheep-heavy
calendar is a concrete next source, recorded in research/refresh_2002.
