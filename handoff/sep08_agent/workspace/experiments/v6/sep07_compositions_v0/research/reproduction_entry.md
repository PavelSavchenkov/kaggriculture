# Reproduce and continue the composition search

Start with `../CURRENT_REFERENCE.json`. It identifies the accepted agent and its
validation report. The last Kaggle submission and the committed catalog agent
are separately recorded there; they are not automatically updated by local
promotions. `post_submission_lineage.md` records the accepted mainline and the
separate composition-search branches; read its snapshot timestamp.

Paths below are relative to the experiment unless a full repository path is
shown. All commands run from the repository root through
`conda run -n kaggriculture`. Do not use Git or upload a submission without a new
explicit request. All work below stays within this experiment. The repository
engine, agent API and day solver are persistent dependencies.

## Build and evaluate an accepted agent

Read the `name` in `CURRENT_REFERENCE.json`, then use it with
`experiments/v6/sep07_compositions_v0/scripts/build_arena.py --agents <name> teammate_shoprouter public_router_v52`.
The command prints the resulting arena path. Run that arena with:

```
--a <name> --b teammate_shoprouter --games 512 --seed-start <chosen-seed>
--seat-mode both --threads 6 --validate --output <new-result-path>
```

The command produces 1,024 complete games: `--games` counts seeds and both
seats double the count. Add `--native-shops` for the official engine RNG stream;
the default uses an independent shop stream to make paired strategy comparisons
stable when policies change weed RNG consumption. Add `--profile` when measuring
production, inputs, timing, labor, failed actions, discards and rival effects.
Do not claim a new audit is unused merely because it has a new output filename.

For the exact built version, use its report's `frozen/FROZEN.json`. This records
the compiler, flags, actual transitive C++ files and rebuild command. The frozen
pair rebuild is tested against complete generic-runner records. Files under
`frozen/` are verification snapshots, not new authoritative engine copies.

## The most successful composition loop

`runs/late_portfolio_001/README.md` is the detailed entry point. It joins the
user's estimation and execution ideas in a working four-way choice:

1. Define dated crop-retention, goose, cow and sheep alternatives for an eligible
   day-13 tile. Preserve existing tomato and strawberry branches.
2. Compile their full daily schedules and supply/trade plans. The original short
   solver budget overstated labor cost; longer searches removed most extra hires.
3. Estimate remaining own farm economics and the public rival herd's effect on
   shared prices over 32 possible future shop sequences, conditional on shops
   already observed. Include actual compiled labor costs.
4. Play every alternative from identical observed prefixes. Compare estimates
   with full-game margins and identify production, timing and funding errors.
5. Compare selectors cheaply using these counterfactual outcomes, then verify
   that the selected C++ agent reproduces the chosen complete game records.
6. Freeze the selected policy before new seeds; test a growing league, native
   games, required operational checks and an isolated rebuild before promotion.

The run contains preparation/build/probe/analysis scripts and source hashes;
`results/late_portfolio_validation.json` points to its independent confirmation.
Generators fail on existing outputs. Use a new run for a new experiment and
record its parameters, inputs and commands; preserve the original evidence.

## Other composition and execution components

| Component | Code and evidence | Practical limit |
| --- | --- | --- |
| Dated crop/animal biology | `include/biology.hpp`, `results/count_spans_001/validation.json` | Service assumptions must match the proposed lifecycle. |
| Fast farm economics and labor estimates | `include/estimate.hpp`, `include/economics.hpp` | Approximate labor, liquidity and trade timing can mis-rank unfamiliar farms. |
| Independent warm/cold outer search | `src/search_compositions.cpp`, `runs/search_v0_001/summary.json` | The original loop uses a fixed public-router forecast and a weak greedy compiler; its negative result is retained. |
| Greedy physical construction | `candidates/composition_greedy_v0/source/agent.cpp` | Legal execution does not ensure the intended farm is realized competitively. |
| Full daily schedules | `scheduler/README.md`, `scripts/format_day_library.py`, persistent `day_solver/` | Rebuild all affected days and downstream stock/physical dependencies after a composition change. |
| Productive wheat/tomato/berry branches | `results/crop_mix_validation.json` and linked runs | Successful fixed continuations, not arbitrary placement search. |
| Whole sheep-farm transfer | `runs/v52_family_001/DAY_LIBRARY.json`, `results/wool_family_context_v2_validation.json` | Exact entry stocks and daily guards matter; rival funding changes realized farms. |
| Public rival-dependent selection | `results/rival_wool_context_v3_validation.json` | Public residuals do not uniquely identify hidden policies. |
| Sale timing | `runs/observed_sale_lead_003/`, `runs/observed_sale_lead_004/`, `runs/observed_sale_validation_002/` | Read the current validation status before treating a candidate as accepted. |

The biggest unfinished research task is a competitive compiler for unfamiliar
compositions and layouts, together with a calibrated labor/finance estimator.
The tested four-way selector supports the cheap-estimation intuition within
known executable alternatives; it does not establish accurate valuation of
arbitrary farms before routing. Keep cold starts, larger herd changes and new
placement alternatives in future work rather than only tuning existing gates.

## Learn from external agents without losing provenance

Use `scripts/refresh_top_evidence.py <new-refresh-name> --previous <old-name>` to
download the current top 12 and six recent player-games each, plus public
notebook metadata. Retain exact replay episode/seat, notebook version, source
hashes and licenses in each component's IMPORT or LINEAGE file. A replay course
is observed behavior, not proof of the player's complete controller.

The latest completed refresh at the September8 11:00 checkpoint is
`research/refresh_sep08_1008/`; later numbered reviews identify newer cohorts.
Its notebook audit records the new sources and why they were or were not ported.
Ahmed V23 has an exact C++ port and layer ablations. Its earlier-sale component
helped, while its dead-stock sales hurt on the tested panel. Borrow components
only after their actual contribution is measured.

For every candidate, keep rejected branches and causal witnesses. The sale-lead
failure at seed 1044 is a useful example: taking $63 of early rival wool revenue
prevented one sheep purchase, accidentally leaving a better-funded rival farm.
Identical requested actions did not imply identical successful purchases or
production. `runs/observed_sale_lead_003/JUN_FINANCING_CAUSE.json` preserves the
trace and the subsequent global start-time experiment.

`IDEAS_LEDGER.md`, `PROFILING_LEDGER.md`, `LINEAGE.md`, `OBJECTIVE_COVERAGE.md`
and the numbered reviews retain the broader process. Each review compares
82 global/local metrics; those are different cohorts, not head-to-head results.

## September8 broader animal composition study

`runs/animal_groups_sep08_001/MATCHED_RESULTS.md` joins the corrected281256-proposal
screen with six complete fixed-world cow/sheep continuations on the strongest
parent. It records actual labor, forecast residuals, group effects and the
source-control failures that exposed impossible timing and stochastic-weed
targets. Current sources compile directly; historical preparation scripts
document each transformation and require a clean copy for regeneration.
This is progress toward general composition compilation, not a new accepted
runtime policy. Preserve future parent branches and market adaptation before
building a shop-dependent selector and running fresh league gates.

`runs/salem_port_sep08_001/README.md` covers the new public C++ port and its
negative3968-game component screen, source parity, operational checks and
retained crop calendars. A weak complete public agent may still supply useful
components; measured component gains do not establish global competitiveness.
