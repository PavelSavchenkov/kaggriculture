"""Record the extended goal, current native profiles and fresh global cohort."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import statistics

RESEARCH = Path(__file__).resolve().parent
EXP = RESEARCH.parent
latest = RESEARCH/'refresh_sep08_0108'
global_means = json.loads((latest/'invariant_means.json').read_text())['means']
profile_path = EXP/'runs/observed_sale_validation_002/native/observed_sale_lead_start_216_vs_rival_wool_context_v3.json'
data = json.loads(profile_path.read_text())
local = json.loads(profile_path.with_suffix('.profile.json').read_text())['means']
local['unit_faults'] = local['faults']
for item in ['WHEAT','CARROT','TOMATO','STRAWBERRY','MELON']:
    local[f'harvest_{item}'] = local[f'produced_{item}']
for side, key in [('sell','SELL_weighted_hour'),('buy','BUY_PRODUCT_weighted_hour')]:
    values = []
    for game in data['games']:
        hours = game['profile'][f'{side}_hours']
        total = sum(sum(row) for row in hours)
        values.append(sum(h*sum(row) for h,row in enumerate(hours))/total if total else 0)
    local[key] = statistics.mean(values)
rows = []
for old in json.loads((RESEARCH/'review_69_comparison.json').read_text()):
    key = old['metric']
    gkey = {'SELL_weighted_hour':'SELL_mean_hour','BUY_PRODUCT_weighted_hour':'BUY_PRODUCT_mean_hour'}.get(key,key)
    rows.append({'metric':key,'global72':global_means[gkey],'local256':local[key]})
assert len(rows)==82 and len(data['games'])==256
(RESEARCH/'review_70_comparison.json').write_text(json.dumps(rows,indent=2)+'\n')
now = datetime.now(timezone.utc).isoformat()
text = f'''# Review 70 — {now}

The user extended the full objective through September 10 at 00:47 UTC. Earlier
closure text describes the initial 24-hour checkpoint only. The active goal,
README, OBJECTIVE and NEXT now agree. The preceding user-requested lineage turn
completed a verified nine-version report; it did not change the best policy.

Current accepted reference is observed_sale_lead_start_216. Its fresh 65,536
games against 32 opponents passed all declared gates. Direct parent 959/1,024,
submitted 957/1,024, teammate 1,011/1,024. No Kaggle ranking is inferred.
Production and labor are unchanged in the native timing comparison. This is
an execution-price gain, not a solution to arbitrary composition construction.

The next experiment isolates a specific estimator/compiler mismatch. The
generic compiler hires from currently visible jobs; the dated estimator counts
the whole day's work. Four fixed farms compare unchanged hiring, eight hands,
ten hands, and the existing estimate_plan count. All other policy behavior is
preserved. Two cold farms and two source compositions prevent this from becoming
another single-incumbent parameter search. Old target-commitment and fertilizer
priority failures remain negative evidence. Measure production, fulfillment,
hires, cost, movement and idle actions before deciding what to change next.

The first build exposed a duplicate definition in the new shared cold-farm
header. It was fixed with an inline function in a separate namespace; the
failed log is retained. The new build/discovery process is live (session80095).
No compiler strength result is claimed before the batch completes.

Fresh top12 x six player-games were retrieved and analyzed in
refresh_sep08_0108. One changed public notebook, holeneckles/grandmaster, was
downloaded and statically inspected. It is a small crop-only greedy policy;
the claimed two daily hires actually issue only one HIRE at hour0, and its
melon horizon can plant too late for the stated first harvest. Its $25,000+
claim is unverified and does not establish strength. Do not prioritize a full
port over the currently measured compiler failures. Keep its original source
and audit, and continue checking future fresh notebooks.

All 82 metrics below use the NEW accepted reference's 256 native games against
its parent and the NEW 72-game global cohort. These are different scenarios and
opponents, not paired proof of a leaderboard gap. Productive crop fertilization
is 20.9% locally versus 43.4% globally, while crop-days are similar. Treat this
as a source of specific crop-calendar questions, not a universal service rule.

Full scope remains: cheap composition economics, placement, exact scheduling,
estimation-error feedback, observed-shop/opponent branches, independent farms,
large changes, a growing league and source attribution. The arbitrary-farm
compiler and labor/finance models remain incomplete. Final900000 stays unused.
Next review01:28UTC; next hourly refresh02:08UTC. No Git, submission or official
catalog copy is authorized by the continuation.

| Metric | Global72 (01:08) | Current native256 |
| --- | ---: | ---: |
'''
text += ''.join(f"| {r['metric']} | {r['global72']:.4f} | {r['local256']:.4f} |\n" for r in rows)
(RESEARCH/'review_70.md').write_text(text)
audit = {'created_utc':now,'ref':'holeneckles/grandmaster','inspection':'Static source only; notebook code not executed.',
    'files_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest()
                    for p in (latest/'notebook_audit/grandmaster').iterdir() if p.is_file()},
    'observations':['DAILY_HIRES=2, but one HIRE is appended only at hour0; the requested target is not realized by this code.',
        'No animals, no land expansion, fixed melon/carrot/wheat priority. The source declares its own profit claim; no exact local parity or strength validation.',
        'days_left>=9 permits late melons although first production age is10. Late harvest requests also use yield without consistently checking first production age.'],
    'decision':'Retain source; not prioritized for C++ port given observed limitations and no evidence of a strong controller.'}
(latest/'notebook_audit/AUDIT.json').write_text(json.dumps(audit,indent=2)+'\n')
entry = f'\n{now}: Review70: goal extended through Sep10 00:47; latest nine-version best validated. New72 global games and one notebook audited; all82 current/global metrics refreshed. Hiring-only generic-compiler experiment builds from measured pending-job versus dated-work mismatch. Next review01:28, refresh02:08.\n'
for name in ['IDEAS_LEDGER.md','PROFILING_LEDGER.md','PROGRESS.md']:
    with (EXP/name).open('a') as file:
        file.write(entry)
print('Review70: all82 metrics updated; next01:28UTC.')
