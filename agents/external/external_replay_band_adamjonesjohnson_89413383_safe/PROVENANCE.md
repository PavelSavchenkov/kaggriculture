# Provenance

- Player: `AdamJonesJohnson`
- Official public episode: `89413383`, seat `0`
- Replay: `replays/2026-08-01/89413383.json`
- Replay SHA-256: `27af26866f0dacb60c2cdba52b06f11cda29c339b19aacaf9952b3a855b69145`
- Engine: `1.32.2`
- Discovery leaderboard snapshot: `2026-09-01T15:07:35`
- Discovery score: `753.5`
- Source action SHA-256: `8f3de8fdeace4daf1715b9c9e2ba6225852d2cae5fe2ac6d4928510cccb92b33`
- Compiled tape SHA-256: `4350dffa65b1f92a06890f09d75176990860a7b08ca35dd887acc4610f8bebea`

The C++ agent returns the recorded action tape with fixed arrays and no heap allocation. It has no strategic edits, repair, or fallback. The leaderboard score is discovery metadata only; local head-to-head results decide bridge admission.

## Safety derivative

This opponent-only derivative preserves the recorded requested route, then applies the pinned Boatlee H7 generic live legality, variable-price-buy, and day-end capacity sanitizer. The safety layer may delete or clamp requests but does not add a farm strategy, route, purchase, or production target.
