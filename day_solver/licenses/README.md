# Dependencies and provenance

This package preserves the locally developed Kaggriculture V30 scheduler and
its validated native dependencies. It does not include Crop Dusta source code,
trained weights or original replay schedules. Benchmark inputs were extracted
from public Crop Dusta replays and reduced to the public day contract.

Bundled dependencies include:

- OR-Tools 9.15.6755, Ubuntu 24.04 x86-64 C++ SDK; Apache 2.0 license copied here.
- PyVRP 0.14.0 native core; MIT license copied here.
- spdlog 1.17.0 headers used for the native routing dependency; license copied here.
- The OR-Tools SDK's protobuf, Abseil, Boost headers, COIN-OR, HiGHS, SCIP,
  compression and other dependency files. Original header notices remain in
  `vendor/include`; distributed SDK license files are under `ortools_dependencies`.
- OpenSSL/libcrypto and GNU C++ runtime libraries copied from the validated
  conda environment. They remain their upstream projects' software, not project code.

The exact SDK archive URLs and hashes are recorded in
`../evidence/dependency_sources.json`; every shipped file is covered by the
package checksum list. `../evidence/import_origins.json` and the original seal
provenance record copy origins. This folder preserves available upstream notices
and does not grant a new blanket license over third-party components.

The complete transition engine in `../include/fast_game_engine` was copied from
the sibling engine folder. Its upstream audit and Apache 2.0 lineage are
documented by the copied engine provenance files alongside those headers.
