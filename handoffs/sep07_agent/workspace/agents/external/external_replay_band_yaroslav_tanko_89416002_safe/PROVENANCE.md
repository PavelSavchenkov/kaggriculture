# Provenance

- Player: `Yaroslav Tanko`
- Official public episode: `89416002`, seat `0`
- Replay: `replays/2026-08-01/89416002.json`
- Replay SHA-256: `75f185f00cd9377daa3675c1c38eafb80ea8ecbd2ab98b3fac3c36132be54689`
- Engine: `1.32.2`
- Discovery leaderboard snapshot: `2026-09-01T15:07:35`
- Discovery score: `744.4`
- Source action SHA-256: `cc87f593beca538a7740b18489285b62260f574d456790395103d3e407400194`
- Compiled tape SHA-256: `310d9a56f3612c975e4ec48b78e67d95d6e1013c537f8a11da234748f6d354d5`

The C++ agent returns the recorded action tape with fixed arrays and no heap allocation. It has no strategic edits, repair, or fallback. The leaderboard score is discovery metadata only; local head-to-head results decide bridge admission.

## Safety derivative

This opponent-only derivative preserves the recorded requested route, then applies the pinned Boatlee H7 generic live legality, variable-price-buy, and day-end capacity sanitizer. The safety layer may delete or clamp requests but does not add a farm strategy, route, purchase, or production target.
