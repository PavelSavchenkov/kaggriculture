# opening_router_v1

Retains opening_router_v0's turn-1 public-worker selector and public_router
branch. The delayed-hiring branch now uses advance_sales_001_7: program 55
with day 18's 14-strawberry sale moved from hour 22 to hour 18 and its complete
worker schedule rebuilt by V30. Composition, input purchases and biological
day endpoint remain fixed in that reconstruction. Full source and derived
schedule lineage is in the referenced package and runs/advance_sales_001/.

The two branches share the initial action and commit before their first market
divergence. No identity, seed, hidden inventory or future shop is accessed.
Route choice resets per instance. The old version remains in the league.
Checks pass: generic/debug pair and independent-thread parity in 16 games,
PASS and self-play. Independent 1,024-game comparisons win 989 versus v0 and
all versus each of public_router, Skomuro, Deniz, Arman and the two tested
teammate variants. Results and limits are in research/review_11.md.
New Mao and 3정훈 replay courses already counter the parent; universal dominance
is not established by improvements against the older response families.
