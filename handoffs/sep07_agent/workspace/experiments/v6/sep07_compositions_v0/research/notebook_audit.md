# Public source audit, September 7

Official Kaggle CLI queries and raw results: notebooks_by_score.json,
notebooks_by_date.json, yhay_notebooks.json. Twelve complete notebooks and
metadata are in notebooks/; notebook_manifest.json records hashes. Source
extraction is static and does not execute notebook setup or install commands.

| Source | Source inspection | Decision |
| --- | --- | --- |
| thomastschinkel/public-state-router | Four prefix-compatible replay routes, three observation branches, weed and stock repairs | Ported as public_router; 8,628-action parity; 251/256 wins versus teammate in discovery |
| tetsutani/shape-the-shop-work-the-pasture | Exact Thomas route blob, extra day-close one-slot capacity reserve | C++ public_capacity_router, 8628 matching source actions; required checks; 72.66% wins versus Thomas in 256 discovery games, loses heavily to v2 |
| destbreso/v7-38-finance7 | yhay81 two-route native chassis, public-state mirror detector and advance sales, hire financing | Source decoded and read; port/test components next |
| destbreso/97-predictable-extracting-decision-trees | Prefix divergence and shallow branch reconstruction from replays | Borrow extraction method; six games/player alone insufficient to establish a robust branch |
| yhay81/three-day-shop-router | Same native backbone as existing teammate | Preserve original provenance; avoid duplicate-family weighting |
| yhay81/six-day-public-state-fieldbook | Native route search lineage and replay study; three C++ modes now ported | 9,347-action parity each; broad tests show opening collapse and reserve tradeoffs; see review 10 |
| lynnsakurai/farming-score-a-mathematical-approach | Refreshed 04:32 source is Tetsutani/Thomas plus final-step full-stock sales; exact diff/hash in refresh_0504 | C++ public_terminal_router, 8628 matching source actions; all 2304 discovery game actions/outcomes equal capacity-only; no measured additional benefit |
| kaitofukami/238-238-known-streams-v58-minimax-closed-loop | Ten controller slots, nine routes, public-capital checkpoint router, residual market-flow selling and weed repair | C++ port complete; 17,256-action source parity; 4/256 wins vs opening_router_v1, 24/256 vs public_router, 248/256 vs Skomuro and all vs Deniz/Arman; retain distinct behavior, not strongest |
| boatlee/v29-r1-adaptive-market-hysteresis | Downloaded recent notebook | Source audit pending; old boatlee port is not equivalent |
| junaid512/02-adaptive-replay-agent | V14 replay with market adjustment; some copied price curves differ from official 1.32.7 | Verify equations/source behavior before reuse |
| dmitriigluzdov/goose-portfolio-historical-lb-2615 | Encoded baseline plus late portfolio layer | Static payload extraction and audit pending |
| busyaprime/800-paired-games-and-the-map-wins | Downloaded recent analysis/code | Source audit pending |
| az05192000gmailcom/kaggriculture-from-orders-to-actual-trades | Junghoon's 04:47 accounting notebook, not competitive policy release; exact accepted-unit, simultaneous-slot and reconciliation method | Independent accounting cross-check; do not treat demonstration agent as the author's strong submission |
| motemen/meta-census-lineage-occupancy | Opening action hashes cluster observable source-family behavior | Same prefix is not proof of full-agent identity or lineage; refreshed source retained for further audit |
| destbreso/x-ray-your-agent | Fresh diagnostic notebook | Source retained; inspect remaining useful profiling methods |

Notebook-reported evaluations have different opponent and seed pools. They are
ideas for testing, not evidence of strength against our league. Exact URLs are
recorded in notebook metadata and manifests. Original source license notices are
preserved; unknown route episode IDs or authors stay explicitly unknown.

## 09:38 audit updates

BoatleeV29-R1 fully audited/ported, 11,504source actions match with forced mirror,
pressure,budgetandweed cases. Requireddeploymentchecks pass. It is much stronger
than archivedBoatlee(255/256wins) but loses heavily to thecurrentleague, including
0/256teammate/Justin/Binghua. Thus earlier notebookreportedstrongpanels do not
transfer directly toourcurrentopponents. Keep its publicflow/reserve component
andcompleteport with exactRngRngsourcecredits; no promotion orratingclaim.

Gooseportfolio sourceparts staticallyextracted: normalizedSHA
b58c152f830eef3abb75ff3006d0f3309f271733df0691ad245742029ce8636c.
The finite-horizon model valuesown+rivalherdproduction, same-product price impact,
visible anddiscountedfuture shops, animal/feed/servicecost. It assumesdailyfull
service/sales andnextdayplacement; routing,storage,finance and future rivalchanges
remain unmodeled. FullMoonbaseandallocationaudit stillpending, no completeportyet.
