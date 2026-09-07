# Session seal

Sealed on 2026-08-30 from the six-hour one-random-shop search session.

- Scenario: random weeds `0.005`, PASS opponent, only the unknown first day-3
  shop enabled.
- Objective: `0.8 * mean(cash) + 0.2 * lower-CVaR10(cash)`.
- Primary audit: 2,048 independent seeds in both seats for every shop; 4,096
  games per shop and 32,768 total.
- Result: mean `122794.218903`, CVaR10 `107444.379089`, J `119724.250940`.
- Integrity: zero failed actions and zero discards; independent same-size audit
  J `119723.583325`.
- Last strict policy improvement: 2026-08-29 23:41:40 BST.
- Source SHA-256:
  `0b351b248910a4957a377d351bd53275e87a95e39d5e4567308886f8ecb697f9`.

The exact vendored source passed optimized and ASan/UBSan builds across every
shop and both seats. `SHA256SUMS` covers this seal and every agent artifact.
