# Reusable day programs

Read RESULTS.md, ANALYSIS.json, COVERAGE.json and OPERATIONAL_CHECKS.json.
Eight C++ policies use twelve locally solved day14..16 programs on two cold farms.
Mode0 is the original compiler. Mode1 matches exact stocks and tile states;
mode2 uses current-stock forward validation with exact tiles; mode3 checks only
layout/species, simulates current stock feasibility and skips redundant actions.

512 discovery games include64 complete original controls. All512 instrumented
coverage games reproduce complete records exactly;104 operational games pass,
including generic/pair/debug/thread equality, self/PASS and zero-budget fallback.
Mode3 activates in8/16 games against the accepted reference for each farm, with
no interrupted programs anywhere in the512-game panel. It improves cash by418
and598 against that reference, but both farms still lose by roughly57k. p362
loses328cash to its original compiler because reduced animal output outweighs
335saved labor cost. This is not an accepted compiler or strongest-agent change.

The observation-built simulator passed2876 current-state and2760 daytime-step
checks; no rival private stocks or actual seed enter the policy. Forecasts assume
rival PASS and no random weeds. Physical feasibility does not establish future
value. The next study, ../day_program_value_sep08_001, compares feasible programs
with original-controller continuations at several horizons.

Run prepare.py, run_discovery.py, analyze.py, run_coverage.py and
check_operations.py through conda run -n kaggriculture. Sources and lineage are
in LINEAGE.json, SOURCE_HASHES.json and each package. The coverage diagnostic's
first compile failed because one namespace was missing; its preserved log is
coverage_compile_namespace_failure.log. No policy source changed for that fix.
