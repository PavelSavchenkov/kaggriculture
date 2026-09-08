from pathlib import Path
from datetime import datetime, timezone
import csv
import json

EXP = Path(__file__).resolve().parents[2]
now = datetime.now(timezone.utc).isoformat()
old = json.loads((EXP / 'research/review_54_comparison.json').read_text())
global_data = json.loads((EXP / 'research/refresh_2002/invariant_means.json').read_text())['means']
trades = list(csv.DictReader((EXP / 'research/refresh_2002/transactions.csv').open()))
rows = []
for previous in old:
    metric = previous['metric']
    if metric.endswith('_weighted_hour'):
        operation = metric.removesuffix('_weighted_hour')
        selected = [row for row in trades if row['operation'] == operation]
        value = sum(float(row['hour']) * float(row['actual']) for row in selected) / sum(float(row['actual']) for row in selected)
    else:
        value = global_data[metric]
    rows.append({**previous, 'global72': value})
assert len(rows) == 82
text = f'''# Review55 — {now}

Due20:08UTC. The previous goal turn made concrete progress: exact day-solver
savings, complete policies, two broad rejected candidates, and a preserved
stronger reference. The original goal continues through September8,00:47UTC.
No Git or submission. Current reference remains opening_q32_b13_v1.

The general animal/wait selector is now implemented in C++. Four immutable
complete calendars share one per-instance parent controller. At day13, only
within the parent's wheat context and a matching physical guard, it compares
keeping crops, goose, cow and sheep. Values use actual compiled daily buys and
sales, seed/animal costs and achievable hire costs. The model samples32 future
shop paths conditional on observed shops. Each scenario uses its corresponding
day20 berry continuation; the actual policy waits for day20 observations to
make that branch. No future price, seed or hidden opponent inventory is used.

Equal physical day contracts permit reuse of on-cow day18 in the shared prefix,
saving another$144 even when the later off berry continuation is chosen. Source
contracts/hashes are recorded in runs/late_portfolio_001/DATA_LINEAGE.json.
All1119 distinct recorded HIRE orders have n=1; the model's fixed labor sum
matches that compiled representation. The root game executes one hire per
order regardless of quantity, so any future generalized import must preserve
that rule. Current data do not rely on quantity-sensitive hiring.

8192 complete counterfactual games give every investment's actual outcome on
the same observed prefix. Predictions agree across all four forced courses.
Median time for all32-scenario choices is.253ms against public_router and.337ms
against q32. For goose, predicted versus realized margin correlation is.90–.92
and mean absolute error$70–$88. Cow correlation is.85–.86, error$441–$449. Sheep
is less reliable: correlation.47–.56, error$451–$647. Future demand uncertainty
and market-model gaps still matter, especially wool. This is concrete support
for cheap ranking followed by exact execution and error feedback.

A grid of42 simple risk/threshold settings on previously used1790000 seeds
selected expected margin minus half a sampled standard deviation. It chooses
wait/goose/cow/sheep, with sheep only in a small subset against public_router.
2048 exact actual-policy games reproduce every selected counterfactual record.
Forced wait/goose match existing parent/context policies in256 full records.
896generic/pair/debug/thread/self/PASS checks pass, including native shops.

First unused1830000..1830511 panel:45056games,22opponents,two policies. Direct
q32 result380W566T78L,utility.64746094,mean+$155.40625. Every opponent's paired
mean margin improves. Current grouped utility rises.94199219→.94316406 but
its95%gain interval[-.00065918,.00295410] still crosses zero. All other
preregistered gates pass, including historical noninferiority. No promotion.
An independent2048-seed1850000 confirmation is running with the policy frozen,
same gates and no pooling or fitting on the first panel. Native/PASS audits and
a source freeze are also running. Do not describe an unresolved confidence
bound as proof of improvement.

Fresh20:02 top12 replay cohort is complete, and two changed notebooks are
downloaded without executing code. Fields of Fortune is byte-identical to the
previous pull despite new metadata. Thomas95.5% uses five719-turn schedules,
ten72-turn blocks and previous-route tests. Its actual referenced inputs are
tomato inventory, milk demand and wool demand. It is a distinct controller
worth porting and checking; the title's win rate remains an author claim.
Raw data and SHA are in research/refresh_2002/THOMAS_PAYLOAD.json.

Keep the full scope: dated composition search, cheap biology/economics/service,
placement, workforce/trade compilation, estimate-versus-execution feedback,
all animal species/wait, larger/cold proposals and replay borrowing, growing
league and unused audit scenarios. The current four-way selector realizes a
major piece of the original intuition, but one tile/entry date is not a general
cold-start composition search. Expand useful dated proposals after validation.

Below are all82 refreshed global72 versus unchanged promoted-q32 local64
comparisons. These are different cohorts, not paired performance evidence.
Next review20:28UTC; next public refresh around21:02UTC. Final900000 untouched.

| Metric | Global72 | q32 local64 |
| --- | ---: | ---: |
'''
for row in rows:
    text += f"| {row['metric']} | {row['global72']:.4f} | {row['local64']:.4f} |\n"
(EXP / 'research/review_55.md').write_text(text)
(EXP / 'research/review_55_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')
entry = f'\n{now}: Review55: four-way C++ investment selector built.8192counterfactuals,2048exact selector parity,896operational checks. Goose estimate correlation.90–.92, all choices~.3ms. First45056fresh panel improves every paired mean; only current-group positive95%bound unresolved. Frozen independent2048-seed confirmation/native audit running; q32 remains reference. Fresh2002 complete, distinct Thomas72-turn router extracted, Fields unchanged. Next20:28.\n'
for name in ['PROGRESS.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md']:
    with (EXP / name).open('a') as output:
        output.write(entry)
print('Review55 recorded', now)
