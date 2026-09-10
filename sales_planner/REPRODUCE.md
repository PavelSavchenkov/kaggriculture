# Build and reproduce

Run commands from the repository root through the `kaggriculture` conda environment. The existing `fast_game_engine/`, `agents/common/` and root agent packages remain dependencies. No Git command is needed.

## Check this package

```bash
conda run --no-capture-output -n kaggriculture python sales_planner/scripts/verify.py --live
```

The script restores the three bundled exposed calendars, checks market arithmetic, split/resume, purchase-witness normalization and rival-delivery bounds, and compares replay results against the frozen session build. It then checks the retained live wrapper in generic and parallel typed-debug runners, self-play and zero-budget games, and runs the original-baseline improvement against PASS. Results go to `runs/package_check/`. Omit `--live` for the shorter component check.

The frozen replay reference predates five counters for later paused experiments. Verification requires those counters to remain zero and compares every original result field except elapsed time. Bundled cases include the carrot-to-feed funding cascade, the milk-to-unused-seeds cascade, and a later public confirmation plan. Their hashes and origins are recorded in data/MANIFEST.json.

Live checks also compare with the frozen binary using `--validate` on both sides. This enables failed-work counters; the original agent already has failed worker actions in these smoke games. The package preserves those counts and action hashes. Passing the check means matching the baseline and the action API, not eliminating every unsuccessful farm action.

The original large confirmation was not rerun for packaging. Its protocols, numerical reports and known losses remain in `evidence/runs/`. The original binary hashes describe the measured binaries; relocating and rebuilding source creates a new build identity.

## Restore more exposed data

```bash
conda run -n kaggriculture python sales_planner/scripts/prepare_data.py --episodes 107305772
conda run -n kaggriculture python sales_planner/scripts/prepare_data.py --all-exposed --download
```

Preparation uses bundled fixtures, matching local archive files, then local raw replays. `--download` permits missing replays to be fetched through the existing Kaggle CLI/account. `--no-archive` verifies that restoration does not depend on the experiment directory. Files go to ignored cache directories. Reserved episodes are refused.

Replay and calendar hashes must match. Extraction also checks the installed official engine source hash. A changed upstream replay or engine requires a new, explicit data-version audit; do not silently replace the recorded fixtures. The manifest keeps old calendar hashes; the latest corrected extraction is the default. Six initial `calendars_v0` files predate the itemless-PLACE parser fix.

## Run sale timing on supplied calendars

```bash
conda run -n kaggriculture python sales_planner/scripts/build.py sales_planner/source/replay_timing.cpp
```

The builder prints a cached binary path. Run it through conda with:

```text
<binary> --source-actions --delivery-bound --purchase-witness --multiple-sales sales_planner/data/cache/107342138.calendar
```

For a batch, create a new `sales_planner/runs/<name>/PROTOCOL.json` using `evidence/runs/multiple_sales_confirmation0425_v0/PROTOCOL.json` as the specification. Replace each case path with `data/cache/<episode>.calendar`, prepare those episodes, remove the historical `binary` and `binary_sha256` fields, and record the new protocol's date and purpose. These episodes are exposed replications now. Then run:

```text
conda run -n kaggriculture python sales_planner/scripts/run_replay_timing_batch.py <binary> sales_planner/runs/<name>
```

The runner compares multiple edits with the one-edit control and original orders, records the actual binary hash, and retains failed and changed cases. Supplied purchase quantities are part of this conditional test; it is not a purchase-prediction benchmark.

## Run complete course comparisons

```bash
conda run -n kaggriculture python sales_planner/scripts/build_arena.py --driver course_timing --agents atakan_demand ahmed_v25 nanare public_router_v52 sale_priority king_rc4 junghoon_wool_sales yusuke_sep08_m2 salem_sep08_m3 public_capacity_router titan_frontier ahmed_v24
```

Copy the specification from `evidence/runs/history_course_multiple_confirmation_v0/PROTOCOL.json` to a new run directory. For exact exposed seed replication retain its seed list; for a new confirmation choose unused seeds and freeze the protocol before outcomes. Run the printed binary with `scripts/run_course_timing.py <binary> <new-run-directory>`, then `scripts/summarize_course_timing.py <new-run-directory>` through conda.

Repeat with `multiple_sales: false` in a second protocol using identical seeds. `scripts/compare_course_timing.py <one-edit-run> <multiple-edit-run>` compares them; `scripts/audit_component.py <multiple-edit-run>` applies the numerical gates. Both actual paid-input repair controls and reacting rivals are evaluated in complete games. Report mean margin, own/rival cash, utility, tails, every failed/changed case, prediction gaps and full policy time.

`scripts/build_arena.py --agents history_course_multiple room_keep` builds a small generic integration runner. Its agent manifests reference root course sources directly. All generated registries and binaries remain under ignored `build/`.

## Older experiments

LEARNINGS.md records each tested idea and outcome. `evidence/RUN_INDEX.json` lists the archive runs and packaged reports; PROVENANCE.json hashes imported sources and evidence. The code for rejected and paused variants is retained so they can be isolated and rerun. Historical protocols and report contents retain their original paths and binary identities; they are evidence, not ready-made new-run manifests.

The raw archive remains at `experiments/v6/sep09_sales_planner/`. It is useful for inspecting an old trace or recomputing a large historical aggregate. Core builds, bundled checks and data retrieval work without it. Do not overwrite an archived protocol or silently substitute a new binary into an old result.
