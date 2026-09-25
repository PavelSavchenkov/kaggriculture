# Source snapshot of experiments/v10/sep24_BC_opus

Taken Sep 25 18:55 UTC (19:55 BST). Everything needed to build and run the agent, the tools and the scripts;
design and API: `../DESIGN.md`, `../ARCHITECTURE.md`. The original experiment README and logs are in
`../docs/experiment_log/`.

Changes against the original: `CMakeLists.txt` finds the repository root by searching upwards (the
original assumed 3 levels), so this folder builds in place; nothing else. The Python scripts still
assume the original depth (`ROOT.parents[2]` = repository root) and sibling folders `models/`,
`data/`, `reports/`, `build*/`: use `../setup_experiment.sh experiments/<version>/<date>_<name>` to
get a working copy.

```bash
conda run -n kaggriculture cmake -S . -B build -DCMAKE_BUILD_TYPE=Release   # BC_PORTABLE=ON for packages
conda run -n kaggriculture cmake --build build -j 8
conda run -n kaggriculture ctest --test-dir build
```

Dependencies (committed in the repository): `fast_game_engine/`, `agents/common/`,
`agents/external/{king_rc4,teammate_shoprouter,investment_context_guarded_001_best,arlene_v4_m31,ahmed_v25}`,
`agents/inhouse/{two_random_shop_league_v179,one_shop_no_geese_league_winner_v1}`, `agent_sep23/`.
The build default model path (`models/selected/model.bin`) is unused: every tool and script sets
`BC_OPUS_MODEL`.

`data/` holds only the small files: `corpus_v5.txt` and `conditions_v5.csv` (the v12 training set),
`styles.json`, `replay_opponents.txt` (frozen top-10 bed), `COMPILER_SOURCE.sha256` (provenance of the
compiler copy from `experiments/v10/sep24_day_compiler_opus`, not committed).
