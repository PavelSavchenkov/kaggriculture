from pathlib import Path
from datetime import datetime, timezone
import json

EXP = Path(__file__).resolve().parents[2]
now = datetime.now(timezone.utc).isoformat()
path = EXP / 'runs/late_portfolio_validation_001/profiles/local64.json'
data = json.loads(path.read_text())
means = json.loads(path.with_suffix('.profile.json').read_text())['means']
previous = json.loads((EXP / 'research/review_55_comparison.json').read_text())
rows = []
for old in previous:
    metric = old['metric']
    key = metric.replace('harvest_', 'produced_') if metric.startswith('harvest_') else 'faults' if metric == 'unit_faults' else metric
    if metric.endswith('_weighted_hour'):
        field = 'sell_hours' if metric.startswith('SELL') else 'buy_hours'
        numerator = denominator = 0
        for game in data['games']:
            for hour, quantities in enumerate(game['profile'][field]):
                numerator += hour * sum(quantities);denominator += sum(quantities)
        value = numerator / denominator
    else:
        value = means[key]
    rows.append({'metric': metric, 'global72': old['global72'], 'local64': value})
assert len(rows) == 82
text = f'''# Review56 — {now}

Due20:28UTC. The original wording was reread at20:31. Full goal remains active
through September8,00:47UTC. No Git or new submission. Previous goal turn was
progress; this turn adds a promoted four-way composition selector and a new
validated public controller with four additional source calendars.

New experimental reference: late_value_s32_t0_r05, promoted20:22:53UTC.
The independent1850000..1852047 confirmation has180224games across22opponents
and two policies. Direct previous-reference result1527W2256T313L, mean+$173.98.
Current grouped utility rises.9451294→.9467957,95%gain[+.0007446,+.0025849];
historical grouped gain is also positive. Every paired mean margin improves.
The teammate result is4046W50L/4096,98.7793%. Old publicV5 is3396W700L/4096.
All preregistered gates pass. The initial45056-game interval crossed zero;
the policy stayed frozen for this independent confirmation, without pooling.

Tradeoffs remain explicit: legacy crop versions lose up to1.0742percentage
points utility and about$19 worst-decile margin. Public routers improve about
1.37–1.44points; teammate win rate stays equal. Native/PASS256 per matchup have
no utility or mean-margin regressions.896 operational checks pass and256 full
native records match the rebuilt frozen source tree (222dependencies). The
current agent has not been copied to the official catalog, pushed or submitted.

This is actual composition adaptation, not only labor savings. It chooses
retain crops/goose/cow/sheep using sampled observed-market value and actual
compiled costs, while preserving parent tomato intent and later berry rules.
On the256-game causal public-router profile it selects163retain,77goose,12cow,
4sheep; against q32 it selects165retain,75goose,16cow. Tomato/berry/melon output
is unchanged; animal output increases. Exact source/data/selection lineage is
in runs/late_portfolio_001 and results/late_portfolio_validation.json.

The user's cheap-value intuition now has measured support: all four choices
with32conditional future-shop scenarios take~.3ms, goose rank correlation
.90–.92, cow.85–.86. Sheep remains noisy.8192counterfactual games support cheap
parameter comparisons;2048 actual selected-policy records match exactly.
This feedback also corrected a short-search labor overestimate and a future-
composition mismatch hidden by equal physical entry states. It is still one
slot/date, not unrestricted placement and multi-investment construction.

Fresh ThomasV5/2 is now a validated C++ league opponent. Five719-turn schedules
use72-turn blocks, prior-route tests and only milk/wool demand plus tomato
market inventory. One tape exactly matches oldV5; four are new. All12230 source
actions and896 operational games match. On4096discovery games across two
controllers/eight opponents, newV5/2 beats oldV5 181–75 directly, but loses
63–193 to our new reference. Its63 wins commonly have0goose/6cow/11sheep by
day12 (34games), and average280.54hires versus our264.25. This is a concrete
larger composition family to learn from. The notebook's95.51% claim is not
our measured current-league rate. Tapes differ before routing boundaries;
do not infer identical physical prefixes from the phrase same opening.

Next prioritize this sheep-heavy calendar as an independent starting family:
examine dated purchases, output and service, then estimate larger herd options
and improve their executable costs/branches. Keep the new portfolio frozen as
the strongest reference and opponent. Also separate unknown-shop forecast
error from market/execution error, especially wool. Greedy placement, larger
family insertion/deletion and cold starts remain open; do not shrink the goal
to the now-successful single-tile selector or only cheaper labor.

New promoted-agent local64 profile: cash${means['cash']:.2f},hires{means['hires']:.4f},
hire cost${means['hire_cost']:.2f},faults{means['faults']:.4f}. Productive fertilizer
coverage is{means['crop_yield_day_maximized']:.5f}, versus global20:02 cohort.37186.
All82 comparisons below use the actual new reference and latest global72;
different global/local cohorts do not establish paired superiority. Fields of
Fortune's new metadata still points to byte-identical code. Next review20:48UTC;
next public refresh21:02UTC. Final900000 reserve remains unused.

| Metric | Global72 | portfolio local64 |
| --- | ---: | ---: |
'''
for row in rows:
    text += f"| {row['metric']} | {row['global72']:.4f} | {row['local64']:.4f} |\n"
(EXP / 'research/review_56.md').write_text(text)
(EXP / 'research/review_56_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')
entry = f'\n{now}: Review56: portfolio promoted after independent180224 confirmation, all gates/native/causal/frozen checks pass. Full82 metrics now new reference. NewThomasV5/2 validated12230actions+896checks, beats oldV5 181/256 but loses63/256 to us. Its winning sheep-heavy0/6/11 herd is next independent composition target. Original goal reread; bigger/earlier herds, placement and cold starts remain active. Next20:48, refresh21:02.\n'
for name in ['PROGRESS.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md']:
    with (EXP / name).open('a') as output:
        output.write(entry)
print('Review56 recorded', now)
