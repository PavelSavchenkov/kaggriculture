# premium_sales_m0

Parent empty_sale_slots_m2; stable positive non-input SELL priority from step720.
Copied behavior: AhmedV24's entry-point market-order transform, which credits
KingRC4's transaction-ordering idea. Source notebook and exact AST difference
are in ../../IMPORT.json. Relative order within both groups is preserved.
The transform has no mutable state; the parent resets its per-game state.
No extra observations, quantities, unit actions or model inputs are introduced.
Local C++ source parity, operations and competitive evaluation pending.
Artifact's Kaggle rating is unknown. Apache2.0 attribution and license retained.
