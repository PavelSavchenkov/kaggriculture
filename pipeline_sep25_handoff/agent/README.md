# Best agent: `pavel-bc-opus-v12-forecast` (model folder `fin_I`)

v12_cond network + DSM opening (days 0-5) + day compiler with sales by revenue at stake, wage-aware
same-day returns, hire cap counting shed stock, collection of every animal's fertilizer, opponent sale
forecast from its visible farm, and herd reach on fully funded days 6-14. Results: RESULTS.md.

| File | Content |
|---|---|
| `main.py` | Kaggle/Local-LB entry point (`agent(observation, configuration)`), identical to the shipped one except the docstring |
| `AGENT.toml` | Local-LB display name and author |
| `model/model.bin.*` | sidecars: `compiler`, `condition`, `decode`, `features`, `opening` (weights: `../weights/v12_cond/model.bin`) |
| `package.sh` | builds the portable bridge and assembles a ready folder + `submission.tar.gz` |
| `play.py` | full games of agent folders in kaggle-environments 1.32.7; prints rewards, slowest call, action hashes |
| `reproduce_validation.py` | replays Kaggle's validation episode of a submission and requires identical actions |

```bash
# needs conda env kagbuild once: conda create -n kagbuild -c conda-forge gxx_linux-64=14 sysroot_linux-64=2.28 cmake make
pipeline_sep25_handoff/agent/package.sh work/forecast_pkg
conda run -n kaggriculture python pipeline_sep25_handoff/agent/play.py work/forecast_pkg starter --seeds 1-2
conda run -n kaggriculture python pipeline_sep25_handoff/agent/play.py work/forecast_pkg work/other_pkg --seeds 1300-1303 --both-seats --procs 4
```

Checked on Sep 25: the folder built by `package.sh` plays the same actions as the shipped
`forecast` bridge (seeds 1-2 vs `starter`: rewards 200,434 and 226,525, identical action hashes);
`reproduce_validation.py` reproduced Kaggle's validation episode of `robust` with 0 of 1,440
differing actions.

To package another candidate: `package.sh <out> pipeline_sep25_handoff/candidates/<name>` (the
`vadim` candidate also needs `DC10_TRIM_D0_ANIMALS=1`, which a package cannot carry; use the
Local-LB repository's `agents/pavel-bc-opus-v12-vadim` instead). Kaggle notes: the kernel image ships
kaggle-environments 1.29.3 with old rules; Kaggle `exec()`s `main.py` without `__file__` (handled);
Kaggle datasets auto-extract `.tar.gz` uploads, so upload archives renamed to `*.bin`.
