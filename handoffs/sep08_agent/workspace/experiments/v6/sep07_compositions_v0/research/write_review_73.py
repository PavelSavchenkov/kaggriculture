"""Refresh all cohort metrics and record the later-day evidence-driven pivot."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RESEARCH = Path(__file__).resolve().parent
EXP = RESEARCH.parent
latest = RESEARCH/'refresh_sep08_0208'
means = json.loads((latest/'invariant_means.json').read_text())['means']
rows = json.loads((RESEARCH/'review_70_comparison.json').read_text())
for row in rows:
    key = {'SELL_weighted_hour':'SELL_mean_hour','BUY_PRODUCT_weighted_hour':'BUY_PRODUCT_mean_hour'}.get(row['metric'],row['metric'])
    row['global72'] = means[key]
assert len(rows) == 82
(RESEARCH/'review_73_comparison.json').write_text(json.dumps(rows,indent=2)+'\n')
now = datetime.now(timezone.utc).isoformat()
text = f'''# Review73 — {now}

Due02:08; written late after the user-requested nine-version lineage report.
That report completed the requested comparison from authoritative validation
files. It did not improve the policy; do not count it as a new promotion.
Best remains observed_sale_lead_start_216; goal stays active through Sep10 00:47.

Twenty-four new complete cold-farm traces reproduce both agents' action hashes
and final cash. The first serializer wrote int8 as control characters; corrected
traces are independently parsed and the invalid files remain separately saved.
Requested productive biology [140,0,0,64,72,0,72,68,116] versus one realized
[128,0,0,62,72,0,70,66,116] shows the SMALL cold farm is already close to its
biological ceiling. The remaining strength gap primarily requires a larger
composition. This supersedes the suggestion to spend all effort on its later
routes. Dense replay farms still show large compiler execution losses.

A concrete service error explains the animal shortfall: full old care banks
suppress CARE on the night that consumes and clears them. Sheep skip day5 and
cows day7. Today care should bank after that production for the next one.
Skipping day0 sheep service shifts a bonus to the second production because
it avoids this faulty cap, so the earlier equal output is not evidence that
initial service is redundant. A one-condition fix now has twelve isolated C++
packages: old/new on two cold farms and two dense farms under recorded and
productive service. Discovery process68193 is live; no result yet claimed.

Next: validate the correction, then search larger dated herds funded from
early products and crop harvests, including goose/cow/sheep/mixed/wait choices.
Use estimate-versus-realization gaps to direct exact day scheduling on these
larger candidates. Keep inherited strong-agent search, independent families,
placement/cash coupling and shop/opponent branches in scope.

The02:08 replay refresh completed72 player-games. Both changed notebooks were
downloaded and statically inspected: motemen lineage census and Georgy Mamarin
live meta report are analysis tools, not new executable strong controllers.
The census hashes first24 actions; this can group openings, but cannot prove
identical later behavior. The report uses actual hires from state and separates
some wheat flows; retain these measurement ideas and source hashes, not a policy
port. Notebook attribution statements are author claims, not independently
proved full lineage. No notebook code was executed.

All82 global metrics below now use the02:08 cohort; current native256 metrics
remain the frozen accepted agent against its parent fromReview70. Different
cohorts are not a paired leaderboard estimate. Final900000 remains unused.
Next review02:28UTC; next replay refresh03:08. No Git or external submission.

| Metric | Global72 (02:08) | Current native256 |
| --- | ---: | ---: |
'''
text += ''.join(f"| {r['metric']} | {r['global72']:.4f} | {r['local256']:.4f} |\n" for r in rows)
(RESEARCH/'review_73.md').write_text(text)
audit = {'created_utc':now,'inspection':'Static source; no notebook code executed.',
    'files_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest()
                    for p in (latest/'notebook_audit').rglob('*') if p.is_file() and p.name!='AUDIT.json'},
    'census':'First24 action hash clusters and historical occupancy plots. Notebook-name labels assert source matching; full policy identity cannot be inferred from opening hashes.',
    'meta':'Replay dataset plots, actual-hire state fingerprints, market flows and whole-game action comparisons. Descriptive analysis; not a new deployable controller.',
    'decision':'Retain source and measurement ideas. No strong-agent C++ port identified in these two changed notebooks.'}
(latest/'notebook_audit/AUDIT.json').write_text(json.dumps(audit,indent=2)+'\n')
entry = f'\n{now}: Review73:24 exact cold traces reveal near-biological output on the small farm and a full-care-bank reset error. Test one-condition correction, then prioritize larger dated investments over polishing this small farm. Fresh72 global games and2 analysis notebooks audited; all82 comparison metrics updated. See research/review_73.md and runs/cold_later_days_sep08_001/CARE_BANK_WITNESS.json.\n'
for name in ['IDEAS_LEDGER.md','PROFILING_LEDGER.md','PROGRESS.md']:
    with (EXP/name).open('a') as f:
        f.write(entry)
print('Review73 complete; next02:28UTC.')
