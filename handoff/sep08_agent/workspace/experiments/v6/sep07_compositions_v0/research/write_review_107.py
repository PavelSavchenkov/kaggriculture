"""Record the new replay cohort and the authorized submission priority."""
import csv
import json
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
EXP = HERE.parent
refresh = HERE / 'refresh_sep08_1408'
means = json.loads((refresh / 'invariant_means.json').read_text())
selection = list(csv.DictReader((refresh / 'top_replay_selection.csv').open()))
episodes = len({row['episode_id'] for row in selection})
old = (HERE / 'review_106.md').read_text()
lines = old[old.index('| Metric |'):].splitlines()
table = ['| Metric | Global72 (14:08) | Current native256 |', lines[1]]
aliases = {'SELL_weighted_hour': 'SELL_mean_hour', 'BUY_PRODUCT_weighted_hour': 'BUY_PRODUCT_mean_hour'}
for line in lines[2:]:
    if not line.startswith('| '):
        continue
    metric, _, local = [part.strip() for part in line.split('|')[1:-1]]
    value = means['means'][aliases.get(metric, metric)]
    table.append(f'| {metric} | {value:.4f} | {local} |')
assert len(table) == 84
stamp = datetime.now(timezone.utc).isoformat()
body = f'''# Review107 — {stamp}

The user authorized one official Kaggle submission, followed by continued work.
Finish the running broad audit, choose the strongest validated candidate, freeze
its C++ source, verify the exact Python archive against C++ and the teammate,
upload once, and reproduce the official validation replay. Do not use Git or
modify the official agent catalog. No automatic second upload is authorized.

The 132,864-game independent audit of accepted empty_sale_slots_m2 and both
repaired animal/premium combinations is live (handle62984). Discovery's q24
leader remains provisional: it improves eight-opponent win score11.816pp but
loses King margin. Preserve the preregistered gates and investigate guard misses.
Current accepted/cold references remain empty_sale_slots_m2/early_melon_b98_m1.

The mixed day9 financial repair has now passed both the physical certificate and
live endpoint: two partial early wheat purchases are replaced by actual quantities
and two wheat bought at hour23. Continuing the whole season fails on day10:
stock and animal-care states diverge. The day9 fix alone is not a usable course.
Next trace day10 animal purchases, placement and feed with cash at each event.

Cow care-cap and one-fewer-worker compiler variants are running (handle42461).
Both care-only contexts completed compilation; full-season audits remain due.
The minimum-worker variants retain strict endpoints. Do not count a solver
certificate as realized profit until the full719-transition game is checked.

Refresh14:08 supplies72 top-player records from{episodes} unique replays. The
82 metrics below update the global cohort and reuse native256 local measurements;
the populations are unmatched and do not establish causal differences. Four
changed notebooks were downloaded without executing cells. Tetsutani is a small
Yusuke derivative: merge duplicate same-product sales and pack SELL-only step550.
AhmedV24 source is unchanged. Dmitrii's Apache2.0 terminal planner preserves
baseline deposit prefixes and tries extra harvest/deposit actions at steps710..718;
self-reported gains are small and unverified locally. Keep these component leads.

After submission preparation, continue animal-service cost, mixed-course funding,
and observation-only rival-flow forecasting. Preserve larger/mixed/cold searches.
Final900000 remains unused. No suitable GPU workload has been identified.
Next review14:50UTC; next replay/notebook refresh15:08UTC.

'''
(HERE / 'review_107.md').write_text(body + '\n'.join(table) + '\n')
note = f'\n{stamp}: Review107. One Kaggle upload authorized. Broad132864 live62984; care compiler live42461. Mixedday9 certified, day10 animal/stock mismatch blocks full course. Global82 metrics updated to1408 cohort72/{episodes}; localnative256 reused. Tetsutani sale-merge and Dmitrii terminal-delivery leads retained. Nextreview14:50/refresh15:08.\n'
for name in ('PROGRESS.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'LINEAGE.md'):
    with (EXP / name).open('a') as stream:
        stream.write(note)
print(f'Review107: 82 metrics, 72 records, {episodes} unique replays.')
