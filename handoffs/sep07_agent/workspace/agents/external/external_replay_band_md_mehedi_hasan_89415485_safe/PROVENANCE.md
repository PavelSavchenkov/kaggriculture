# Provenance

- Player: `Md. Mehedi Hasan`
- Official public episode: `89415485`, seat `0`
- Replay: `replays/2026-08-01/89415485.json`
- Replay SHA-256: `5b8de1efc91180cfab9da49b981a9f24a6013f6cb4b31901b839da698bb1b1a9`
- Engine: `1.32.2`
- Discovery leaderboard snapshot: `2026-09-01T15:07:35`
- Discovery score: `819.6`
- Source action SHA-256: `9343a294b58e1de84249cf2cdaccc630b969489c2e0b4873418e09192eac6e44`
- Compiled tape SHA-256: `4c0c4fe47d2bc1703368fdc3eb2d25a4687c58fa1cce6605acea7b0ff056fb22`

The C++ agent returns the recorded action tape with fixed arrays and no heap allocation. It has no strategic edits, repair, or fallback. The leaderboard score is discovery metadata only; local head-to-head results decide bridge admission.

## Safety derivative

This opponent-only derivative preserves the recorded requested route, then applies the pinned Boatlee H7 generic live legality, variable-price-buy, and day-end capacity sanitizer. The safety layer may delete or clamp requests but does not add a farm strategy, route, purchase, or production target.
