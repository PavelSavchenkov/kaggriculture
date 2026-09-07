# Provenance

- Player: `XDang13`
- Official public episode: `89917554`, seat `0`
- Replay: `replays/2026-08-04/89917554.json`
- Replay SHA-256: `793f3e9231150027c090f8a0dfd206082d9d93e6c0a8d4e83a145d6238f75824`
- Engine: `1.32.3`
- Discovery leaderboard snapshot: `2026-09-01T15:07:35`
- Discovery score: `576.4`
- Source action SHA-256: `70ecac2a37b942fd4716d2cbcdecf8869f3d3632581bc3db7d79477783c83275`
- Compiled tape SHA-256: `2aaf411d3340af143ea1a6b6269fbd45e1423692507b71777df926d9c0db7510`

The C++ agent returns the recorded action tape with fixed arrays and no heap allocation. It has no strategic edits, repair, or fallback. The leaderboard score is discovery metadata only; local head-to-head results decide bridge admission.

## Safety derivative

This opponent-only derivative preserves the recorded requested route, then applies the pinned Boatlee H7 generic live legality, variable-price-buy, and day-end capacity sanitizer. The safety layer may delete or clamp requests but does not add a farm strategy, route, purchase, or production target.
