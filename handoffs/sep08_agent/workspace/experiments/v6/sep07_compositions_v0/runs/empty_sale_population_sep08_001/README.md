# Unguarded versus guarded sale ordering

Compare unchanged `empty_sale_slots_m2` with `empty_sale_floor_m1` on new fixed
seeds. The old unguarded audit failed its per-game no-regression rule on two $6
cases. Its aggregate and direct evidence was stronger than the guard's, so this
new study tests population strength without relabeling the old failed result.

Read PREREGISTERED.json before results. It fixes 71,680 fresh games on 35
opponents and 6,144 native/PASS games. Primary groups give equal weight to the
teammate, public controllers, replay reconstructions, and the last submission
plus nine accepted descendants. Extra experimental siblings remain reported.
Selection requires positive mean-margin confidence, win-score noninferiority,
individual mean-margin and tail conditions, and direct superiority to the guard.

Run all commands through `conda run -n kaggriculture`:

```
python experiments/v6/sep07_compositions_v0/runs/empty_sale_population_sep08_001/prepare.py
python experiments/v6/sep07_compositions_v0/runs/empty_sale_population_sep08_001/run.py fresh
python experiments/v6/sep07_compositions_v0/runs/empty_sale_population_sep08_001/run.py native
python experiments/v6/sep07_compositions_v0/runs/empty_sale_population_sep08_001/analyze.py fresh
python experiments/v6/sep07_compositions_v0/runs/empty_sale_population_sep08_001/analyze.py native
```

Preparation and runners refuse to overwrite previous protocol/results. Both
panels reuse the hashed broad arena from the completed guard study. Both policies
already passed 1,024 operational and 64 isolated rebuilt games; their unchanged
source hashes and exact evidence paths are recorded for reuse. No automatic
promotion, new submission, or official catalog change occurs here.
