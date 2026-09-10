# corpus — Kaggriculture replay codec

Canonical, information-complete storage for downloaded Kaggle replays, plus the
C++ loader that lets the mining stack consume them.

A Kaggriculture episode is fully determined by `(configuration, seed, action
stream)`: replaying the recorded actions through the pinned 1.32.7 engine
regenerates every state bit-exactly. So the canonical stored form is the action
stream, and **the engine is the decompressor**. Codec correctness is exactly
engine parity.

| | |
|---|---|
| 44 unique episodes, raw | **1.42 GB** |
| packed `data/*.kagz` | **1.43 MB** (27–85 KB each, **994×**) |
| re-simulate whole corpus in C++ | **11.4 ms** (0.259 ms/episode) |
| verify whole corpus with per-state parity hash | 180 ms |

The three largest containers (83–85 KB) hold episodes where the `carbonapi`
opponent echoed its entire input observation back inside its action for 160
turns; those keys are preserved verbatim rather than dropped.

## Why this exists

`day_contract.hpp` and the composition tooling consume a `kag::Sim` plus per-hour
`kag::Action` values. Nothing in the repo could turn a downloaded replay into
those — `fast_game_engine/export_trace.py` only generates fresh episodes locally.
`include/corpus/episode.hpp` is that seam.

## Verification

Every claim is gated by re-execution, not assertion. All three gates pass on
44/44 episodes:

| gate | check | result |
|---|---|---|
| ingest | `pack` replays the action stream through the official engine and refuses to write unless every one of the 720 recorded observations matches and final money equals the recorded rewards | 44/44 |
| round trip | `verify --roundtrip` rebuilds the complete replay document and compares it to the source file | 44/44 exact |
| cross-engine | `trace` export replayed by `fast_game_engine/build/validate` | 44/44, 31,636 transitions, 0.24 s |
| C++ loader | `verify_corpus` loads `.kagz` directly and checks the canonical parity hash of all 31,680 states | 44/44 |

"Exact" round trip means the reconstructed document compares equal (`==`) to the
parsed original, including `remainingOverageTime` at full double precision and
every non-standard action key. JSON object key *order* is not preserved, being
semantically irrelevant.

## Usage

```bash
python3 kagz.py pack ../replays -o data        # ingest + certify (~4 s/episode)
python3 kagz.py verify data                    # re-replay through the Python engine
python3 kagz.py verify data --roundtrip        # also rebuild and diff the source JSON
python3 kagz.py ls data                        # manifest table
python3 kagz.py unpack data/106590253.kagz -o out.json
python3 kagz.py trace data -o traces           # fast_game_engine trace .txt

make && ./build_verify data/*.kagz             # C++ loader + parity check
```

`pack` and `verify` parallelise well: `ls data/*.kagz | xargs -P 8 -n 1 python3 kagz.py verify`.

## C++ API

```cpp
#include "corpus/episode.hpp"           // header-only; needs -lz and sim.hpp

corpus::Episode ep;
corpus::load("data/106590253.kagz", ep);

// actions[t] is the action recorded on state t, i.e. the one that drove
// t-1 -> t. Index 0 is the framework's placeholder and is never applied.
kag::Sim sim(ep.config);
for (int t = 0; t + 1 < ep.n_steps; ++t)
    sim.step(ep.actions[t + 1][0], ep.actions[t + 1][1]);

corpus::verify(ep);                     // re-simulate, check every parity hash
kag::Sim start = corpus::at_day(ep, 12);// state at the start of day 12
```

`at_day` plus the 24 hourly actions of that day is exactly the input
`day_contract.hpp` wants.

## The off-by-one

`steps[t].action` is recorded on the state it **produced**, not the state it was
chosen from. The agent that emitted it saw `steps[t-1].observation`. Anyone
mining `(state, action)` pairs must use that shift; pairing `obs[t]` with
`action[t]` leaks the action's own effect into its input.

Confirmed independently by the `policy_observation` key that `carbonapi` attaches
to its actions: at step `t` it contains exactly `steps[t-1]`'s observation (plus
a `step` field).

## Files

| file | role |
|---|---|
| `kagz_format.py` | pure container read/write, no engine dependency |
| `kagz.py` | CLI: pack / verify / unpack / trace / ls |
| `kaggle_index.py` | resumable Episode API crawler for submission IDs and ratings |
| `include/corpus/episode.hpp` | header-only C++ loader + parity verifier |
| `verify_corpus.cpp` | C++ verification and timing CLI |
| `data/*.kagz` | the corpus (committed; 1.43 MB) |
| `traces/*.txt` | derived interop export (not committed; regenerate with `kagz.py trace`) |

## Container layout

```
header : magic "KAGZ", version, engine version + source sha256 (decode refuses
         under a different engine), episode id, seed, step/seat counts, the 13
         configuration fields, team names, statuses, rewards, provenance blob,
         action symbol table, parity hash of every state, chain digest
body   : zlib, per step per seat -- farmer/hands/market as element lists over the
         symbol table, then remainingOverageTime as an exact-value table, then
         any non-standard action keys verbatim
```

Actions are stored as generic symbol/int element lists rather than the
normalized `(op, arg, count)` triples `fast_game_engine`'s trace format uses.
That costs a few bytes and buys byte-lossless reconstruction: the triples cannot
distinguish `["PICKUP","COW"]` from `["PICKUP","COW",1]`, nor preserve a stray
argument like `["HIRE",70]`. Both occur in real replays.
