# Shared findings: index (read-only)

One file per session, each written only by its owner (no write races). Read all of them before starting new work.
- findings/day_compiler.md: Day compiler (dc11 / dc12 rework). It also holds the team's stop list (proven not promising);
  send stop-list additions to the Day compiler session and it adds them.
- findings/imitation.md: Imitation (M&M copy, duel beds, league, sales).
- findings/weaknesses.md: Weaknesses analysis (validation, live).
- findings/bc.md: BC / kaggriculture-8b (networks, packages, hand-off).

Conceptual framing and the rework plan: experiments/v10/sep29_dc12/REWORK.md (dc12 home: experiments/v10/sep29_dc12/).
