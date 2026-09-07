# Reproducing the submission and research pipeline

Run the commands below from the repository root. Every Python, compiler and package command uses `conda run -n kaggriculture`. Reproduction is CPU-only. The original platform is Linux x86-64 with GCC 13.3.0; the bundled V30 scheduler also targets Linux x86-64.

## Environment and immutable inputs

[environment.yml](../environment.yml) records the principal installed Python versions. If the environment does not exist, create it with `conda env create -f handoffs/sep07_agent/environment.yml`, then run subsequent commands through that environment. A system C++20 compiler is also required. Python 3.12 and GCC 13.3 were used for the recorded checks. The submission's runtime itself uses only Python's standard library.

```bash
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py verify
```

This checks readable and compressed snapshot files, all 58 exact submitted C++ source dependencies, original deployment hashes, and the final handoff manifest when present. Original historical metadata is preserved, including original-machine paths. Its binary hash records are evidence; native binaries are rebuilt rather than committed. The original teammate's small `agent.so` is retained because the official-runtime comparison uses that externally supplied entry point, in addition to the independent C++ port.

## Rebuild exactly what was uploaded

```bash
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py build-submitted
```

The helper creates `_work/submitted/`, compiles the frozen pair arena, policy exporter and action-parity checker, runs the original packer, and checks these byte-exact hashes:

| File | SHA256 |
|---|---|
| `main.py` | `f5f58e14b0cafd0f150a505d81e63feb9426c8415771d23bde054b839f6b8d97` |
| `submission.tar.gz` | `53acf9d1d5e3206e28ea6f365af39786a9e74c4b3bfe6851b5bd21e8740901f4` |
| `policy_data.json` | `7c8289788ca70ca941fc783ed36c49f45827518d1358a118971225b382e67107` |

The archive contains only `main.py`. Gzip and tar metadata are deterministic. Do not rebuild this submission using later `workspace/include` files: its own `submitted/source_tree/` is the authority.

```bash
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py evaluate-submitted
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py evaluate-submitted --native
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py build-submitted --debug
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py evaluate-submitted --debug --games 8
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py verify-packed
```

`--games` is the seed count; both seats double the number of games. Defaults reproduce seeds 1150000–1152047 and 4,096 matches for each shop mode. For a quick smoke use `--games 8`. The packed check defaults to 32 seeds and both seats plus four PASS/self games, using `kaggle-environments` and the original teammate entry point.

The large original reports did not collect failed-action diagnostics and therefore contain placeholder zero fault counts. The handoff rerun enables diagnostics. The comparer checks cash, action hashes, production, trades, discards, timing-independent outcome fields and all other comparable values, excluding only those newly populated fault fields when diagnostic modes differ. This is not a claim that the original agent makes zero unsuccessful requests.

The helper never submits externally. Current account credentials or Kaggle access are unnecessary for these local checks.

## Rebuild intermediate agents and current WIP

The [agent index](../agents.json) lists all preserved local-format packages. Friendly league aliases are in `workspace/experiments/v6/sep07_compositions_v0/configs/league.json`.

```bash
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py build-agent justin_guarded_hires_001_best
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py build-agent investment_context_guarded_001_best
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py build-agent fertilization_004_0_closure --opponent public_router
```

These create a disposable `_work/session/` mirror, build a concrete pair from the preserved manifest, and run 32 seeds/both seats by default. Use `--games`, `--seed`, `--threads`, `--opponent` and `--debug` to change the test. Each agent instance is reset per episode; only legal observations enter gameplay. The WIP command verifies executability, not promotion.

## Restore archived historical inputs into the usable workspace

```bash
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py prepare-session
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py prepare-session --all-evidence
```

The first command restores archived C++/Python source inputs, including historical `.inc` tables, into **the same `_work/session/` tree used by builds**. It rebases original absolute repository paths in copied scripts and saved build commands to that workspace. Frozen evidence is not edited. Repeating preparation preserves already rebased paths. `_work/relocation.json` records modified historical files.

`--all-evidence` additionally restores raw replays, packed fixtures, full game results and other archived data. Allow roughly 10 GB. This enables historical research scripts that need original replay JSON or full snapshot tables. These source scripts are historical tools, with their original stage-specific arguments and assumptions; preparation solves path relocation and missing archived inputs, but does not assert that rerunning every failed or superseded experiment is desirable.

For evidence inspection without creating the research workspace, use the separate generic extractor:

```bash
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py hydrate --prefix submitted/kaggle_validation/
```

Its default output is `_work/expanded/`, retaining the `submitted/` or `workspace/` prefix. Use `prepare-session`, not this generic extractor, before executing historical pipeline scripts.

## Rerun exact day compilation

The dependency manifest checks the repository's tracked V30 files before compiling. No untracked external experiment is required.

```bash
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py compile-days --target improve_care
conda run -n kaggriculture python handoffs/sep07_agent/reproduce.py compile-days --target improve_care 150 experiments/v6/sep07_compositions_v0/runs/reproduced_justin_day 1 3 hire
```

The second command extracts program150's successful services/trades, proposes removing a hire, rebuilds one complete day under a three-second solver budget, checks exact end state/cash and runs the resulting full agent against live opponents. The same path must not already exist. This end-to-end smoke reproduced a day16 reduction with matching endpoints/cash during handoff verification.

Solver search uses wall-time limits, so a rerun may choose a different valid route or return UNKNOWN on a different machine/load. The frozen schedules and contracts are included for exact policy reconstruction. A feasible rebuilt route is not a proof of global optimality; UNKNOWN is not proof that the composition is impossible.

## Pipeline map and reproduction checkpoints

All paths below are relative to `_work/session/experiments/v6/sep07_compositions_v0/` after preparation. Original commands, inputs, hashes and results remain in each run's metadata and ledgers.

| Stage | Code / inputs | Output and interpretation |
|---|---|---|
| Rules, dated farm occupancy and cheap production | `include/biology.hpp`, `src/biology_profile.cpp`, `src/search_compositions.cpp`, `src/cold_compiler.cpp` | Cohorts and service calendars; exact dated biology where checked, approximate routing/economics |
| Replay import and normalized executable courses | `scripts/`, `replays/`, `research/`, `league/top_replay_library/IMPORT.json` | 180 source courses with donor IDs/hashes and action parity; private donor branching is not recovered |
| Source day extraction | `include/day_contract.hpp`, `src/rebuild_days.cpp`, `src/improve_care.cpp` | Accepted field work, dated purchases/withdrawals, source inventory and tile contracts |
| Submitted worker schedules | `runs/justin_reduce_hires_001/`, `runs/justin_day_library_001/` | V30 inputs, complete routes and checked physical endpoints; 22 selected components |
| Submitted shop composition search | `src/search_shop_herd.cpp`, `include/shop_herd.hpp`, `runs/shop_herd_combinations_001/` | Two purchase edits, demand modes, fixed/relative controls, exact league evidence |
| Later general animal decisions | `src/compile_animal_entries.cpp`, `include/deferred_animal.hpp`, `include/animal_investment_value.hpp` | Goose/cow/sheep/wait at one reserved opportunity; compile-before-promotion and context limitations |
| Estimator errors and legal shop integration | `runs/atakan_oracle_ablation_001/`, `runs/atakan_sampled_shops_001/`, `include/sampled_animal_value.hpp` | Offline oracle diagnosis versus legal observation-only sampling; complementary specialist rather than incumbent |
| Current crop WIP | `src/improve_fertilization.cpp`, `include/guarded_sequence.hpp`, `runs/fertilization_004/`, `runs/fertilization_005/` | Additional productive fertilizer and route continuation, original sale timing preserved; broader gates pending |
| Exact league evaluation | `scripts/build_arena.py`, `scripts/evaluate_packages.py`, `include/evaluation.hpp`, `include/profile.hpp` | Common seeds and both seats, own/opponent cash, action hashes, output, trades, labor, faults and lower tails |
| Review and next proposals | `research/review_*.md`, `IDEAS_LEDGER.md`, `PROFILING_LEDGER.md`, `OBJECTIVE_COVERAGE.md`, `NEXT.md` | Positive and negative findings, global-player invariants, incomplete work and scope corrections |

The composition hypothesis earned useful fast valuation and compilation tools, but the full autonomous outer loop remains incomplete. Reproduction of a retained package is deterministic; reproducing an entire adaptive research session is not promised. The records preserve what was actually tried, including incorrect early estimates and the later evidence that corrected them.
