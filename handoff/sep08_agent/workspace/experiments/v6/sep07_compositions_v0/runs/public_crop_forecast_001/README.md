# Public current-crop forecast

The crop biology forecast is correct under its explicit service assumptions. It improves Atakan branch estimates modestly and does not improve the sheep expansion portfolio. Retain the isolated helper and diagnostic evidence; no new agent was promoted or registered.

`source/crop_forecast.hpp` reads only the opponent's currently public crop tiles: species, planting date, held yield, water status and fertilizer expiry. It assumes daily productive water and immediate harvest/sale access. Five controls vary known versus maintained fertilizer and harvest at first ripe, first cap or final productive age. Repeated crops have exactly four dated production events. No future planting, crop removal, animal expansion, private inventory, unknown shops or donor future observations enter this helper. An optional separate wheat-feed subtraction is explicitly an uncertain market-disposition control.

The independent C++ engine check covers25,791 current crop tiles from504 observations of72 globally selected top-player games. All128,955 dated forecasts across the five service assumptions match exact engine harvest output. The diagnostic supplies fertilizer and three colocated workers to isolate biology. It does not prove the opponent can fund, route, deposit or sell those outputs. `BIOLOGY_CHECK.json`, `BIOLOGY_LINEAGE.json` and the frozen engine headers preserve the check.

Against actual later replay harvests, current-crop total quantity error falls from268.48 units per observation for a zero forecast to77.90 with known fertilizer and52.28 with full fertilizer. Dated error falls to99.62 and69.57 respectively. Current visible lifecycles account for87.2% of later strawberry harvest and87.6% of melon, but only20.2% of wheat,5.7% of carrot and44.9% of tomato. Future planting is the major missing component for short crops. These are repeated snapshots, not504 independent games. Harvest output is counted from successful harvests; trading does not inflate production.

Both complete-course families preserve their frozen own buys, sales, labor and other fixed costs. Current animal projections, current-day consumption timing and market models remain source exact. Each context tests33 observation-only configurations: three future-shop integrations (old35%discount, full mean and64 stratified sequences), five crop assumptions, and separate wheat-feed controls. Both own-profit and relative-margin objectives are scored against actual complete fixed-course outcomes. All640 discovery and1,280 fresh reconstructed prefixes match saved public physical state and both cash balances exactly. Original model baselines also match exactly.

Discovery uses seeds1000..1031 in both seats against five strong opponents per family. Atakan sampled64 margin with full-fertilizer current crops wins229/320 versus222 without crops, adding$356.37 mean margin. Known-fertilizer crops win227. Sheep remains74/320 across these choices. Full-fertilizer and known-fertilizer sampled64 margin controls were recorded in `fresh_1660000/PREREGISTRATION.json` before unused outcomes were generated.

Fresh seeds1660000..1660063 in both seats produce3,200 exact fixed-course games and640 contexts per family:

| Family / forecast | Wins /640 | Mean margin | Mean branch regret |
| --- | ---: | ---: | ---: |
| Atakan, no crops |451|$6,190.57|$1,396.71|
| Atakan, known fertilizer |451|$6,199.53|$1,387.75|
| Atakan, full fertilizer |453|$6,296.35|$1,290.93|
| Sheep, no crops |181|−$9,854.90|$422.20|
| Sheep, known/full fertilizer |181|−$9,864.60|$431.89|

For the selected full-fertilizer Atakan control, the64-seed clustered bootstrap95% interval is−$159.99 to+$471.51 mean margin and−1.875 to+2.8125 percentage points of wins. The measured fresh gain is small and uncertain. Branch margin-delta error improves from$7,746.09 to$6,851.41. Sheep error worsens and its four discovery V5 wrong choices remain wrong. The optional wheat-feed controls were not preregistered for promotion: some add fresh wins while reducing mean margin. No native or deployment gate was run because no deployable policy was retained.

Local Atakan traces explain the remaining gaps. Full-fertilizer current crops predict30 future wheat versus223.24 actual net sold,154.73 strawberries versus250.33, and69 melons versus68.88. There are no visible carrots at the decision despite57.97 later net sold. The current-herd forecast also misses later eggs, wool and fertilizer. Production is not a sale schedule; wheat can be fed and private held output can be sold later. `local_quantity_residuals.csv` reports all nine products and dated errors across960 branch-contexts.

Seed1028 versusV5 remains instructive. Sampled shops already choose cow correctly before adding crops. Goose versus cow raises exact rival milk net revenue by$15,483; every crop control still predicts$7,482.14, with258 projected milk versus249 actual sales. Thus this large remaining error is not missing current crop output or missing milk quantity. Future shops and timing/price response remain material. Full-fertilizer crops improve the own melon delta from$2,700 to$1,218, near the exact$1,229, but predict only$29.13 of the exact$547 rival strawberry delta. All product effects are retained in `seed1028_product_attribution.csv`; no hidden rival logic is inferred.

A full three-branch valuation with64 shop sequences takes a median0.300ms for full-fertilizer crops (p95 0.326ms, max0.627ms over320 calls). The no-crop baseline median is0.312ms; this timing noise does not establish a speed advantage. Forecast biology adds little cost relative to the existing market calculation. No GPU was needed.

The next useful model work is to forecast dated replanting from observed crop rotations, separate output from sale/feed/held-stock disposition, and include animal fertilizer supply. Each needs explicit observation-only controls and new exact branch checks. This helper should not be inserted globally merely because its biology is correct.

`DONOR_INPUT_LINEAGE.json` records all raw replay and cohort hashes. `BRANCH_INPUT_LINEAGE.json`, each `build/*BUILD.json`, `fresh_1660000/COMMANDS.json` and `RESIDUAL_REPORT.json` preserve inputs, frozen C++ dependencies, commands and outcomes. Reproduce extraction with `donor_inputs.py`, `branch_inputs.py`; compile/run with `run_branches.py`; score with `analyze_donors.py`, `analyze_branches.py`, `analyze_residuals.py`. `fresh_audit.py` intentionally refuses to overwrite its first-run preregistration; the exact recorded commands can reproduce each output in a new directory. `freeze.py` rebuilds the single-crop check from copied engine/API headers. Run all Python and build commands via `conda run -n kaggriculture`.

Donor/global context and existing local families remain in this same experiment: `../../research/refresh_1342`, `../atakan_portfolio_001`, `../atakan_sampled_shops_001`, `../atakan_oracle_ablation_001`, and `../sheep_expansion_portfolio_001`. Their source packages remain unchanged. Atakan's donor is Atakan Aldemir; the separate sheep expansion donor is 自己找差距. This is a locally inferred public-state model, not recovered private donor code. No core header, catalog, official agent, handoff or submission was changed.
