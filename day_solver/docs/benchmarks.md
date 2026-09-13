# Benchmarks and their limits

Current release1.2.0 paired results and self-contained evidence are documented
in [release_1_2_0.md](release_1_2_0.md). The measurements below describe earlier
releases and historical cohorts.

The packaged Crop Dusta inputs omit `start.shed_capacity`, which means unlimited
storage. Their availability withdrawals also abstract the caller's market orders.
Passing these contracts is not the same test as preserving actual timed trades
under the game's 100-item shed limit. Keep these historical regressions separate
from the full-game leader evaluations.

A September 13 pilot re-extracted 87 days from three existing Crop Dusta archive
games using the same bounded-capacity, canonical-order, full-game validator as
the leader tests. Frozen retained main passed 87/87 at both 4s and 12s limits;
the 4s run averaged 305ms, median 183ms, maximum 2.491s. Regret at 2s passed86/87,
averaging224ms. These games were analyzed historically; this is a comparable
exposed pilot, not a new large unseen validation. Evidence is in
`experiments/v7/sep12_composition_to_schedule/results/strict_v438_crop_full_contract_family/`.

The later complete bounded-capacity archive comparison covers1943ordinary days
from67 historically examined Crop games. At4s, retained main succeeds1922/1943
(98.9%), averaging307ms; the paired pre-repair build succeeds1921/1943,
averaging311ms. There is one gain and no paired losses. This is broader exposed
validation, not a new unused-replay test. Frozen sources, physical-input hashes
and full-game validation are recorded in experiment result v511.

On145ordinary days from one exposed replay per current leader, the same retained
main4 policy succeeds123/145 at original replay workers,143/145 at+1,139/145
at+2 and140/145 at+3. Mean times are1096/723/757/712ms, including failures.
The paired pre-repair counts are123/141/139/140, with no losses (v510).
These worker counts are independent calls, not a sequential4s policy.

## Historical coverage

| Version / cohort | Unique days | Strict | Hard days | Episodes |
| --- | ---: | ---: | ---: | ---: |
| Python / Gate9, unseen | 1,323 | 1,323 | 1,005 | 60 |
| V20 / Gate13, unseen | 1,399 | 1,399 | 1,055 | 60 |
| V25 / Gate14, unseen | 1,340 | 1,338 | 1,055, of which 1,053 passed | 60 |
| V30 / real development | 323 | 323 | Mixed development workload | Previously exposed |
| V30 / synthetic | 9 | 9 | Not applicable | Not applicable |
| V30 / previously slow | 242 | 242 | Selected for prior runtime | Previously exposed |

Gate13 extracted and converted all 1,740 complete days without failures; 341
duplicate inputs were excluded. All 60 episodes passed every retained day, and
33,576 hourly inventories matched the independent ledger.

Gate14 also extracted/converted all 1,740 complete days without failures; 400
duplicates were excluded. There were two 900-second UNKNOWN results, no invalid
returned schedules and no false infeasibility claims. Fifty-eight of 60 episodes
passed all retained days. Accepted schedules contributed 32,112 checked hours.
The known failures are `106126272_p0_d16` and `106139776_p1_d16`; V20 and frozen
Python also timed out on them. V30 has no new unseen result resolving this gap.

These cohorts use different frozen versions. The tables must not be read as
one final solver passing all of their union. “Not worse” means satisfying the
same fixed day requirements in 24 hours, not reproducing the original route,
using fewer moves, making more profit or improving the agent's strategy.

## Historical speed

| Version / cohort | Median | Mean | Maximum |
| --- | ---: | ---: | ---: |
| Python / Gate9 | 2.462 s | 4.938 s | 262.23 s |
| V20 / Gate13 | 329.892 ms | 544.763 ms | 35.826 s |
| V25 / Gate14 | 339.398 ms | 2.580 s | 900.075 s |
| V30 / real development | 243.076 ms | 422.772 ms | 6.672 s |
| V30 / previously slow | 1.135 s | 2.340 s | 32.142 s |

V25's mean includes failures. Gate13 p90 was 783.605 ms: 1,087/1,399 inputs
finished within 500 ms and 1,307 within one second. The recorded evaluation pass
took 391 seconds excluding collection and independent checking. V30 development
had 259/323 within 500 ms and 298/323 within one second.

These were shared-workstation runs on an Intel Core i7-14700K, Linux x86-64,
using GCC 13.3 and OR-Tools 9.15.6755. Broad evaluation normally ran two day
processes concurrently; quick CP-SAT trials used one thread and the native
fallback up to eight. Some development comparisons overlapped other diagnostics.
Do not turn cross-cohort medians into a causal speedup or a guaranteed latency.

Paired evidence is stronger for individual changes: direct materialization
reduced a matched comparison from 74.643 to 62.770 seconds (15.9% less);
partial neighborhood selection reduced a subsequent matched comparison from
65.812 to 64.299 seconds (2.3% less). Each comparison had 324 strict schedules
and 7,776 independently checked hours. Timings include complete solver work;
component-only gains are not substituted for whole-day gains.

## Repeat exposed benchmarks

The package includes 574 public v3 inputs in `benchmarks/cases/` and a local
path/hash catalog in `benchmarks/cases.json`. They contain no original routes
or original worker schedules. Available groups:

| Group | Count | Purpose |
| --- | ---: | --- |
| `smoke` | 12 | Three real API controls and nine synthetic/numeric controls |
| `quick` | 81 | Original quick development set, including nine synthetic inputs |
| `development` | 323 | All real development inputs |
| `synthetic` | 9 | Tomato, goose/egg, coop, late hire/land and large-count controls |
| `slow` | 242 | Previously slow inputs |

From this directory:

```bash
conda run -n kaggriculture python tools/benchmark.py work/smoke --group smoke
conda run -n kaggriculture python tools/benchmark.py work/dev --group development --jobs 2
conda run -n kaggriculture python tools/benchmark.py work/slow --group slow --jobs 2
```

The benchmark runner needs only Python's standard library and the bundled
executables. It uses a thread pool to manage independent native solver
subprocesses, so Python's GIL is not serializing their search. Every success
must pass the native auditor and independent 24-hour inventory ledger. It
records failures and timeouts, hashes the inputs and executable, preserves all
individual reports, and reports solver time separately from elapsed batch time.
Solver-time summaries exclude process startup and independent audit overhead.

The inventory checker uses the original tested ledger logic with a direct v3
task extractor. Its output matched all 12 saved packaging ledgers exactly.
The smoke set is mostly tiny inputs; its median is not representative of real
Crop Dusta days. These are regression reruns, never unseen validation.

## Evidence locations

- `evidence/crop_dusta_gate9_reserved/final_summary.json`
- `evidence/crop_dusta_gate13_reserved/final_summary.json`
- `evidence/crop_dusta_gate14_reserved/final_summary.json`
- `evidence/native_quick_portfolio_v30/development_summary.json`
- `evidence/native_quick_portfolio_v30/{full_v1,broader/results_v1,tail242_v1,kernel_ablation_v1}/`
- `evidence/verification/`: checks performed for this standalone package.

Historical reports retain their original absolute provenance paths and hashes.
Those paths are records, not package dependencies. Use the local case catalog
for reruns. No replay episodes were opened or new solver optimization attempted
during packaging.
