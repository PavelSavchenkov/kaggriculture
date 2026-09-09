# Preserve verified schedules between workforce searches

The defect was in this experiment's compiler. Each run searched workforce sizes
again, without retaining schedules verified in earlier runs. A 30-second timeout
could therefore make the new run choose a more expensive workforce.

Root `day_solver/include/day_solver/scheduler.hpp` documents an empty result as
UNKNOWN. It receives a fixed workforce, accepts no incumbent, and does not compare
hire costs across calls. Its quick_v30 backend retains an accepted result within
a call. This evidence does not identify a root solver bug. No root solver files
were changed and no Git command was used.

In context1019, the repeated search failed to rediscover day25 with no extra hire
and day29 with one extra hire. Those schedules existed in the care-only run. The
new run saved$178 on days22/28 but added$322 on days25/29, giving a net loss$144.
Keeping the verified cheaper days instead gives a net gain$178. Context1014 also
gains$178. Both farms' production and the rival's final cash remain unchanged.
These are two fixed scenarios, not a league improvement claim.

The compiler now accepts an optional incumbent course:

```text
care_minimize OUTPUT SEED SPEC SECONDS RESUME_OR_DASH INCUMBENT_COURSE
```

For each workforce size it checks a saved day's exact market orders, reconstructs
the solver's physical purchase contract, strictly replays all worker requirements,
and separately replays the original executable orders against the live opponent.
It accepts the saved schedule only if the requested complete endpoint is reached.
This validation happens before further search at that workforce size. Zero search
time is permitted when an incumbent directory is supplied.

`check_incumbents.py` rebuilds both complete courses with zero new search time,
retains all15 days in each, and independently reproduces every full-game result
field. Evidence: `incumbent_zero_budget_v2/RESULTS.json`. Hire cost remains4866
instead of5044. The first failed integration check is retained in
`incumbent_zero_budget/`: executable sales must not be passed unchanged to the
solver's physical replay, which represents them as inventory deadlines.

`check_invalid_incumbent.py` supplies matching orders but replaces the saved
worker actions with PASS. The caller must reject that schedule. The fix never
treats a previous certificate as proof for a changed current problem.

Build through the conda environment:

```bash
conda run -n kaggriculture cmake -S experiments/v6/sep07_compositions_v0/runs/animal_service_cost_sep08_001 -B experiments/v6/sep07_compositions_v0/runs/animal_service_cost_sep08_001/build
conda run -n kaggriculture cmake --build experiments/v6/sep07_compositions_v0/runs/animal_service_cost_sep08_001/build --target care_minimize -j2
conda run -n kaggriculture python experiments/v6/sep07_compositions_v0/runs/animal_service_cost_sep08_001/check_incumbents.py
conda run -n kaggriculture python experiments/v6/sep07_compositions_v0/runs/animal_service_cost_sep08_001/check_invalid_incumbent.py
```

The checks intentionally require fresh output directories; preserve old results
before rerunning. `source/compile_before_incumbents.cpp` preserves the pre-fix
compiler. This change does not alter the already frozen submission candidate.
