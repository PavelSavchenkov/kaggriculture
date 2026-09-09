# Generate day tasks from a cold composition

Construct the mixed farm's first day directly from dated crop/animal lives and
the owned-land-first layout. No source worker route, owner hint or recorded
tile-work sequence is supplied. Generate planting/watering and animal
construction/placement/service tasks, fixed seed/animal/wheat purchases and eight
hires. Ask the persistent day solver to realize the exact next-morning farm.

Three service alternatives are explicit: feed/care all new animals; feed/care
only the sheep; or establish all animals without first-day feed/care. The latter
two change economic output and may lose production. They are hypotheses to
compare in complete games, not assumed equivalent service optimizations.

Completed evidence: all three day problems solved in51–67ms;768 complete
discovery games,32 exact old control records,96 exact instrumented games with
all24 scheduled hours used, and52 operational games passed. The latter include
generic/pair/debug/thread equality and all four modes in self-play and against
PASS. The initial static-pair result labels were corrected with an identical
four-game replay; the original is preserved. Read ANALYSIS.json, COVERAGE.json
and OPERATIONAL_CHECKS.json. No strong-agent promotion: mean gains are only
about$1–$100 in this cold continuation, with later losses still unresolved.

The solver does not handle money or capacity. Every returned schedule is also
checked in the root full engine against PASS for exact endpoints, successful
actions, inventory and discards. A subsequent full-game policy must pass
opponent-price and operational checks before any strength claim. This first-day
prototype does not yet solve arbitrary full-season construction.

Lineage: local cold_farm from compiler_labor_sep08_001; placement rule from
compiler_placement_sep08_001; persistent day_solver and the existing local
day_contract conversion/helpers. All dependencies stay in this experiment or
persistent root resources. Run through conda using run.py; each new attempt
needs a fresh output directory.
