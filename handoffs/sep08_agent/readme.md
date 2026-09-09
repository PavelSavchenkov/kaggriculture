# September 8 agent and optimization-loop handoff

This is the continuation package for the composition-search session, frozen on
September 8, 2026. Start here, not with a dated historical “best agent” claim.
All required new source and evidence is stored here or in the accompanying
agent packages. Identical content already committed in the September 7 handoff,
the C++ engine, and the day solver is referenced by path and SHA256.

## Which agent to use

| Role | Agent | Meaning |
| --- | --- | --- |
| Latest submitted policy | `animal_repair_q24_premium_m2` | Official IDs 56101451 and identical repeat 56102764, both COMPLETE. |
| Accepted research reference | `empty_sale_slots_m2` | Last policy that passed its declared promotion protocol. |
| Useful unfinished successor | `cow_service_retained_q24_premium_m2` | Saves labor in two cow courses; only discovery evidence, unchanged wins and choices. |
| Independent cold reference | `early_melon_b98_m1` | Locally constructed farm; still far weaker than the mainline. |
| Teammate benchmark | `teammate_shoprouter` | Retained C++ port of the teammate's shoprouter-rl-v2. |

The submitted policy is in
[`agents/external/animal_repair_q24_premium_m2`](../../agents/external/animal_repair_q24_premium_m2/README.md).
It won 4,040/4,096 native matches against the teammate and 3,883/4,096 against
the previous submission in the exact frozen C++ audit. Its broad win score rose
substantially, but its native margin-gain confidence interval crossed zero.
Submission does not overwrite that failed research gate.

The archive SHA256 is
`d65b8150db33becdedb4f752e8d773fe1c1b24306cb4459974104564d6bde374`.
The repeat server validation reproduced 1,438 actions and both final rewards.
No further Kaggle upload is part of reproduction.

## Read in this order

1. [Goal and operating rules](docs/goal.md): the user's actual ideas and constraints.
2. [Policy architecture](docs/policy.md): exactly what the submitted agent does.
3. [Development lineage](docs/lineage.md): parents, borrowed components, wins and failures.
4. [League and promotion](docs/league.md): opponents, weights, seed panels, gates and tradeoffs.
5. [Reproduction](docs/reproduction.md): build, verify, recover evidence, and resume safely.
6. [Optimization workflow](docs/workflow.md): proposal → estimate → compile → compare → repair.
7. [Next work](docs/next.md): concrete unfinished tasks and how to judge them.
8. [Verification results](docs/validation.md): exact package, agent and pipeline checks.
9. [Detailed strategy report](docs/strategy_report.md): ranked drivers, bottlenecks and 14 separable optimization contracts.

For depth, read [the full component ledger](docs/component_lineage.md),
[ideas ledger](docs/ideas_ledger.md), [profiling ledger](docs/profiling_ledger.md),
and [original objective](docs/original_objective.md). Historical documents retain
their original timestamps and verdicts; the table above is the snapshot status.

## Quick start

From the repository root, with the `kaggriculture` conda environment available:

```bash
conda run -n kaggriculture python handoffs/sep08_agent/reproduce.py verify
conda run -n kaggriculture python handoffs/sep08_agent/tools/build_catalog.py --agents animal_repair_q24_premium_m2 empty_sale_slots_m2 teammate_shoprouter
conda run -n kaggriculture python handoffs/sep08_agent/reproduce.py rebuild-submission
conda run -n kaggriculture python handoffs/sep08_agent/reproduce.py prepare
```

The builder prints an arena path. Run it through conda with `--a`, `--b`,
`--games`, `--seed-start`, `--seat-mode both`, `--threads`, `--validate`, and
`--output`. `--games` counts seeds, so both seats double the actual match count.
See reproduction.md for tested commands and output locations.

`prepare` creates an isolated, ignored `_work/session/` with the original
experiment layout. `--evidence` restores large full-match profiles; `--replays`
restores selected source/current-top replays. Full restoration is large because
it restores original JSON bytes, not because the compressed repository package
copies the engine or day solver. Do not edit the archived workspace in place.

## What is retained

- C++ sources, immutable generated programs, proposal generators, compilers,
  valuation code, inspectors, evaluators, analyzers and run protocols.
- Exact deployment source, exporter, templates, archive, operational checks,
  server validation records, and original large teammate/prior match results.
- Successful and failed day problems, schedules, certificates and funding cases.
- Full paired match profiles, complete run summaries, source freezes and guard
  investigations supporting the reported decisions.
- Source notebooks, provenance and selected raw replays; the exact inventory
  identifies unselected historical replay downloads omitted from this snapshot.
- 45 current comparison agents, including 39 newly cataloged packages. Historical
  opponents are retained deliberately; catalog storage is not a promotion claim.

`evidence/inventory.json` maps every recoverable path to exact bytes, storage
encoding and SHA256. `evidence/repository_dependencies.json` pins committed
dependencies. Build outputs and live process handles are not authoritative.

The main unresolved problem is a competitive compiler and estimator for unfamiliar
farms. The successful agent chooses among known executable families; it does not
yet realize the full general composition-first search proposed by the user.
