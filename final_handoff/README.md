# Kaggriculture: final handoff (Aug 26 – Oct 1, 2026)

This folder is the end-to-end record of our Kaggriculture work. It covers how the agent evolved and why, every important positive
and negative result, the final agents (exact Kaggle archives and the means to rebuild them), the best networks and forecasters,
the code that trained and evaluated them, and the team logs behind each decision.

Read in this order:

| # | File | What it answers |
|---|---|---|
| 1 | [01_PROGRESSION.md](01_PROGRESSION.md) | The tutorial: each phase, what we tried, why, what happened, what we learned |
| 2 | [02_ARCHITECTURE.md](02_ARCHITECTURE.md) | How the final agent works: network -> decode -> day compiler -> market seller; every model file and key |
| 3 | [03_EVALUATION.md](03_EVALUATION.md) | How we judged changes: test beds, their biases, statistics, and the rules we ended with |
| 4 | [04_LEARNINGS.md](04_LEARNINGS.md) | Catalogue of what worked and what failed, with numbers |
| 5 | [05_MODELS.md](05_MODELS.md) | The behaviour-cloning networks and opponent forecasters: data, training, selection, files |
| 6 | [06_FINAL_AGENTS.md](06_FINAL_AGENTS.md) | The final Kaggle pair and Local-LB entries: what differs, checks, how to rebuild and submit |
| 7 | [07_PROCESS.md](07_PROCESS.md) | How the team of AI sessions worked, the rules the user set, and process mistakes to avoid |

## Final state in one table

| Role | Agent | Kaggle | Notes |
|---|---|---|---|
| Final pick 1 | `pavel-bc-opus-v17d-dc12m3-d3crop-m68` ("base", "m3") | 56720080 (re-submit of 56690263) | Most-tested agent: 152 live games, settled 2807.8, 54.5% vs the top 10 |
| Final pick 2 | `pavel-bc-opus-v17d-dc12m19-honest-m68` ("honest1") | 56720831 | Base + clean forecaster + splitfert + hire-funding fix; no part fitted to our own lineage |
| Local-LB #1 | `pavel-bc-opus-v17d-dc12m19-fc2ens-m68` ("f1") | not submitted | 1651 on the team Local-LB; part of its lead is knowledge of our own agents |
| Local-LB honest entry | `pavel-bc-opus-v17d-dc12m3-fc3nv-m68` | not submitted | Base + clean forecaster only; Local-LB #3 (1524) |

Best Kaggle public score reached by any of our agents: **2878.0** (`pavel-bc-opus-v17d-dc11v59-ens-d3crop`, Sep 28). Scores are
ratings against a pool that got much stronger during the last days. The same archive (the base) settled at 2807.8 on Sep 30 and
started at 2626 when re-submitted the same evening. Compare agents only within the same time window ([03_EVALUATION.md](03_EVALUATION.md)).
The full submission history is in [kaggle_submissions.csv](kaggle_submissions.csv).

## Folder map

| Path | Contents |
|---|---|
| `agents/` | The four agents above. Each has its text model files, `WEIGHTS.txt` (binary model files -> `weights/`, sha256), bridge `source/` + `standalone/` CMake (Kaggle agents), the exact submitted `.tar.gz` (Kaggle agents), and `checks/` (local verification, Kaggle kernel output, validation replay + its exact local reproduction) |
| `agents/assemble_agent.py` | Rebuilds a runnable agent folder (weights with hash checks + bridge compiled from source). Tested: the rebuilt `locallb_m3_fc3nv` plays the recorded self-play game exactly (87,469 / 87,469) |
| `weights/` | Every distinct binary once (121 MB): main network `v17g6ft5`, the four ensemble members, forecasters `big2` (base's), `fc2` (3 seeds), `fc3nvens` (3 seeds, clean), the next-morning head. `SHA256SUMS` |
| `code/` | Network and forecaster training, day-compiler patches and key docs, the bed engine source, the evaluation tools (beds, exact Local-LB judge, compute gate), submission tooling. See [code/README.md](code/README.md) |
| `evidence/` | Decision logs: the Imitation ledger (every decision with time and numbers), the team state file, each session's findings, the honest-line part list |
| `kaggle_submissions.csv` | All 50 recent team submissions with dates, descriptions and public scores |

## Quick start

```bash
# rebuild and play one agent (from the repository root; conda env "kaggriculture")
conda run -n kaggriculture --no-capture-output python final_handoff/agents/assemble_agent.py kaggle_56720831_honest1 /tmp/honest1
# re-verify an exact submitted archive in the official environment (4 games, both seats, timing)
cd final_handoff/agents/kaggle_56720080_base_m3 && conda run -n kaggriculture --no-capture-output python verify_submission.py \
    --archive pavel-bc-opus-v17d-dc12m3-d3crop-m68.tar.gz --output /tmp/VERIFICATION.json
```

Built with the official `kaggle-environments` 1.32.7. The Kaggle kernel image ships 1.29.3 with older rules, so kernel checks
install 1.32.7 from a dataset first (see `agents/*/checks/kaggle_kernel/check.py`).

## Related earlier handoffs in this repository

- `handoffs/sep07_agent/`, `handoffs/sep08_agent/`: the composition-search / fixed-course era (Sep 7–8).
- `agent_sep23/`: the first DayIntent behaviour-cloning agent with a day compiler.
- `pipeline_sep25_handoff/`: the BC Opus v12 pipeline (Sep 24–25) that everything after it is built on.
- Local-LB repository (`T3pp31/kaggriculture-localLB`), folder `mm_copy_handoff_sep29/`: the M&M-copy networks and team findings (Sep 29).
