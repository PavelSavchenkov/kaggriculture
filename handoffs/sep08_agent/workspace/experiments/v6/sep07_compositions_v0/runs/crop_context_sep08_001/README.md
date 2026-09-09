# Crop choice in the strongest agent

Expose the day12 wheat/tomato decision while preserving the strongest agent's
opening, sheep branches, day schedules, late investment and sale improvements.
The unchanged control retains its original shop threshold and later eligibility.
Two forced alternatives measure the remaining-season value of each crop choice.

prepare.py copies the current experiment source closure into private namespaces,
adds per-instance parameter forwarding, and creates three C++ packages.
run_discovery.py compares them on common seeds against nine opponents and checks
complete mode0 records against the original current agent. No promotion implied.
Future observed-price/flow estimation must be evaluated against these outcomes.
All commands use conda run -n kaggriculture. Preparation is one-time only.

Complete:4,608 discovery games;1,152 exact original controls;1,152 exact
common prefixes and public day12 observations;52 required operational games.
Neither forced choice is retained. The per-game hindsight best has only$14.08
mean margin headroom across8active opponents, so no new price-selector fit is
prioritized for this pair. See RESULTS.md, OBSERVATIONS.json, SELECTION.json,
OPERATIONAL_CHECKS.json and SOURCE_AUDIT.json. Original225dependencies unchanged.
