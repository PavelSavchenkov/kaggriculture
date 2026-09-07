# firstday_m85_v0

Program 55's complete worker and later market course, with only day 0's market
component borrowed from Mao program 85. Exact source IDs, seats, submissions,
hashes and global snapshot are in league/feeltheagi_55/IMPORT.json and
league/mao_85/IMPORT.json. No new branch or hidden input; current worker count
is normalized through the verified replay library.

The first market difference is a one-unit wheat round-trip reduction at step
6, but that edit alone does not explain the gain. Nine causal component
combinations were tested in results/market_course_audit_001 and _002. On 256
discovery games, this combination wins all against opening_router_v1, keeps
parent55's cash/margin versus public_router and wins 79.3% versus Mao85. It
still loses all to Junghoon78. Full-game generic/debug/thread, PASS and self
checks pass. The combined router is independently gated as opening_router_v2;
this component alone has no universal superiority claim. No fixed physical
endpoint is assumed for a market component swap.
