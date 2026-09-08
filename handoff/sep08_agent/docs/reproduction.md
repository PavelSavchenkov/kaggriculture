# Reproduce the policy and continue the loop

Run commands from the repository root. Use the `kaggriculture` conda environment.
The recorded environment is in `evidence/environment.json`. The core requirements
are Linux x86-64, C++20/GCC, CMake, Python and NumPy. Kaggle adapters additionally
use the recorded kaggle-environments version. The committed day solver provides
its own documented runtime and vendor dependencies.

## Verify and recover the snapshot

```bash
conda run -n kaggriculture python handoff/sep08_agent/reproduce.py verify
conda run -n kaggriculture python handoff/sep08_agent/reproduce.py prepare
```

The second command restores the original experiment at:

```text
handoff/sep08_agent/_work/session/experiments/v6/sep07_compositions_v0/
```

Source and normal metadata restore by default. Large complete match profiles
and raw replay downloads are optional:

```bash
conda run -n kaggriculture python handoff/sep08_agent/reproduce.py prepare --evidence --replays
```

Every restored file is checked against its original SHA256 before writing.
The engine, solver and official catalog are repository dependencies, not copied
into the handoff. Large new files use lossless xz/gzip blobs; identical older
files reference the committed September7 archive. The inventory records each
choice. Full optional restoration expands tens of gigabytes of original JSON.
Use the default restoration unless you need full historical profiles or replay
reanalysis. No file in the live uncommitted experiment is needed.

Historical source freezes, command logs and process IDs remain unchanged evidence.
Absolute binary paths in those logs are not live executables. Rebuild the binary
and use the path printed by its builder. Almost all original scripts derive
paths from `__file__`; one historical fresh_courses_2202 helper has its original
absolute path and should be adapted in a new run if reused.

## Rebuild the exact submitted archive

```bash
conda run -n kaggriculture python handoff/sep08_agent/reproduce.py rebuild-submission --destination handoff/sep08_agent/_work/deployment
```

The result is under
`_work/deployment/submissions/sep8-composition-adaptive-v1-repeat1/`.
`rebuild.py` verifies every frozen source, rebuilds the C++ exporter, regenerates
the tables and Python adapter, then requires the exact original artifact hashes.
The tar contains only `main.py`. Expected archive SHA256:

```text
d65b8150db33becdedb4f752e8d773fe1c1b24306cb4459974104564d6bde374
```

`SUBMISSION.json`, `IDENTITY.json`, the packed/file-runner/branch validations and
server validation reports preserve what was actually tested and submitted.
Rebuilding never calls Kaggle submission. Do not run the retained `submit_once.py`
without a separate explicit request.

## Build and run C++ agents

```bash
conda run -n kaggriculture python handoff/sep08_agent/tools/build_catalog.py --agents animal_repair_q24_premium_m2 empty_sale_slots_m2 teammate_shoprouter king_rc4
conda run -n kaggriculture python handoff/sep08_agent/tools/build_catalog.py --pair animal_repair_q24_premium_m2 teammate_shoprouter
conda run -n kaggriculture python handoff/sep08_agent/tools/build_catalog.py --agents animal_repair_q24_premium_m2 teammate_shoprouter --debug
```

Each command prints an arena path. Invoke it through conda, for example:

```text
conda run -n kaggriculture <printed-arena-path> --a animal_repair_q24_premium_m2 --b teammate_shoprouter --games 2048 --seed-start 2370000 --seat-mode both --threads 4 --budget-expansions 100000 --native-shops --validate --output work/sep08_teammate_repeat.json
```

That repeats the exposed 4,096-game submission audit; it is a reproduction,
not a fresh strength test. The expected result is4,040 wins and56 losses.
Remove `--native-shops` for the independent common shop stream. Add `--profile`
to collect detailed production, labor, market and failed-action accounting.

The generic arena also supports `--typed-pass` when the opponent is PASS. This
calls a concrete C++ agent/PASS pair for each catalog type. The handoff checks
all45 agents against generic mode, separate instances and one/multiple threads.
`evidence/CATALOG_VALIDATION.json` gives the actual counts and parity fields.
Additional debug, typed teammate and relocated checks are in the companion
validation reports. Action hashes and rewards are compared, not just win counts.

For a new experimental agent, keep the local C++ package format and pass a JSON
registry mapping its name to an absolute package path or repository-relative
path with `--registry`. The builder includes transitive compiler dependencies
in the cache key for custom registries. Do not add every trial to `agents/`.

## Run a new paired screen

```bash
conda run -n kaggriculture python handoff/sep08_agent/tools/run_comparison.py --candidate cow_service_retained_q24_premium_m2 --parent animal_repair_q24_premium_m2 --seed-start 2600000 --seeds 32 --output work/sep08_cow_screen
```

Choose and check your own unused seed block before claiming fresh confirmation;
2600000 is an example, not a certification that another teammate has not used it.
The script freezes its discovery protocol before matches, builds the requested
catalog, uses both seats and separate independent/native panels, and reports
per-opponent means, tails and paired seed-cluster intervals. Native seeds start
10,000 after the supplied block. It never promotes or submits a policy.

Use the same `--registry` mechanism for a new candidate. For broad promotion,
also run active-branch fixtures, guard diagnostics and the declared operational
checks. This convenient screen does not replace those checks or the frozen
confirmation protocol in league.md.

## Rebuild the animal compiler and audit witnesses

After `prepare`, use these paths:

```bash
conda run -n kaggriculture cmake -S handoff/sep08_agent/_work/session/experiments/v6/sep07_compositions_v0/runs/animal_group_policy_sep08_001 -B handoff/sep08_agent/_work/session/experiments/v6/sep07_compositions_v0/runs/animal_group_policy_sep08_001/build
conda run -n kaggriculture cmake --build handoff/sep08_agent/_work/session/experiments/v6/sep07_compositions_v0/runs/animal_group_policy_sep08_001/build --target check_policy audit_public compile_flexible -j 4
```

Run `check_policy` with the restored `LIBRARY_FIXTURES.txt`. It replays the12
construction fixtures and checks library actions, cash and guards against the
independent source courses. `compile_flexible` accepts the dated entry spec,
source seed, output directory, solver budget and optional certified prefix;
the exact CLI is recorded in `mixed_funding_sep08_001/RESUME_PROTOCOL.json` and
implemented at the end of its source file. Use `day_solver/with_runtime.sh`
when launching solver-linked executables.

The mixed course is already fully compiled. Run the portable checker to create
fresh audit outputs instead of trying to overwrite the archived `audit/` folder:

```bash
conda run -n kaggriculture python handoff/sep08_agent/tools/check_compiler.py
```

It builds the required targets, replays all12 construction fixtures, checks all30
mixed-course endpoints and719 transitions, and compares the corrected forecast
CSV byte for byte. The mixed result must remain -4923 own cash, -4956 rival cash
and +33 margin relative to the parent. The original `audit.py` deliberately
fails when its output directory already exists; retain that evidence and use a
fresh destination for repeated checks.

## Reproduce analysis or start new search

For full broad analysis, restore `--evidence` (optionally with `--prefix experiments/v6/sep07_compositions_v0/runs/animal_repair_sep08_001/`), then run the restored
`runs/animal_repair_sep08_001/analyze_broad.py`. It validates the source freeze,
reads all159 paired matchup files and diagnostics, and recreates population
intervals and per-opponent metrics. The later `finish_broad_review.py` records
the completed guard classification and final failed numeric gates.

For new work, copy/adapt a relevant run inside the restored experiment or create
a new self-contained experiment with the needed source closure. Preserve the
original protocols and results. Many historical generators use
`mkdir(exist_ok=False)` intentionally: they are recipes for a fresh run, not
commands to overwrite completed evidence. Read workflow.md and next.md before
spending a large search budget.
