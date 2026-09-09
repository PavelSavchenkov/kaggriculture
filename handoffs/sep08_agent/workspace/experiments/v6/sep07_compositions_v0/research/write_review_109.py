"""Record deployment parity and the diagnosed caller-side timeout regression."""
import csv
import json
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
EXP = HERE.parent
refresh = HERE / 'refresh_sep08_1508'
means = json.loads((refresh / 'invariant_means.json').read_text())['means']
selection = list(csv.DictReader((refresh / 'top_replay_selection.csv').open()))
episodes = len({row['episode_id'] for row in selection})
old = (HERE / 'review_108.md').read_text()
lines = old[old.index('| Metric |'):].splitlines()
table = ['| Metric | Global72 (15:08) | Current native256 |', lines[1]]
aliases = {'SELL_weighted_hour': 'SELL_mean_hour', 'BUY_PRODUCT_weighted_hour': 'BUY_PRODUCT_mean_hour'}
for line in lines[2:]:
    if line.startswith('| '):
        metric, _, local = [part.strip() for part in line.split('|')[1:-1]]
        table.append(f'| {metric} | {means[aliases.get(metric, metric)]:.4f} | {local} |')
assert len(table) == 84
stamp = datetime.now(timezone.utc).isoformat()
body = f'''# Review 109 — {stamp}

One Kaggle upload remains authorized, with no attempt yet. The broad132864 audit
is still running; accepted reference remains empty_sale_slots_m2. Do not promote
the provisional q24/animal/premium candidate until all preregistered gates and
guard investigations are resolved.

Frozen C++ beats teammate4040/4096 (98.63%, mean margin11793.09) and last
submitted3883/4096 (94.80%, mean margin3430.81). The actual unpacked Python agent
matches all94908 actions in132 official-environment full games. It wins63/64
against teammate and62/64 against last submission. The128 paired final-cash
records also equal C++. File-path selfplay completes. An additional27 exact C++
trajectories match19413 actions, including the day23 weed repair and two known
missed-guard courses. That cohort does not activate the rival-wool transfer.
Frozen source-only rebuild reproduces all three artifact hashes exactly.

The first guard audit reproduces134 full game records across34 matchups, including
86 missed-guard games. Every first divergence is tile38 EMPTY versus WEED on
day23. Later differences are wheat growth, own wheat seeds and shed wheat;
animal state remains covered and matching. Further completed broad files are
being traced before a final diagnosis is accepted.

All four care/workforce courses pass independent719-step replay and all daily
endpoints with equal production on both farms. Care omission alone saves$0.
Searching one fewer worker saves$178 in context1014 but initially loses$144 in
1019 because fresh timed searches fail to rediscover known cheaper days25/29.
Retaining the cheaper certified schedule per day instead saves$178 in BOTH
contexts, with identical output and rival cash. Hire cost5044 to4866.

The user's question prompted a root API inspection: day_solver/scheduler.hpp
explicitly returns UNKNOWN, takes a fixed workforce, and accepts no previous
schedule. Root quick_v30 preserves its accepted result within a call. The bug
is in our experimental outer optimizer's failure to retain schedules across
runs. The caller now accepts a saved course, rechecks orders, strict physical
requirements and full-game endpoints, and reuses it before escalating hires.
A zero-new-search regression check is being built. No root solver change or
Git operation is warranted by this evidence.

The original ideas remain active: dated composition and fast economics, cheap
but improvable placement/labor estimates, exact scheduling, service exceptions,
observed-shop and opponent branches, replay borrowing with lineage, cold/larger/
mixed farms, league growth and estimate-versus-realized error correction.
Priority remains the requested submission, then improved workforce courses and
mixed-course funding/service. Keep rival-flow forecasts and public component
leads queued. No GPU work is needed; the unrelated training stays untouched.

Refresh15:08 contains72 player records/{episodes} unique replays; changed notebooks
are Nusrati2715.6 and AhmedV24. Inspect contents before treating them as new
strategies. The82 measurements below use the fresh global cohort and reuse the
accepted localnative256 cohort; unmatched populations do not show causal gains.
Final900000 remains unused. Next review15:30UTC, refresh16:08UTC.

'''
(HERE / 'review_109.md').write_text(body + '\n'.join(table) + '\n')
note = f'\n{stamp}: Review109. Packed94908 plus rare19413 actions exact; frozen CPP teammate4040/4096, lastsubmission3883/4096. Broad132864 pending, no upload. Both retained-incumbent cow courses save178 with identical output. Timeout regression belongs to experiment caller, not root solver; integrated zero-budget check pending. Fresh global72/{episodes}, 82 metrics updated. Nextreview15:30/refresh16:08.\n'
for name in ('PROGRESS.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'LINEAGE.md', 'NEXT.md'):
    with (EXP / name).open('a') as stream:
        stream.write(note)
print('Review109 saved with82 metrics.')
