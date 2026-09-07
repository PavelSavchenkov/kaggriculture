# Bohann opening v1

Strongest validated local reference at September 7, 2026, 17:20 UTC. This is a
complete observation-only C++ agent, not a Kaggle submission. Use `agent.json` to
discover its API and sources. [Reproduction commands](tests/README.md) work from a
checkout without the original experiment.

The policy retains Justin Lee's course, local terminal recovery, observed-shop
herd investment, guarded V30 worker days, and wheat/tomato/berry continuations.
It borrows only the first two market sequences from Bohann Wang's episode
106497007, seat 0. Day 12 selects the tomato continuation with at least two
observed tomato-consuming shops and a matching physical state; otherwise it uses
productive wheat. Both retain the day-20 strawberry-demand branch. Animal choices
use observed shops, public rival state, costs and whole-herd market effects.

New tests of this exact catalog package, native official RNG, both seats:

| Opponent | Wins / games | Mean cash margin |
| --- | ---: | ---: |
| Teammate shoprouter | 4,059 / 4,096 | $11,148.80 |
| Last submitted investment agent | 3,815 / 4,096 | $367.68 |
| King RC4 | 1,011 / 1,024 | $20,877.89 |
| Public router V5 | 881 / 1,024 | $3,947.69 |

There were no ties in these matchups. These are local results, not a leaderboard
rating. The earlier 19-opponent promotion panel beat the immediate crop-mix parent
969/1,024, but the opening lost two wins against V5 relative to that parent. Its
large King gain involves opponent liquidity and subsequent behavior; extra wheat
turnover is not extra crop production. Physical route guards do not guarantee
future finance, and general multi-investment construction remains unfinished.

[VALIDATION.json](VALIDATION.json) records the package checks and compressed full
game evidence. All 5,120 independent-shop and 1,024 native source-comparison games
match every saved game field, including both full action hashes. Generic, typed,
debug and serial 64-game records agree; PASS and independent self-play finish.
[RESEARCH_VALIDATION.json](RESEARCH_VALIDATION.json) preserves the broader promotion
results and tradeoffs. [PROVENANCE.json](PROVENANCE.json) records all component
lineage; [SOURCE_MANIFEST.json](SOURCE_MANIFEST.json) maps the frozen sources to
this package. Include paths and private namespaces changed; policy logic did not.

All policy dependencies are under `source/`. Only `agents/common/api/` and the
header-only `fast_game_engine/` remain repository dependencies. The day solver was
used offline; its code, runtime, and build artifacts are not bundled or required
to run this policy. Do not relabel the borrowed strategy as wholly in-house work.
