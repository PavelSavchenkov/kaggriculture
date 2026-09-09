# Yusuke Shop Router 0908: verified public C++ port

Read FRESH_RESULTS.md first. The faithful port wins all 1,024 fresh direct games
against our current reference, but scores 87.39% against six other opponents
where our reference scores 94.19%. Keep it as a strong counter in the league;
it has not replaced the current reference.

Mode 0 is the base tape; mode 1 adds the day-6 Yarn decision; mode 2 is the full
upstream day-6/day-27 router; mode 3 is a local amendment that preserves an early
alternate farm. The amendment underperforms the faithful source. Each variant
is a complete C++ package under proposals/. LINEAGE.json records the exact
notebook, archive, model and tapes. Original replay IDs and artifact rating are
unknown; retain attribution.

Validation covers all 2,876 literal tape actions, 24 routing cases, 24 resets,
48 operational games, 3,968 discovery games and 28,672 fresh comparison games.
Generic/pair/debug/thread outputs agree; all source dependencies stay frozen.
gap_diagnostic/REPORT.md compares production and costs in the same direct games.

prepare.py exports data; verify_source.py checks source parity;
check_operations.py validates execution; run_screen.py and run_fresh.py run
matches; analyze.py and analyze_fresh.py report results. Run all commands through
the kaggriculture environment. No submission or official catalog copy was made.
