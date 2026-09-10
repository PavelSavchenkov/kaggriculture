# Kaggriculture

Build, test, and improve C++ agents for the Kaggriculture farming game.

## Folders

| Folder | Purpose |
| --- | --- |
| `prompts/` | Written rules for development in this repo, usually referenced by `AGENTS.md`. |
| `experiments/` | Local, uncommitted work in progress, usually one folder per idea. |
| `agents/` | Saved agents: `inhouse/` for local strategies, `external/` for outside strategies, `common/` for shared code. |
| `fast_game_engine/` | Fast C++ game simulator for testing agents. |
| `day_solver/` | Builds worker schedules for one day. |
| `fast_day_solver_estimator/` | Quickly estimates daily labor cost and worker counts. |
| `handoffs/` | Full development pipelines behind some submitted agents. Written for AI, not humans; give these to your AI. |

## Development

Creating a Conda environment named `kaggriculture` is highly recommended.

1. Start a Codex session in this repo. Give it a goal and a workspace: `experiments/v[x]/[date]_[name]/`, for example `experiments/v6/sep10_better_hiring/`.
2. Have it follow [AGENTS.md](AGENTS.md). This already includes following the relevant `prompts/` rules, including [the experiment pipeline](prompts/experiments_pipeline.md).
3. Monitor thinking logs and test results. Guide the AI as it works.

Experiments are self-contained except for using Git-committed artifacts.

## Continue component work

Current components: [day_solver/](day_solver/README.md) and [fast_day_solver_estimator/](fast_day_solver_estimator/README.md).

Point your AI to the component folder and ask it to start an improvement experiment under `experiments/`. It should first read the component's problem scope, inputs, outputs, objective, and all past learnings, then build on that work.

After meaningful gains, ask it to update the corresponding root component folder with the improved code, new learnings, and updated pipeline for that problem.
