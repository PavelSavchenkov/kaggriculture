# Provenance

- Player: `Jeff Horon`
- Official public episode: `89417087`, seat `1`
- Replay: `replays/2026-08-01/89417087.json`
- Replay SHA-256: `83d3c034b83322d8bd9b2ff2c50cfe2df491e07d834d5692c597335f78dbcc12`
- Engine: `1.32.2`
- Discovery leaderboard snapshot: `2026-09-01T15:07:35`
- Discovery score: `665.1`
- Source action SHA-256: `658841027023d504ce773aa56774a2efea8254b4cfb5d200ebdf35eef8c233c5`
- Compiled tape SHA-256: `18ff8431e99e939149904f5ad9b4b36eace235917e6bd878fa1477156cbd2436`

The C++ agent returns the recorded action tape with fixed arrays and no heap allocation. It has no strategic edits, repair, or fallback. The leaderboard score is discovery metadata only; local head-to-head results decide bridge admission.

## Safety derivative

This opponent-only derivative preserves the recorded requested route, then applies the pinned Boatlee H7 generic live legality, variable-price-buy, and day-end capacity sanitizer. The safety layer may delete or clamp requests but does not add a farm strategy, route, purchase, or production target.
