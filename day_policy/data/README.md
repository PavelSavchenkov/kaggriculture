# Replay test data

174 unique episodes, 207 player-games, 6,210 player-days. The unrestricted
extractor retains 5,310 compatible days: development 2,239 and combined
validation 3,071. The old restricted extractor retained 2,198 days.
`COHORTS.json` records exact selection order, game IDs, seats, player names,
snapshot ranks and selection rules. The development list includes only the
selected development split. These are recorded ranks, not a current leaderboard.
`SOURCE_PROVENANCE.json` records the original downloaded JSON hashes, sizes,
rewards and statuses for every included episode; those large JSON files are
not required or included.

Each `replays/<episode>.txt.gz` contains one native action trace, compressed with
gzip level 9 and timestamp zero. Store an episode once even if both seats are
used. `TRACES.json` records decompressed size and SHA-256. The stored traces total
5,076,349 bytes; the decompressed text totals 28,571,255 bytes. Both are under
50 MB. Native dawn/layout struct files are generated test outputs, not source
data, and are deliberately omitted.

The storage follows the same principle as the repository's replay corpus:
seed + configuration + actions reconstruct states through a pinned engine.
No corpus files or codec are needed at runtime. This package uses its existing
normalized trace format, not `.kagz`; it does not reconstruct the original JSON,
nonstandard agent metadata or overage-time fields.

## Decompressed trace format

- Seed and 719 action transitions.
- `CONFIG` followed by the engine's eleven gameplay configuration fields.
- `ENGINE`, version 1.32.7 and official-source SHA-256.
- Two normalized action records per transition: unit/order counts, then
  `(operation, argument/item, quantity)` triples in execution order.
- `TRUTH` followed by 720 state records: both rewards, market inventory and
  canonical full-state parity hash.

`tools/trace/case.hpp` defines parsing and validation. It rejects engine identity
mismatches and checks rewards, inventory and full-state hashes at every step.
The extractor and progression harness retain both players' original requests.
The first action record drives dawn to the next state; each following truth
record describes the state after that action. No original worker assignment is
given to the day policy.

Run `tools/test.py prepare` as shown in the root README. It expands traces and
regenerates native ABI-dependent case files outside this folder, with per-day
exclusions and geometry audits. Rebuild these files after changing the compiler,
engine or input structs. No network downloads or Python game engine are needed.
