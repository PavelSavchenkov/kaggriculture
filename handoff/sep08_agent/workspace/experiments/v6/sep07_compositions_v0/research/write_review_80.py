from datetime import datetime, timezone
from pathlib import Path
import json

R = Path(__file__).resolve().parent
E = R.parent
now = datetime.now(timezone.utc).isoformat()
rows = json.loads((R / 'review_79_comparison.json').read_text())
assert len(rows) == 82
(R / 'review_80_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')
report = json.loads((E / 'runs/joint_resource_routes_sep08_001/ANALYSIS.json').read_text())
assert report['games'] == 960 and report['exact_old_joint_full_records'] == 192
checks = json.loads((E / 'runs/empty_sale_validation_sep08_001/CHECKS.json').read_text())
assert checks['games'] == 1024
text = f'''# Review80 — {now}

Due04:28UTC. Goal remains active through Sep10 00:47UTC; no blocker.
Best remains observed_sale_lead_start_216. There are nine accepted descendants
of the last submission. No Git, official catalog copy or external submission.

Cumulative resource routing finishes960 games with192 complete old-route controls
equal. Earlier wheat harvest/fertilizer collection can reduce estimated initial
stock and change withdrawals, but benefits vary by farm. Even the better dense
cases remain below the original compiler. The current-stock scarcity penalty
usually hurts. Do not claim that more sophisticated input costing fixes incomplete
daily task sets or resource exchanges across workers. Operational checks follow.
Next compiler step should compare a full feasible day-solver schedule with these
same failed states, then add demonstrated missing constraints. No more cost-only
variants until that comparison supplies a concrete mechanism.

Empty-sale mode2 completes1024 operational games: generic/pair/debug/thread
records equal, full self-play/PASS checks and native/custom shops pass. Frozen
pair rebuilding and67584 fresh games are still running. Its5632-game native
audit already failed the required per-game parent margin gate once(-6). This
candidate must not be promoted as-is. Two exact full-game traces are being
built for that case, including same-state one-step raw-order counterfactuals.
Those will test whether removing zeros shifts input trades or whether even
pure non-input sale timing can help the rival more than ourselves. Keep any
new policy independent of seed/opponent identity and give it fresh audit seeds.

NEXT.md now identifies live work and supersedes the stale03:00 checkpoint.
No jobs were restarted. No accepted source was changed by these diagnostics.
The broader goal still includes large composition/family changes, cheap funding
and labor estimates, compilation, observation-driven branches and replay reuse.
Route failures are an execution bottleneck, not evidence against composition
search itself. Retain separate incumbent improvement and cold-start work.

All82 metrics below carry Review79's04:08 fresh72-player-game cohort and the
frozen accepted-agent256-game native panel. These unmatched cohorts are useful
for gap finding, not a head-to-head leaderboard estimate. No new public source
since the04:08 audit. Next review04:48; next replay refresh05:08UTC.

| Metric | Global72 (04:08) | Current native256 |
| --- | ---: | ---: |
'''
text += ''.join(f"| {row['metric']} | {row['global72']:.4f} | {row['local256']:.4f} |\n" for row in rows)
(R / 'review_80.md').write_text(text)
entry = f'\n{now}: Review80: cumulative-input routes960/192 exact controls give partial, farm-dependent recovery but remain below original dense compiler. Stop cost-only variants; compare exact whole-day task/resource schedules. Empty-sale1024 operational games pass; native parent margin gate already fails once(-6), so no promotion. Broad fresh and isolated rebuild live; causal native trace launched. All82 metrics carry04:08 cohort. Best unchanged.\n'
for name in ['IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'PROGRESS.md']:
    with (E / name).open('a') as out:
        out.write(entry)
print('Review80 complete; next04:48UTC.')
