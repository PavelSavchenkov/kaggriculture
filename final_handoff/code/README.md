# code/

Snapshots of the code behind the final agents and their evaluation. The agents' runtime source (network inference, decode,
day compiler, bridge) is in `../agents/kaggle_*/source/`. Scripts were copied from working folders, and many still contain
the absolute paths of those folders (`experiments/v10/...`, `work/...`). Adjust them before running.

| Folder | Contents | Origin |
|---|---|---|
| `training_network/` | `train.py` (DayIntent BC training, latest), corpus builders (`build_corpus_v5.py`, `build_fresh_corpus.py`), strength labels (`build_conditions.py`), export to the C++ format (`export_step.py`), Local-LB play / projection helpers | `experiments/v10/sep24_BC_opus/scripts`, `sep29_bc_mm/scripts` |
| `training_forecaster/` | `fc_tf_intra.py` (trainer: dawn transformer + intra-day head), `fc_tf_export.py`, dataset builders, offline evaluation, the exact seed recipes `train_fc2seeds.sh` / `train_fc3nvseeds.sh` | `work/sep26_wide_losses`, `experiments/v10/sep29_bc_mm/fc` |
| `day_compiler/patches/` | dc12 patches m1–m23 on the dc11 tree (funding, saleslots, nighttrim, survivalfloor, hirecheck, splitfert / nearanimals, dawnshare, ...) | `experiments/v10/sep29_dc12` |
| `day_compiler/docs/` | `KEYS.md` (every key), `DESIGN.md`, `REWORK.md`, `AUDIT.md`, `PROGRESS.md` | same |
| `bed_engine_src_m14or7ds/` | C++ tree of the test-bed binaries: `duel_mm` (G3 / opponent-replay / hybrid beds via `DUEL_OPP_REC`, `DUEL_OPP_SELL`), `full_games_dc11`, `teacher_day` (one-day / multi-day continuations). CMake finds the repo root three levels up, so it builds in place here | `work/sep29_fund/src_m14or7ds` |
| `evaluation/beds/` | Runners: `hybrid_bed_run.sh`, `opponent_replay_run.sh`, `g3_fixed_opponent_run.sh`, `frozen_lb_replay_run.sh`, `mkbridge.sh` (judge-ready agent folder with a rebuilt bridge), world lists | `experiments/v10/sep29_mm_copy` |
| `evaluation/imitation_scripts/` | `paired_read.py` (paired margin / own / opp), `h2h_wdl.py`, `day_split.py` (per day-window split), `flow_read.py` (hourly product flows), others | same |
| `evaluation/exact_lb_judge/` | Weaknesses' exact Local-LB judge: `lbgame.py` (one game with the LB's own `load_agent` / `run_single_game`), `lbrun.sh`, `lbsum.py` (seed-clustered paired reads), `gatecheck.py`, `lbproject.py` (LB Bradley-Terry projection), `mkagent.sh` | `work/sep29_validation/lb` |
| `evaluation/live_continuations/` | Day compiler's split tools (`lbsplit.py`, `span_pair.py`) for live-game continuations | `experiments/v10/sep29_dc12/tools` |
| `runq_gate/` | The team compute gate (`slot.sh`, `status.sh`), sequential stopping (`seqtest.py`, `seqwatch.sh`), its README | `work/runq` |
| `submission/` | `build_submission.py`, `verify_submission.py`, the Kaggle kernel check, `audit_kaggle_validation.py` | `submissions/sep29d-*`, `sep30-*` |
