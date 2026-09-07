# September 7 agent handoff

This folder preserves the **exact last submitted agent**, the development pipeline and evidence that led to it, later validated agents, and unfinished work at the handoff checkpoint. It contains the session's C++ source and agent packages, original replay data, evaluation records, positive and negative experiments, and component attribution. Large evidence files are compressed without losing their original bytes.

**Submitted:** `shop_herd_s6_m3_g1`, Kaggle submission **56074695**, status **COMPLETE**, uploaded September 7 at 10:21:58 UTC. The directly submittable file is [submitted/submission.tar.gz](submitted/submission.tar.gz); its only archive member is `main.py`. Archive SHA256: `53acf9d1d5e3206e28ea6f365af39786a9e74c4b3bfe6851b5bd21e8740901f4`.

The submitted policy reconstructs Justin Lee's farm course, adds terminal recovery, uses 22 locally rebuilt worker schedules, and changes two day-7 cow purchases to sheep when already observed wool demand exceeds milk demand. It predates the general goose/cow/sheep/wait evaluator and the later workforce repairs. The Python submission adapter is generated from the authoritative C++ policy and was checked action by action.

## Start here

1. [Development history](docs/development_history.md): how the strategy changed, what evidence motivated each change, intermediate versions, and corrected metrics.
2. [Submitted component lineage](docs/submitted_lineage.md): donor players, episodes, submissions, borrowed ideas, locally written components, and exact hashes.
3. [Learnings and failed approaches](docs/learnings.md): what the composition idea, estimator, day solver, replay borrowing, and adaptation actually achieved and where they failed.
4. [Reproduction instructions](docs/reproduction.md): rebuild the identical submission, repeat teammate tests, rebuild intermediate/WIP agents, restore evidence, and rerun the day compiler.
5. [Metrics and version map](docs/metrics_and_versions.md): distinguish the upload, later validated local reference, specialists, and unpromoted work.
6. [Original objective](docs/original_goal.md): the user's full composition-first intuition, without reducing it to small animal substitutions or labor savings.

## Quick reproduction

Use Linux x86-64, a C++20 compiler, CMake and the `kaggriculture` conda environment. The original compiler was GCC 13.3.0. Python package versions are in [environment.yml](environment.yml); CPU execution is sufficient. These commands run from the repository root:

```bash
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py verify
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py build-submitted
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py evaluate-submitted
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py evaluate-submitted --native
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py verify-packed
```

The two C++ commands each run **4,096 matches**: 2,048 seeds, both seats. They check all comparable per-game outcome fields against the original frozen reports. The packed command runs 64 official-environment teammate games plus four PASS/self games and checks **48,892 actions** against C++. No command uploads to Kaggle. Generated binaries and results go to ignored `_work/`; frozen inputs remain unchanged.

The handoff reproduction results are recorded in [verification.json](verification.json). Rebuilding must produce the original three artifact hashes, not merely a similarly performing policy.

## Which agent is which

| Role | Package or artifact | Status |
|---|---|---|
| Exact upload | [submitted/submission.tar.gz](submitted/submission.tar.gz) | Uploaded and COMPLETE; unchanged bytes |
| Exact submitted C++ package | [agent.json](submitted/source_tree/experiments/v6/sep07_compositions_v0/runs/shop_herd_combinations_001/proposals/shop_herd_s6_m3_g1/agent.json) | Uses the separate, frozen `submitted/source_tree/` |
| Later broad local reference | [investment_context_guarded_001_best](workspace/experiments/v6/sep07_compositions_v0/candidates/investment_context_guarded_001_best/agent.json) | Validated after submission; not uploaded |
| Current crop WIP | [fertilization_004_0_closure](workspace/experiments/v6/sep07_compositions_v0/runs/fertilization_packages_001/proposals/fertilization_004_0_closure/agent.json) | Small one-opponent discovery gain; not promoted |
| Intermediate agents and league | [agent index](agents.json), [league catalog](workspace/experiments/v6/sep07_compositions_v0/configs/league.json) | C++ packages retain `agent.json`, `README.md`, and `source/agent.hpp/.cpp` |

```bash
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py build-agent investment_context_guarded_001_best
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py build-agent fertilization_004_0_closure --opponent public_router
```

## Contents and evidence

- `submitted/`: exact upload, exporter/adapter/verification sources, original teammate runtime, 58 frozen C++ dependencies, and official receipts/reports.
- `workspace/`: a frozen repository-shaped research workspace. It contains the complete experiment code, intermediate agents, planning/search modules, original ledgers, periodic review checkpoints, replay-import metadata, constraints and generated schedules. Required pre-existing public agents are included under `workspace/agents/external/`.
- `evidence/inventory.json`: every captured path, hash, size and storage location, plus an explicit list of omitted rebuildable executables/objects/caches. More than 9 GB of original text/data are represented by readable files and deduplicated gzip evidence blobs.
- `evidence/blobs/`: lossless compressed replay, fixture, evaluation and historical build inputs. [Reproduction instructions](docs/reproduction.md) explain how to restore them into the exact tree used by scripts.
- `docs/`: readable decision history, lineage, findings, limits and reproduction map. Chronological source ledgers are preserved even where later evidence corrects earlier conclusions.

The research compiler uses the repository's already tracked **V30 day solver**, through the relative `workspace/day_solver` link. Its exact files and repository revision are recorded in [day_solver_dependency.json](evidence/day_solver_dependency.json). The submitted agent itself does not require the day solver, network access, a GPU, or any live experiment source.

The main unfinished work is general composition construction and reliable dependency rebuilding across multiple investments and crop rotations. Profitable waiting was not established. The crop WIP must pass broader paired tests before promotion. Use the latest source [NEXT.md](workspace/experiments/v6/sep07_compositions_v0/NEXT.md), [objective coverage](workspace/experiments/v6/sep07_compositions_v0/OBJECTIVE_COVERAGE.md), and [crop audit](workspace/experiments/v6/sep07_compositions_v0/research/crop_conversion_audit_001/README.md) when resuming.
