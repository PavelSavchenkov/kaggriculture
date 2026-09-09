# Day-program continuation value

Read LINEAGE.json and the eight C++ packages under proposals/. The same twelve
locally solved programs are compared with the original compiler from the current
observation. Mode0 is exactly the preceding relaxed controller. Modes1/2/3 select
programs using one-day, three-day and remaining-season continuation forecasts.

Forecasts assume rival PASS, no random weeds and no new shops. Short horizons
mark remaining products/seeds at common observed prices; full horizon uses final
cash. These are explicit approximations. Discovery uses the same 512-game panel;
analyze.py checks128 complete relaxed-controller controls and compares against
the original compiler as well. Coverage records activation, forecast steps,
predicted gain and act latency. Operations cover generic/pair/debug, self/PASS,
thread determinism and zero-budget fallback. No policy is promoted here.

Run scripts through conda run -n kaggriculture: prepare.py, run_discovery.py,
analyze.py, run_coverage.py and check_operations.py. Preparation refuses to
replace packages; runners refuse to replace game directories.
