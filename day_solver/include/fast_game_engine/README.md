# Fast Kaggriculture engine

Single-threaded C++20 port of the official Kaggriculture 1.32.7 transition engine,
tuned for this machine's Intel Core i7-14700K. The core is header-only: include
`sim.hpp` and compile it into a C++ strategy or search program.

The target is the official competition configuration: a 10x10 board, at most ten
market orders per turn, and the 1.32.7 default market. The implementation includes
all crop, animal, town, shed, land, hire, market lockstep, atomic planting, weed,
decay, and CPython RNG behavior.

## Build and verify

```bash
make -C fast_game_engine
make -C fast_game_engine check
make -C fast_game_engine benchmark
```

The release build uses GCC 13 with `-O3 -march=native -mtune=native`, LTO, and
exceptions/RTTI disabled. It deliberately does not use `-ffast-math`: changing
floating-point price evaluation would put parity at risk.

For a workload-trained binary:

```bash
make -C fast_game_engine pgo
```

This trains `build/bench_pgo` on `TRACE`. PGO should be trained on the strategy
distribution that the final program will actually run.

## API

```cpp
#include "sim.hpp"

kag::Config config;
config.seed = 70117;
kag::Sim sim(config);

kag::Action player0, player1;
player0.clear();
player0.units[0] = {kag::OP_NORTH, 0, 1};
player0.finalize();
player1.clear();
player1.finalize();
sim.step(player0, player1);

double score = sim.reward(0);
```

Call `Action::finalize()` after filling an action. The engine remains correct
without precomputed metadata, but finalization enables the no-plant fast path.
`State` is trivially copyable, so tree search can snapshot it with a plain copy.
No transition allocates heap memory and the engine starts no threads or processes.

## Parity evidence

`make check` replays 56 episodes exported by the official Python engine:

- three dense public-strategy episodes, with up to 15 units and 10 orders;
- three starter-agent episodes;
- two purpose-built edge episodes covering coops, geese, every unit operation,
  every market operation, atomic plant rejection, invalid/no-op paths, animal
  escape/production, and quantities larger than 16-bit action counts;
- 48 deterministic state-aware fuzz episodes: 24 full default games and 24 games
  varying all supported scalar configuration fields, with adversarial operations,
  invalid items, negative/zero/large quantities, up to 29 units, and all crops,
  animals, structures, and shop states.

Every one of the 29,682 recorded states is checked with a canonical 64-bit hash,
not only final reward. The hash covers time, completion, shops, market inventory
and prices, money, positions, land, hires, shed, seeds, insertion-ordered unit
inventories, and every field of every tile. All 29,626 transitions match exactly.
The same corpus also passes through the non-finalized action-metadata fallback.

Supplemental checks compare all 540,009 default market prices for inventories
from -20,000 through 40,000, plus 15,748 mixed sequential CPython RNG outputs.
GCov reports 98.38% executable-line coverage and 98.60% branch execution in
`sim.hpp`; 88.77% of branches are taken in both directions. Remaining untaken
code is defensive, build-variant, or outside the documented default-market scope.
The complete corpus passes ASan and UBSan. See `PARITY.md` for the methodology.

The trace exporter pins both the package version and the audited source digest:

```text
kaggle-environments 1.32.7
bc8a54879ef02c7ea64b8b333d6a976f0ea65c4949149d01f463f23bccee653e
```

## Performance on this machine

Representative workload: both seats replay a dense 719-turn strategy, one
process pinned to one P-core. Values are medians; normal desktop background load
causes small run-to-run variation.

| Engine/build | Episodes/s | Time/episode |
|---|---:|---:|
| Official Python interpreter function | 19.6 | 51.0 ms |
| Official full Kaggle framework | 0.39 | 2,544 ms |
| Initial faithful C++ port | 2,619 | 0.382 ms |
| Optimized release | 4,638 | 0.216 ms |
| Optimized with PGO | 4,709 | 0.212 ms |

The optimized release is about 1.77x the initial C++ port and 237x the direct
official interpreter. The much larger full-framework ratio also includes Kaggle's
observation copying, validation, and agent plumbing, which this core simulator
does not reproduce.

TSC phase instrumentation on the optimized workload attributes approximately
40% to unit actions, 15% to market work, 4% to town demand, 3% to lifespan decay,
and 38% to end-of-day work (including exact CPython RNG). Linux hardware counters
were unavailable because this host has `perf_event_paranoid=4`, so optimization
decisions use phase timing and controlled feature ablations.

## Optimization decisions

- Fixed contiguous state: `State` is 7,816 bytes and `Action` is 464 bytes.
- 128-bit fixed occupancy masks replace repeated board scans for plants, animals,
  empty tiles, and lifespan decay while retaining exact row-major RNG order.
- A next-event decay scheduler avoids inspecting plants before their first legal
  decay tick.
- A 432 KiB static price table covers the competitive inventory range; exact
  scalar evaluation remains the fallback outside it.
- A dirty-product mask recalculates only prices whose inventory changed.
- Time events use next-tick counters instead of repeated integer division.
- PASS actions, empty market phases, unused inventory slots, and atomic-plant
  setup are skipped.
- GCC confirms 256-bit vectorization in the MT19937 twist and state operations.
  An explicit AVX2 town loop was neutral within noise, so the readable scalar
  loop is the production default and compiler auto-vectorization is retained.

Paired ablations around a 4,640 episodes/s baseline reduced throughput by about
8% without the price table, 4% without dirty prices, 36% with full-board decay
scans, 15% without decay scheduling, and 3% with full-board entity scans.

### Static versus dynamic storage

`layout_bench` compares the two-board tile working set as a fixed array and a
reused `std::vector`. Reused traversal was effectively comparable (the vector was
about 10% faster in this isolated compiler-generated loop), while construction
and zeroing cost 57.5 ns for fixed storage versus 175.3 ns for the heap-backed
vector, a 3.05x penalty. Static storage wins for this engine because episodes and
search snapshots are created frequently, sizes are known, and it guarantees no
per-transition allocations—not because vectors are inherently slow to traverse.

The default uses 16-bit bounded shed/seed/unit counts to keep snapshots compact.
`-DKAG_WIDE_COUNTS` switches them to 32-bit for unusually rich custom scenarios;
on the official workload it enlarged state and was about 1% slower.

## Scope

The fast path intentionally fails fast when `board_size != 10`, when more than
16 order slots are configured, or when invalid interval values are supplied.
Sparse `marketParams` overrides and malformed JSON-action coercion are not part of
the typed C++ API. Use the wide-count build for nonstandard configurations capable
of exceeding normal competition inventory counts. Within the official 1.32.7
competition/default configuration and normalized actions, transition state is
fully parity-tested as described above.

Useful commands:

```bash
make -C fast_game_engine comparisons
make -C fast_game_engine extended-check
make -C fast_game_engine vector-report
make -C fast_game_engine profile
make -C fast_game_engine python-baseline
```
