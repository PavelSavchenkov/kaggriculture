from datetime import datetime, timezone
from pathlib import Path
import json

R = Path(__file__).resolve().parent
E = R.parent
now = datetime.now(timezone.utc).isoformat()
rows = json.loads((R / 'review_87_comparison.json').read_text())
assert len(rows) == 82
(R / 'review_88_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')
body = f'''# Review88 — {now}

Due07:28; recorded after answering the requested lineage summary. That reporting
turn verified existing evidence but added no new research result. Research now
resumes; no live old process exists and there is no blocker. Current reference
remains empty_sale_slots_m2. Full objective remains active through Sep10 00:47UTC.
No Git, catalog copy, submission, subagents or GPU changes. Final900000 untouched.

Cold service_bank_p362_m2 has now passed the independently frozen2560-game
confirmation, all6gates and13216originalcodehashes. Equal-opponent owncash gain
614.41, seed-cluster95%[410.76,822.27]. All5opponentmeans positive. Directoldrelaxed
parent182W10T64L/256,+1185.63margin. Still0W256L against reference, margin-60495.49.
Coverage640exactrecords: every activated program finishes; none abandoned.
This improves one weak composition's execution, not our strongest agent.

Reprioritize the cold branch around composition quality as well as compilation.
The original p362 estimate itself forecast57373cash and-37417margin, with a6984
fundingdeficit and212uncoveredwork. Those were old fixedflow scenarios, not paired
with the recent confirmation: do not compare their cash means as estimator error.
Measure intended lifetimes, actual placement/lifetimes with ideal service, and
realized output on common fresh games. Day-level profiles omit harvest timing;
do not present a day-mask approximation as exact replay or a certified bound.

Farm Signal Engine C++ port prepared, not compiled or tested yet. It reuses the
verified V5/2 five schedules and routes, adds only the faithful99-slot day-close
rule. Next: component oracle parity, exact disabled controls, full games and
required operational checks. This bounded reuse study runs alongside the cold
composition diagnosis; it must not replace the broader search objective.

All82metrics below explicitly reuse07:08 top72player-games/68replays and frozen
current native256; no new replay cohort is claimed. Populations are unmatched.
Current farm has much less tomato output, more milk/strawberries, and fewer
faults/hires than this cohort. These differences motivate alternatives, not a
claim that any product mix alone causes strength. Nextreview07:48,refresh08:08UTC.

| Metric | Global72 (07:08, reused) | Current native256 (reused) |
| --- | ---: | ---: |
'''
body += ''.join(f"| {r['metric']} | {r['global72']:.4f} | {r['local256']:.4f} |\n" for r in rows)
(R / 'review_88.md').write_text(body)
entry = f'\n{now}: Review88. Cold service-bank fresh2560 confirms +614.41 owncash across5 opponents but still0/256 vs best. Next diagnose composition/placement/service gaps on common games; old p362 estimate already weak and underfunded. Farm Signal C++ capacity component prepared, parity/discovery pending. All82 comparison metrics explicitly reuse07:08 cohort/currentnative256. Best unchanged.\n'
for name in ['IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'PROGRESS.md']:
    with (E / name).open('a') as out:
        out.write(entry)
print('Review88 complete; next07:48UTC.')
