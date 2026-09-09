# Late animal schedules

This run answers whether the day solver can improve a new animal composition.
It can: the initial short solve substantially overstated labor cost. A later
full-policy failure also exposed why physical entry guards must preserve the
parent's selected future composition. Both findings are retained below.

Sources and first biological/economic estimates are in
../late_animal_rotation_001. The first proposal replaces two future wheat
cycles and carrot on tile(1,3), beginning day13, with an animal through day29.
It does not compare goose with a cow already occupying that tile.

The first3-second goose schedules used9 extra worker-days/$987 in the off berry
continuation and8/$898 in the on continuation. These were schedules found,
not proofs of necessary labor. retry.cpp gives the fixed original workforce
more search time; run_retries.py and run_second_pass.py preserve budgets and
all successes and UNKNOWN results. Offday23 needed93.47seconds. Offday22 is
still UNKNOWN after120seconds; neither that nor failed route reuse proves
infeasibility.

COMBINED30_AUDIT.json proves the first6/$665 and7/$754 savings, with identical
production and sold quantities. TERMINAL_AUDIT.json extends savings to8/$898
in both leaves. On uses no extra hires; off retains one $89 hire on day22.
The final-day route leaves one unsold fertilizer uncollected, with unchanged
sales and rival actions. Every cash gain equals saved hire cost. Actual final
matches end at hour22; the solver's hour23 PASS padding is never scored.

The physical solver nets same-hour product buys/sales; actual executable
market orders preserve both. Certify the raw solver schedule first, restore
actual markets, then verify the full game. Replaying mixed executable orders
against the net physical contract was an invalid check, preserved in earlier
diagnostic artifacts. retry.cpp contains the corrected integration.

format_packages.py freezes complete C++ courses in proposals/. The normal
policy uses the common off prefix on days13..19, then the parent's observed
berry-demand rule on day20. Both branch entry guards match exactly. The forced
on compiler used different earlier routes; PACKAGE_PREFIX_PARITY.json proves
all64 full records against the correct shared-prefix audit. Earlier parity
failures are preserved, with the precise reason. CHECKS.json proves768 full
generic/pair/debug/thread checks under custom and native shop streams.

The initial3328-game screen was positive for optimized goose. The first
43008-game fresh broad screen failed: FRESH_ANALYSIS.json. Its major losses
came from overwriting the parent's tomato future in physically identical
entry states. That changed24tomatoes into wheat before adding goose. The
correction late_goose_wheat_context retains the parent's existing day12
tomato-shop threshold. It is tested on new seeds in ../late_goose_context_001;
the old1790000 cohort is now diagnostic. No agent in this run is promoted
merely because solver savings or a small screen are positive.

run_species_retries.py gives cow/sheep the same30-second fixed-workforce retry.
species30/AUDITS.json proves unchanged production/sales/rival actions for64
games per leaf. All extra hires disappear for sheep and the on cow course;
off cow still has unresolved days. These routes support general animal/wait
comparisons; they are not by themselves an economic recommendation.

Run all scripts/builds with conda run -n kaggriculture. CMakeLists.txt builds
retry, reuse, retime and forced-leaf audits against persistent root day_solver
and fast_game_engine. Launch those solver-linked tools through
day_solver/with_runtime.sh. Exact commands are retained in process records.
Generation scripts fail if outputs exist, preserving the original artifacts;
use a new run/output name for a reproduction or alternative budget.
