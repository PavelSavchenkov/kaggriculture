# External agents

This catalog keeps materially different public, teammate, and replay-derived
C++ policies. Near-duplicate parameter variants and superseded ports remain in
self-contained experiments instead of becoming separate official agents.

## Primary competitive references

| Agent | Role |
|---|---|
| `investment_context_guarded_001_best` | Strongest broadly validated local policy on September 7; adaptive herd investment with guarded V30 routes |
| `teammate_shoprouter` | Teammate's strongest retained `shoprouter-rl-v2`, ported to the local C++ API |
| `atakan_demand` | Three-course cow/sheep/goose portfolio selected from observed shop demand |
| `atakan_integrated_s64_margin` | Complementary Atakan portfolio selected by 64-scenario margin integration |

## Current public controllers

| Agent | Distinct behavior |
|---|---|
| `king_rc4` | Multi-route public controller with liquidity, recovery, and terminal layers |
| `public_router_v5` | Thomas five-tape staged public-state router |
| `kaito_v58` | Ten-route public controller with shop, capital, equality, and market-flow state |
| `titan_frontier` | Kaito routes plus similarity-gated finished-product sale advance |
| `public_kaito_h10_front_run_fast` | Earlier optimized Kaito front-running lineage |
| `boatlee_h7_fast_sanitize` | Boatlee public route portfolio |
| `c68_h18_fast_sanitize` | C68 adaptive public lineage |
| `public_indar_e279_unchanged` | Indar market-choice policy |
| `public_roman_hamburger_anchor_unchanged` | Hamburger-anchor policy |
| `public_skomuro_2000_cpp` | Independent 2,000+ route baseline |

## Replay phenotypes

| Agent | Source role |
|---|---|
| `replay_crop_dusta_ge3000_100223989` | Crop Dusta top-player course |
| `replay_ryo_ge3000_95029942` | Ryo Hasegawa top-player course |
| `replay_subramanya_ge3000_96594837` | Subramanya N top-player course |
| `replay_arman_ge3000_94541153` | Arman Tuganbaev top-player course |
| `test_replay_mrkiwi_ge3000_93167917` | MrKiwi top-player course with safe extra-worker handling |
| `external_replay_band_adamjonesjohnson_89413383_safe` | Older calibrated replay-band role |
| `external_replay_band_jeff_horon_89417087_safe` | Older calibrated replay-band role |
| `external_replay_band_md_mehedi_hasan_89415485_safe` | Older calibrated replay-band role |
| `external_replay_band_xdang13_89917554_safe` | Older calibrated replay-band role |
| `external_replay_band_yaroslav_tanko_89416002_safe` | Older calibrated replay-band role |

`external.json` is authoritative per-package metadata. A Kaggle rating is
`null` when no defensible rating was recorded; a notebook title or local match
result is not converted into a guessed rating. Replay snapshot ratings describe
the player when discovered, not proof that the recorded episode used the same
submitted policy.

External source may be studied for common mechanisms and failure modes. It may
not be linked into, copied into, or used as an opaque behavior block in an
in-house candidate.
