# Public notebook audit, September 7 at 15:45 UTC

Neither update adds a new competitive policy to our existing league. Sources were pulled with the Kaggle CLI and inspected as JSON, AST and decoded literal data. No notebook cells or downloaded Python agents were executed. No C++ duplicate was created, no gameplay runs were added, and reserved seeds 1720000+ remain unused.

## Igor Zharov: Smart Farm Strategy Lab

The [15:08 update](https://www.kaggle.com/code/flexonafft/kaggriculture-smart-farm-strategy-lab) replaces its previous capacity router with the five-course controller already available as [public_router_v5](../../league/public_router_v5/README.md).

- New extracted agent SHA-256: `c89d3dd2e9cbdb97f95f2d51bbe2de687d64329069443da7c2ea788ce47fdb5c`.
- Previous agent SHA-256: `2b97e2c653018ac4aeffb8463ec91c8f26b097b4cef81289acc25ccdbc68f916`, matching the earlier `public_capacity_router` source.
- The new source differs from the pinned [Thomas notebook](https://www.kaggle.com/code/thomastschinkel/kaggriculture-93-8-win-rate-public-state-router) only in its leading title docstring. Removing that docstring makes the entire module AST identical, including globals, reset behavior, functions and stdin adapter.
- All 3,595 tape actions and five decision trees decode to byte-identical data, SHA-256 `0310a7c23ad176b614ec61dbc01d604459d9f1b4ea0f38e8094d77ca9789e5fc`.

The useful branch logic is already represented: choose the wool course at day 6 when a Yarn Store has been revealed; otherwise use milk demand to choose between two courses. At day 24, carrot price chooses a late course. At days 0, 12 and 18, the callable selects course 0. The many computed opponent and farm features never influence this tree. The source adds no physical continuation guard or funding repair.

The existing port has 10,073 original-source action parity checks, all reachable tree routes covered, PASS/self-play and 16 identical generic/debug/thread game records. This audit verifies that its current C++ source hashes still match that validation. These are earlier checks inherited through exact source equivalence, not fresh results or a new strength claim. See [IGOR_AUDIT.json](IGOR_AUDIT.json), [source diff](igor/v5_source.diff), [copied validation](reference/v5_validation.json) and [copied component lineage](reference/v5_IMPORT.json).

Both public sources refer to a missing `provenance.json`. Exact code equality establishes equivalent behavior; it does not establish original authorship or recover the donor replay IDs. Prior nearest-tape matches remain similarities, not definitive lineage. The downloaded metadata supplies no separate license. Preserve both public source attributions when reusing this controller.

## 3정훈: From Orders to Actual Trades

The [15:18 update](https://www.kaggle.com/code/az05192000gmailcom/kaggriculture-from-orders-to-actual-trades) changes 14 string constants used for labels, formatting and printed descriptions. Its complete AST is unchanged after blanking strings. Eighteen of nineteen function ASTs are exactly unchanged; the nineteenth only renames the `market_row` output label. Markdown now describes same-engine quote verification more cautiously. See [ORDERS_AUDIT.json](ORDERS_AUDIT.json), [code diff](orders/code.diff) and [text diff](orders/notes.diff).

This is an accounting notebook by the current leader, not disclosure of the leader's competitive agent. Its stated demo uses simple CARROT/WHEAT/TOMATO work and one goose against the starter. It has no new composition search, shop branch or crop calendar. The demo purchases fertilizer to illustrate spending but does not issue `FERTILIZE` actions.

Useful existing accounting practices remain:

1. Reconstruct actual filled quantity, not requested quantity. Quote both players at each corresponding market slot before committing either trade; preserve slots when the rest of an order fails.
2. Apply unit actions before market settlement, including shared tile mutations and same-turn seed oversubscription. A just-deposited product can be sold immediately.
3. Check signed, absolute and worst-turn cash residuals. Zero net cash residual does not prove individual product fills; reconcile inventories and biology too.
4. Treat bought wheat as a purchase, not automatically feed use. Resold wheat and fertilizer can inflate gross sales without any production gain.
5. Value complete dated plans with actual seed, animal, worker and land spending. Default isolated supply curves omit those costs and opponent responses; they cannot establish a profitable crop replacement.

The notebook requires both players' full private states and tile grids for offline reconstruction, uses a private engine helper, and states that it was tested with `kaggle-environments==1.32.7`. Its accounting implementation was inspected here, not independently executed or revalidated. None of that private information may enter a deployed policy.

## Frozen evidence and reproduction

[LINEAGE.json](LINEAGE.json) records hashes and original paths. [EXTRACTION.json](EXTRACTION.json) records raw source hashes and the extraction method. Original notebooks and metadata remain in `orders/` and `igor/`; prior raw notebooks, pinned V5 source/validation and fresh listing/leaderboard copies are in `reference/`. The fresh parent snapshot is [research/refresh_1545](../../research/refresh_1545/notebook_changes.json).

Run from the repository root:

```sh
conda run -n kaggriculture python experiments/v6/sep07_compositions_v0/runs/public_notebook_audit_1545/extract.py
conda run -n kaggriculture python experiments/v6/sep07_compositions_v0/runs/public_notebook_audit_1545/audit.py
```

These commands perform static processing only. `extract.py` reads the pinned earlier snapshots within the same experiment; `audit.py` refreshes copied references and asserts all stated equivalence checks. [COMMANDS.json](COMMANDS.json) records the original download and analysis commands. [FINAL_AUDIT.json](FINAL_AUDIT.json) freezes every artifact except itself.
