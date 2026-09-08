# Changed public notebooks — September7,19:02 refresh

Downloads and exact hashes are in NOTEBOOK_PULLS.json and each notebook's
PULL.json. No notebook code was executed. AST literal extraction and payload
decoding are reproduced by audit_notebooks.py.

## V5 Hybrid

Source: https://www.kaggle.com/code/souvikdbiswas/kaggriculture-v5-hybrid-agent

All five719-turn tapes, all decision trees, feature extraction and tree traversal
are exactly the Thomas V5 controller already ported as public_router_v5. The
new callable adds weed repair and active-worker normalization. Normalization
already exists in our C++ parent. The added rule clears a weed beneath a worker
when its scheduled action is PLANT, BUILD_COOP, BUILD_PASTURE, WATER or PASS.
The Python exception fallback has no counterpart because typed observations
are valid and local C++ agents must not throw across the API boundary.

The C++ variant is ../../league/public_router_v5_repair. Exact comparison is
V5_NOTEBOOK_COMPARISON.json; parent lineage and unknown original tape provenance
remain in ../../league/public_router_v5/IMPORT.json. Do not attribute all five
underlying schedules to the new notebook author.

896 discovery games compare repaired and original V5 against seven opponents,
including self-play and PASS. Win counts stay identical. Mean margin changes
range from0 to+$6.69 against active opponents and-$4.86 against PASS. Some unit
faults disappear. This is a small repair, not a new strong strategy family or
an improvement over our incumbent. V5_REPAIR_DISCOVERY.json contains all results.
Keep it as a diagnostic variant; broader operational/source parity remains
required before promoting it as a separately validated league reference.

## Fields of Fortune

Source: https://www.kaggle.com/code/ayeshasummaiyya/fields-of-fortune-strategic-farming-agent

The source is a reactive crop controller with per-day labor targets based on
owned tiles, geographical worker zones, nearest-shed placement and a ranked
mixture of up to three crops. These are useful concrete cold-start ideas.

The callable explicitly sets top_animals to an empty set. Animal purchases and
structure growth are therefore disabled. Its animal-value function subtracts
purchase cost from base product revenue but omits feed, care bonuses, fertilizer,
labor, displaced crops and price impact. Its ongoing-crop estimate assumes
doubled production without charging the required fertilizer/service. The local
crop action harvests at maturity before considering watering/fertilizer; that
can miss same-day yield. There is no supplied verified strong-opponent result
that justifies porting the entire1400-line controller ahead of our current work.

Retain zones/placement as ideas, but do not treat this notebook's ranking
formulas as evidence that a functioning adaptive animal agent is available.
Our dated animal estimator already models the missing biology and market flow.

## Can Specialists Beat One Agent

Source: https://www.kaggle.com/code/mansiaggarwal88/kaggriculture-can-specialists-beat-one-agent

This is a presentation/timeline and plotting notebook, without an executable
game policy. It reports several failed specialist/assignment approaches and
local-to-leaderboard gaps, but the underlying match records and agents are not
included. Treat the numbers as author claims. No C++ port is available from
this content. The practical lesson to test is whether assignment, feasibility
and market decisions remain consistent, not whether a named architecture wins.
