# Funding failure in a two-cow proposal

FUNDING_WITNESS.csv replays the saved executable day8 schedule in the actual
engine against a live public router. At hour0/order8 the farm has406 cash,
requests two400-cost cows, receives one and retains6cash. At hour4 its ordinary
two-sheep purchase still succeeds. The physical day problem assumed two cows
arrive at hour0; its certificate therefore cannot prove full economic execution.
The missing cow leads to a failed feed and one extra ending wheat. The old
3-second and diagnostic failures remain unchanged in their original folders.

compile_v3.cpp uses the existing insert_funded_order helper to find an hour that
pays the whole added animal quantity while preserving other recorded orders.
It changes purchase timing, not requested animal count or productive service.
Its full-engine day8 endpoint passes with the original workforce. Day9 also
passes with one extra hire in the current30-second attempt. A completed season
and paired single-animal decomposition are still needed for an economic claim.

The helper is a funding screen with recorded rival actions and source unit
routes. Every solved day still requires a full live-opponent endpoint check.
No root solver, root engine, previous agent or submitted artifact was changed.
