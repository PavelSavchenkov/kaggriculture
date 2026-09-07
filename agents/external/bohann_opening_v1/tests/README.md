# Reproduce the catalog agent checks

Run from the repository root with the `kaggriculture` conda environment, CMake and
a C++20 compiler. The recorded build used GCC 13.3.0 on Linux x86-64. No experiment,
download, GPU, day-solver runtime, or generated cache is needed. Builds and new
results belong under `work/`, not inside this agent package.

```bash
conda run -n kaggriculture cmake -S agents/external/bohann_opening_v1/tests -B work/bohann_checks/build -DCMAKE_BUILD_TYPE=Release
conda run -n kaggriculture cmake --build work/bohann_checks/build --target arena pair debug -j2
```

`arena` provides runtime selection of this agent, its included crop-mix parent,
the catalog teammate, last submitted investment agent, King RC4, public router V5,
and PASS. `pair` specializes this agent versus the teammate. `debug` validates the
same policy with assertions enabled. Existing investment and King headers share
a public-router class name, so the generic opponent adapters use separate
translation units; the new agent isolates all its own strategy symbols.

`--games N --seat-mode both` runs **2N complete games**, one per seat for each
seed. The output's `games` array contains every result. `--native-shops` uses the
official engine RNG for shops and weeds; without it, the runner uses a separately
seeded shop stream revealed only when the official engine unlocks each shop.
The latter supports paired strategy comparisons when different actions consume
different weed randomness. Every run validates action metadata and 719-turn
completion. Policies receive only the legal observation API in both modes.

To reproduce the new 4,096-game teammate and last-submission matches:

```bash
conda run -n kaggriculture work/bohann_checks/build/arena --a bohann_opening_v1 --b teammate_shoprouter --games 2048 --seed-start 1780000 --seat-mode both --threads 4 --validate --native-shops --output work/bohann_checks/teammate.json
conda run -n kaggriculture work/bohann_checks/build/arena --a bohann_opening_v1 --b investment_context_guarded_001_best --games 2048 --seed-start 1780000 --seat-mode both --threads 4 --validate --native-shops --output work/bohann_checks/investment.json
```

Use the same command with `--b king_rc4` or `--b public_router_v5` and
`--games 512` for their 1,024-game audits. Use `--seed-start 1750000 --games 512`
without `--native-shops` to reproduce the old frozen-source panel against each
of those four opponents or `crop_mix_t2_wheat`. Native source comparisons use
`--seed-start 1753000 --games 128 --native-shops` against the parent, teammate,
King and V5.

For generic/pair/debug/thread parity, run this command with each executable and
different output filenames; also repeat `arena` with `--threads 1`:

```bash
conda run -n kaggriculture work/bohann_checks/build/arena --a bohann_opening_v1 --b teammate_shoprouter --games 32 --seed-start 1000 --seat-mode both --threads 4 --validate --output work/bohann_checks/generic64.json
```

The four `games` arrays must be exactly equal, including cash, opponent cash,
both action hashes, production, sales, discarded quantities, failed actions and
workforce counts. Ignore only top-level runtime measurements when comparing
reports. Full self-play uses `--b bohann_opening_v1 --games 16 --seed-start 1753000`;
PASS uses `--b pass --games 128 --seed-start 1753000`, both without native shops.
The batch runner owns and resets two distinct agent instances per worker.

[../VALIDATION.json](../VALIDATION.json) lists every executed command, result,
artifact hash and matching reference hash. The lossless files in `evidence/`
contain the exact full results, including both action hashes; decompress a file
and compare its `games` array with a new run. `REPOSITORY_DEPENDENCIES.json` pins
the shared engine/API and opponent source bytes used by these checks. Changes to
those dependencies can change results and should be treated as a new validation.

The old research runner is not needed to reproduce the checked agent. Its profile
and evaluation helpers were copied into `support/` solely for these C++ tests;
neither helper is part of the deployable agent manifest. No engine or day-solver
implementation was copied. The package's historical source paths are attribution
only; all build includes resolve within this package or the committed catalog,
shared API/runtime and engine.
