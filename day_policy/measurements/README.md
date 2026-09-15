# Measurements

`REPORT.md` and `SUMMARY.json` are the final September 15 measurements for the
unchanged policy source delivered here. The JSON also contains per-player
timings for our carried placement and original-hire-filtered per-player results.
`../PROVENANCE.json` records the measured executable hash and source hashes.

`CAPS.md`, `CAPS.csv` and `CAPS.json` add the requested caps 10/11/13 comparison.
Each row keeps only days whose original successful hires are <= that cap, then
tests the solver at that cap. The same filter applies to timings. The CSV splits
every cohort and the combined comparison set by all 30 original agents, including
late-day timings, p95/max times, mean hires and hire savings on success. Both
original dawns and our carried placements are reported. Hour-23 results remain
a separate diagnostic. `CAPS_VALIDATION.json` records repeat parity and source
identity for the additional cap-10 runs. All reported timings are uninstrumented.

`rows.zip` holds 101 CSV files: the selected runs and repeats, Legacy comparisons,
placement audits, return bounds, placement microbenchmarks and replay-pattern
evidence. Repeated solver runs retain separate timing samples and identical
non-time fields. Old worker-only/PGO runs, logs, binaries and duplicate summaries
are omitted. The `placement_*_release_*` names are measurement identifiers;
they do not refer to external build folders.

In result filenames, `s0` is Legacy, `s1` is Staged, `r0` retains strict original
return deadlines, `r1` moves all deadlines to hour 23, and `hN` caps hires at N.
`original` starts each test at its original dawn; `own` uses a grid carried from
game start with our placement. The farmer is excluded from all hire counts.
Late means days 20–28. Mapping failures remain in the original denominator but
have no solver timing sample.

Recorded solver times combine two sequential CPU14-pinned runs, including failed
calls, hire minimization and verification inside the solver. The evaluator's
second verification is excluded. Native GCC 13.3, O3/native/LTO, no PGO or
fast-math; other machine activity was not controlled. Legacy comparisons use
one run. Do not compare these times directly with a different machine/build.

`DEVELOPMENT_VALIDATION.json` records the checks performed before packaging:
28 deterministic configuration pairs, 2,580 sanitizer replay calls, release/debug
regressions and generic/pair full-game smoke parity. `../PACKAGE_CHECK.json`
records the separate checks run from the root delivery folder.

Use `tools/test.py unpack-reference` and `tools/report.py` from the README to
recompute the report. New benchmark runs produce their own per-call CSVs,
`SUMMARY.json` and `RUN.json` outside the package. The saved report is not
silently replaced by the package-validation timings.
