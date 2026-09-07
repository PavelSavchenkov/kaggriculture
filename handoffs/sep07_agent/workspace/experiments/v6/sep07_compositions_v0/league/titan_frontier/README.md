# TITAN frontier

Faithful C++ port of the frozen Apache-2.0 TITAN source: Kaito Fukami's three v43 courses and shop branching, independent bounded weed repairs, and SELL-slot ranking; Igor Zharov's observed-stock helpers; LARK's finished-product sale advance when public farm-signature distance is at most two.

The first Yarn shop selects the first wool course from turn 88. A second Yarn shop, when the first is another type, selects the other wool course from turn 153. All three child controllers observe every turn. The optional bakery market-maker is disabled in the source configuration.

The LARK overlay preserves worker actions and inherited market slots, changes finished-product SELL quantities to projected available stock after pickup reserves, and fills spare market slots. It uses older CARROT/TOMATO/EGG price curves than the current Kaito parent. This literal difference is preserved. Zero-quantity duplicate SELL slots are retained as in the source.

`AgentCore(false)` supplies the Kaito-only causal baseline in `../kaito_v43`. Exact module, route, author, source, and license lineage is in `IMPORT.json`; the original `LICENSE` and `NOTICE.txt` are retained. No actual leaderboard rating is claimed. Both TITAN and its Kaito-only baseline independently match 11,504 source actions. Generic/debug/thread, PASS and self-play checks pass.

On 128 common-seed games, TITAN wins 123 against its parent, with mean margin +$4,793. It wins only 12 against the submitted shop-herd agent, 18 against teammate and 5 against King. The overlay leaves all game records equal to the parent against four of five modern opponents, because the similarity gate is usually inactive. On 32 parent-match profiles, early sales change funding and actual production: own cash +$410, margin +$4,251, strawberry output +31.25 and melon output +12.25. This is not an equal-production timing result. Retain for research; no incumbent promotion. Full evidence: `results/titan_frontier_validation.json`.
