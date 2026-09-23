# agent_sep23

`agent_sep23` is the best evidence-backed behavior-cloning agent from
`experiments/v9/sep23_BC/`. It is a complete C++ game-playing agent: a dawn
policy predicts semantic `DayIntent`s, the day compiler turns them into legal
hourly actions, and a reactive executor repairs schedules as public state
changes.

## Selected production identity

| Part | Selected artifact |
|---|---|
| Model | submission 56424800 transfer run, best checkpoint at epoch 21 |
| Model SHA-256 | `9c22aaa6051568a592afa39e710468f9f0a9e34edfede89271c6a207d5de47e8` |
| Native model | `model/model.bin`, 2,037,777 bytes, `BCW23004` FP32 format |
| BC architecture | width 128, depth 2, causal history, financial/coordinated/accounting/space-mask/product-plan features, 509,359 parameters |
| Day compiler | product-specific collection fallback |
| Compiler source hash | `ed154431f1983df6c9b6d4fc411791cd6eb6417aec363b12d8c975cd6c1a02b4` |
| Production options | collection refinement on, recovery mode 1, trusted reactive executor, 256 worker attempts |

The model and compiler are a pair. Do not replace one and attribute the result
to this release without rerunning the gates in `EVIDENCE.md`.

## Where to start

- `DESIGN.md` and `design.pdf`: exact policy/compiler contract and rationale.
- `LINEAGE.md`: what changed, what evidence selected this pair, and rejected
  alternatives.
- `TRAINING.md`: replay collection, leakage controls, exact two-stage training,
  and native export.
- `EVIDENCE.md`: quality gates, latency, local-LB results, full-game failures,
  and the most useful next experiments.
- `provenance/`: compact machine-readable records copied from the selected run.
- `training/`: frozen Python source for both training stages and the data-pipeline
  source; `reproduce.py` executes the exact two-stage recipe.
- `source/`, `day_compiler/`, and `model/`: production runtime.

Original detailed logs remain at `experiments/v9/sep23_BC/`; this folder keeps
the release-critical source, model, contracts, and summaries so the agent does
not depend on uncommitted experiment files.

## Build and smoke test

From the repository root:

```bash
conda run -n kaggriculture cmake -S agent_sep23 -B work/agent_sep23_build -DCMAKE_BUILD_TYPE=Release
conda run -n kaggriculture cmake --build work/agent_sep23_build -j 2
conda run -n kaggriculture ctest --test-dir work/agent_sep23_build --output-on-failure
```

The manifest is `agent.json`; its agent type is
`kag::agents::agent_sep23::Agent`. The default constructor loads
`agent_sep23/model/model.bin`. A packager that relocates the model should define
`BC_DEFAULT_MODEL_PATH` for `source/agent.cpp`.

Run `conda run -n kaggriculture python agent_sep23/verify_artifacts.py` before
using or modifying the release.
