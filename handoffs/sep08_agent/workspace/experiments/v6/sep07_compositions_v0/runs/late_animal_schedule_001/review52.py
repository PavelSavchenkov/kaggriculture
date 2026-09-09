from pathlib import Path
from datetime import datetime, timezone
import csv
import json

EXP=Path(__file__).resolve().parents[2]
now=datetime.now(timezone.utc).isoformat()
previous=json.loads((EXP/'research/review_51_comparison.json').read_text())
global_data=json.loads((EXP/'research/refresh_1902/invariant_means.json').read_text())['means']
trades=list(csv.DictReader((EXP/'research/refresh_1902/transactions.csv').open()))
rows=[]
for old in previous:
    name=old['metric']
    if name.endswith('_weighted_hour'):
        operation=name.removesuffix('_weighted_hour')
        selected=[row for row in trades if row['operation']==operation]
        global_value=sum(float(row['hour'])*float(row['actual']) for row in selected)/sum(float(row['actual']) for row in selected)
    else:
        global_value=global_data[name]
    rows.append({**old,'global72':global_value})
assert len(rows)==82
text=f'''# Review 52 — {now}

Due 19:08 UTC. The original 24-hour goal remains active through September8,
00:47 UTC. The previous goal turn answered what the added hires meant; this
turn carries out the user's request to optimize those schedules with our solver.
No Git, catalog update or Kaggle submission is authorized.

The initial 3-second search was too short to treat its extra hires as necessary.
With a 30-second budget and the original worker count, the solver found routes
for six of nine affected days in the off continuation and seven of eight in the
on continuation. Full-game audits match all 64 original records before the
change. In each continuation all64 candidate games preserve production, sale
quantities and rival action hashes. In the46 activated games, the off course
saves6 hires/$665, and the on course saves7 hires/$754. Every cash gain equals
the hire-cost reduction. Mean margin against the unchanged-crop compiler
control changes to -$68.90625 off and +$58.390625 on across64 games. These are
discovery results, not a promoted new agent or a fresh broad-league claim.

This changes the earlier conclusion: most of the added labor was a limitation
of the first search budget. Do not charge a composition the cheapest labor cost
found so far as though it were unavoidable. Use it as an achievable upper cost;
spend more exact effort when avoiding a $55/$89/$144 hire can change the choice.
A failed time-limited search remains UNKNOWN. Estimated biological outputs
were correct; execution quality materially changed the investment's economics.

Remaining off days22/23 were unresolved within30seconds. The final day requires
a separate integration fix. Its day-solver contract nets same-hour product
buys and sales, while the executable market action preserves both. Replaying
that combined action inside the physical day solver double-counts the raw buy
because actual sales belong to the caller, so the retry tool's second replay
is not a valid final-day check. Preserve the solver's certified physical schedule,
restore actual market orders, and use the full game for economics/stock checks.
The independent diagnostic also removes six final-day care actions that cannot
change production; their actual full-game benefit is not validated yet.

The larger crop-to-animal compiler has completed goose, cow and sheep variants
plus an unchanged-crop control for both later berry continuations. Each is a
complete17-day course including a checked real final turn. Cow/sheep initial
routes are weaker and their extra-hire budgets also need interpretation; do not
repeat the assumption that short-search labor is a minimum. Joint placements
or relaxed low-value service are possible next local searches, guided by value.

The current promoted reference remains opening_q32_b13_v1. Its broad and new
opening-league evidence is in results/opening_market_validation.json. The
committed catalog remains Bohann; the last submission remains investment.
The original goal still includes fast composition economics/service estimation,
placement and schedules, trade timing, estimate-versus-execution feedback,
general animal/wait decisions, larger or cold proposals, fresh replay borrowing,
and improvement against a growing league. Do not narrow it to labor savings.

Fresh19:02 evidence:72 player-games,53 unique replays, mean cash$97672.46,
hires276.92, hire cost$5268.81, productive fertilizer coverage.41804. Changed
notebooks are Fields of Fortune, Can Specialists Beat One Agent, and V5 Hybrid;
metadata is retrieved, but their code still needs a static content audit.
The82 comparisons below use the new global cohort and unchanged promoted-q32
local64 profile. Different cohorts are not paired performance improvements.
Next review19:28 UTC; next public refresh around20:02 UTC.

| Metric | Global72 | q32 local64 |
| --- | ---: | ---: |
'''
for row in rows:text+=f"| {row['metric']} | {row['global72']:.4f} | {row['local64']:.4f} |\n"
(EXP/'research/review_52.md').write_text(text)
(EXP/'research/review_52_comparison.json').write_text(json.dumps(rows,indent=2)+'\n')
entry=f'\n{now}: Review52: longer day-solver search saves6/$665 or7/$754 hires per activated goose game, all64 production/sales/rival actions unchanged per leaf. Mean versus crop control now-$68.91 off / +$58.39 on. No new promotion. Final-day mixed-market replay check needs full-game interpretation; two off days unresolved. Global1902 refreshed72/53; three notebook code audits pending. Next19:28, goal through00:47.\n'
for name in ['PROGRESS.md','IDEAS_LEDGER.md','PROFILING_LEDGER.md']:
    with (EXP/name).open('a') as file:file.write(entry)
next_file=EXP/'NEXT.md'
archive=EXP/'checkpoints/NEXT_0812_historical.md'
if not archive.exists():archive.write_bytes(next_file.read_bytes())
next_file.write_text(f'''# Current state — {now}

Active goal ends September8,00:47UTC. Current reference:
`CURRENT_REFERENCE.json`, opening_q32_b13_v1. Committed catalog Bohann and
last submitted investment remain separately frozen. No new Git/upload/catalog
operation is authorized. Current AGENTS applies; retired strategy guide stays
retired. CPU is appropriate; leave the unrelated GPU job intact. No subagents.

Immediate task: finish the day-solver optimization requested by the user.
`runs/late_animal_schedule_001/COMBINED30_AUDIT.json` proves6/$665 and7/$754
hire savings per activated game, with unchanged production, sales and rival
actions in64 games per leaf. The first3-second solve overstated achievable
labor cost. No claim of minimum workers is justified by UNKNOWN.

Fix retry.cpp's post-merge physical check for final-day simultaneous buy/sell
netting: the pure solver schedule is certified, but mixed executable orders
must be checked in the full game. Its original combined check emits unexpected
market-order warnings and end-stock errors caused by replay semantics. Preserve
all failed-attempt evidence. Off29 fixed and no-care variants need actual-game
audit; default day29 route still uses one extra hire. Retry off days22/23 with
more budget or reuse/repair the successful on routes when contracts permit.
Integrate only candidates that preserve actual production and required sales.

`runs/late_animal_rotation_001/` contains exact goose/cow/sheep/control courses,
198-proposal dated C++ estimates per off/on source, and causal profiles. Their
base is q32 and they retain alternative berry continuations. Unconditional
goose initial routes lose money; after solver improvements the on leaf gains
$58.39 mean margin on64 discovery games. Package observation-only WIP policies,
preserve the berry branch, compare both leaves and appropriate observed-shop
selection, then fresh broader league if promising. Do not infer unavoidable
costs for cow/sheep from the same short compilation budget. Consider joint
placements and locally skipping low-value service when they improve economics.

Original full scope remains active: composition search and valuation, placement,
worker/trade compilation, prediction-versus-execution feedback, general animals
and wait, independent/cold and larger composition changes, replay/public-source
learning, and repeated growing-league evaluation. Keep new proposals moving.

Fresh evidence is `research/refresh_1902/`, complete72 player-games/53 replays.
Three changed notebook refs in notebook_changes.json still need code pulls and
static comparisons. Next public refresh around20:02. Review52 has all82 metrics;
next review19:28. All tool processes were terminal at the checkpoint. No new
submission is authorized; final seed reserve900000 remains unused.
''')
print('Review52 recorded',now)
