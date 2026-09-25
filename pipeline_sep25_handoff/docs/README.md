# Source documents

Copied as they were (Sep 25, ~20:05 BST). They cite paths of the original locations; map them with the
table below.

| Folder | Content |
|---|---|
| `design/day_intent.md`, `design/day_compiler.md` | the two contracts (repository `designs/`, not committed elsewhere) |
| `design/day_policy_SPEC.md` | route solver contract (repository `day_policy/SPEC.md`, working-tree version) |
| `design/network_compiler_design_sep21.txt` | the Sep 21 full network + compiler design (from `work/opus55_analysis_sep24/design.txt`) |
| `BC_issues_sep25.md` | self-contained write-up of the BC pipeline and its issues, written for outside advice (repository root) |
| `experiment_log/` | the experiment's own `README.md`, `LEARNINGS.md` (Sep 24 night), `PROGRESS.md` (audit log, ~every 30 min), `IDEAS_LEDGER.md` (ranked ideas with status, includes two external reviews), `HANDOFF_sep25*.md` (session handoffs, morning/afternoon/evening) |
| `weakness_analysis/` | the companion session's study of the same agents: `REPORT.md` (weak points, first probes X1-X13), `GAPS.md` (five gaps to gold level), `BOUNDARY.md` (which decisions belong to the network), `KAGGLE_REPLAYS.md` (our Kaggle games and all 14 losses), `OPPORTUNITY_night_inventory.md`, `VALUE_MODEL.md` (how to train a value model), `BUDGET_FIX.md`, `ROTATION_LOG.md` (5-hour log of probes), `CYCLES.md` (its next 24-hour plan: top-3 gaps every 3 hours, started 19:45) |

| Path in the documents | In this handoff |
|---|---|
| `experiments/v10/sep24_BC_opus/{source,agent,tools,scripts,day_policy_local,opponents,tests}` | `experiment/...` |
| `experiments/v10/sep24_BC_opus/models/<name>` | `weights/v12_cond` + `candidates/<name>` (LINEAGE.md section 6 maps names) |
| `experiments/v10/sep24_BC_opus/reports/<run>` | rows with `set` = `<run>` in `evidence/*.csv` |
| `experiments/v10/sep24_BC_opus/data/{corpus_v5.txt,conditions_v5.csv,styles.json,replay_opponents.txt}` | `experiment/data/` |
| `designs/*.md` | `docs/design/` |
| `work/sep25_bc_weakness/tools/*`, `q*.py`, `kaggle_ours/*.py` | `analysis/tools/`, `analysis/queries/`, `analysis/kaggle/` |
| `work/sep25_bc_weakness/exp_patch/` (`OPUS_*` probe flags) | not included: probes that won were ported into the experiment's compiler as `CompileOptions` (collect_all, forecast_blend, race_dp, forecast_prior, race_early, race_steps, melon_tie_now, route_search, sell_order 2, trim_model); the others lost |
| `submissions/sep25-bc-opus-v12-*` | Local-LB repository `agents/pavel-bc-opus-v12-*`; `agent/` here for `forecast` |
