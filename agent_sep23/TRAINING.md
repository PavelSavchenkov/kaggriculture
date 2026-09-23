# Data and training reproduction

## Collect replay data without leakage

1. Freeze submission IDs and ranks before downloading. The selected corpus used
   public rank-20 replays only. Store stable team, submission, episode, and seat
   identities.
2. Replay every episode in the exact local engine and verify observation/action
   parity. Reject engine mismatches; do not repair them with future information.
3. Treat both seats and the whole episode as one split unit. Hash the stable
   identity once into 70% train, 15% validation, 10% development, and 5%
   confirmation. Never split dawns independently.
4. Keep confirmation episodes unopened and unanalyzed until the final agent is
   fixed. The frozen selected arrays contain zero confirmation dawns.
5. At each dawn, construct features from the public observation, own private
   state, and only earlier own actions. Future replay actions may define labels,
   never inputs.
6. Convert the expert's future consequences to one semantic `DayIntent` under
   the frozen binder. Record invalid, projected, strengthened, and missing
   labels explicitly. Do not silently drop failure denominators.
7. Export native records, pack arrays, and hash every array plus the selected
   identities. The frozen corpus has 66,720 dawns from 1,864 episodes and 2,224
   perspectives. Full shapes and per-array hashes are in
   `provenance/model_contract.json`.

The compact historical pipeline source is in `training/data_pipeline/`. It is
kept to explain and recreate collection logic; it expects the original
experiment layout and compiled native exporter. The authoritative original
commands and detailed logs remain under `experiments/v9/sep23_BC/`.

To rebuild a new cohort with the committed experiment tooling, first put a
leaderboard CSV in `experiments/v9/sep23_BC/data/current_leaderboard/` and keep
`data/excluded_episodes.json` current. Then run from the experiment directory:

```bash
conda run -n kaggriculture python scripts/collect_replays.py \
  --teams 20 --top-episodes 100 --episodes 40 --workers 3 \
  --name fresh20 --download
conda run -n kaggriculture python scripts/export_traces.py \
  --selection fresh20_selection.json --workers 4 --processes
conda run -n kaggriculture python scripts/build_corpus.py \
  --name fresh20_complete --workers 8
```

Before the second command, rebuild `build/libtrace_hash.so`; before the third,
rebuild `build_native/bc_export`. Use the experiment's `source/trace_hash.cpp`,
`source/export.cpp`, and CMake target `bc_export`, and retain the generated
compiler manifest. `build_corpus.py` independently exports shards, packs them
with `bc.data`, writes per-array hashes, and refuses to open confirmation rows.
Treat `summary.json:data_hash` as the corpus identity. The selected model can be
reproduced only from the exact frozen hash below; a newly collected cohort is a
new experiment and needs new evaluation.

## Exact selected training

Requirements: the `kaggriculture` conda environment, CUDA, PyTorch, and the
frozen `fresh20_complete/arrays` directory whose summary data hash is
`0ef20bdc…`. Both stages use AdamW, learning rate 0.001, weight decay 0.001,
batch 512, seed 2301, gradient clipping at 1, strict FP32 validation, no AMP,
and highest matmul precision.

Stage 1 uses all eligible rank-20 data for 82 epochs:

```bash
conda run -n kaggriculture python agent_sep23/training/reproduce.py \
  --data experiments/v9/sep23_BC/data/fresh20_complete/arrays \
  --output work/agent_sep23_reproduction
```

`reproduce.py` runs both stages. Its equivalent stage-1 flags are width 128,
depth 2, `--financial --coordinated --accounting --space-mask
--product-plans`, rank 20, 82 epochs, batch 512, four CPU threads, and checkpoint
saves every 20 epochs.

Stage 2 initializes from stage 1 `best.pt`, filters to submission 56424800,
uses two CPU threads and compiled loss, and runs 500 epochs with saves every 125.
Always export stage 2 `best.pt`; the chosen validation minimum is epoch 21.

The final export is:

```bash
PYTHONPATH=agent_sep23/training/stage2 \
conda run -n kaggriculture python -m bc.export \
  work/agent_sep23_reproduction/stage2/best.pt \
  work/agent_sep23_reproduction/model.bin
```

The reproduced binary must hash to
`9c22aaa6051568a592afa39e710468f9f0a9e34edfede89271c6a207d5de47e8`.
If it does not, stop: check the corpus hash, source hashes, selected best epoch,
CUDA/PyTorch environment recorded in the contracts, and export flags before
evaluating gameplay.

## Why two stages

The broad stage learns the game-wide semantic representation and rare states.
The narrow stage learns the strongest selected submission's style with only
2,010 training dawns. Initialization is essential; training that narrow set
from scratch is data-starved. Validation-based checkpoint selection is also
essential because the narrow run overfits quickly.
