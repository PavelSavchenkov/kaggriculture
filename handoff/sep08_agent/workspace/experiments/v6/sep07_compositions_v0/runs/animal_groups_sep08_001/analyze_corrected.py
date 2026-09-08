"""Summarize the corrected full-lifecycle screen and retain the original error."""
from collections import Counter
from pathlib import Path
import csv
import json

RUN=Path(__file__).resolve().parent
folder=RUN/'estimates_corrected_1000'
execution=json.loads((RUN/'estimates_corrected_1000_EXECUTION.json').read_text())
assert execution['returncode']==0 and execution['sources_unchanged']
controls=[json.loads(line) for line in (folder/'CONTROLS.jsonl').read_text().splitlines()]
assert len(controls)==18 and all(r['animal_lives_exact']>0 for r in controls)
best={};counts=Counter();seconds=0.;selected={}
initial=json.loads((RUN/'fixtures_3s/PROTOCOL.json').read_text())['cases']
initial_keys={(c['seed'],c['estimate']['rotations']):c for c in initial}
with (folder/'proposals.csv').open() as f:
    for row in csv.DictReader(f):
        for k in ['seed','id','count','entry_day','animal_cost','removed_animals','seeds_saved','field_operations_delta','source_visit_deficit']:
            row[k]=int(row[k])
        for k in ['mean_own_gain','mean_margin_gain','margin_sd','risk_score','estimate_seconds']:
            row[k]=float(row[k])
        seconds+=row['estimate_seconds'];counts['proposals']+=1
        counts[f"edited_{row['count']}"]+=1
        counts[f"positive_{row['count']}"]+=row['risk_score']>0
        counts['displaces_planned_animals']+=row['removed_animals']>0
        key=row['seed'],row['count']
        if key not in best or (row['risk_score'],-row['source_visit_deficit'])>(best[key]['risk_score'],-best[key]['source_visit_deficit']):best[key]=row
        key=row['seed'],row['rotations']
        if key in initial_keys:
            original=initial_keys[key]
            selected[original['name']]={'original':original['estimate'],'corrected':row}
result={'controls':controls,'crop_lifetimes_exact':sum(c['crop_lives_exact'] for c in controls),
    'animal_lifetimes_exact':sum(c['animal_lives_exact'] for c in controls),'full_control_games':36,
    'counts':dict(counts),'estimate_seconds':seconds,'microseconds_per_proposal':seconds*1e6/counts['proposals'],
    'best':list(best.values()),'initial_fixture_revaluation':selected,
    'scope':'Retrospective source calendar,18exposed shop streams,public_router seat0;32conditional future-shop samples and unpriced marginal labor. No new deployable policy or league gain.'}
(RUN/'CORRECTED_ANALYSIS.json').write_text(json.dumps(result,indent=2)+'\n')
lines=['# Corrected crop and animal opportunity costs','',
    f"{counts['proposals']:,} proposals;36 full source/control games;{result['crop_lifetimes_exact']} exact crop lifetimes and{result['animal_lifetimes_exact']} exact animal lifetimes.",'',
    f"Estimator time {seconds:.3f}seconds, averaging{result['microseconds_per_proposal']:.2f}microseconds per proposal including32shop samples.",'',
    'The screen now subtracts already planned animal output, feed, fertilizer and work. ',
    'It credits canceled purchases only up to remaining future buys. Animals already ',
    'bought are sunk inventory. A changed tile can replace or advance a planned animal; ',
    'its edit count is not necessarily a net herd increase.','',
    '| Initial fixture | Original margin estimate | Corrected margin estimate | Planned animals replaced |',
    '| --- | ---: | ---: | ---: |']
for name,value in selected.items():
    a,b=value['original'],value['corrected']
    lines.append(f"| {name} | {float(a['mean_margin_gain']):+.2f} | {b['mean_margin_gain']:+.2f} | {b['removed_animals']} |")
lines += ['', 'Original flawed scores and duplicate-build compiler fixtures remain preserved. ',
    'Corrected estimates still require compiled schedules, real funding/storage checks ',
    'and an observation-only branch policy before any playing-strength claim.','']
(RUN/'CORRECTED_RESULTS.md').write_text('\n'.join(lines))
print({k:result[k] for k in ['crop_lifetimes_exact','animal_lifetimes_exact','estimate_seconds','microseconds_per_proposal']})
for name,v in selected.items():print(name,v['original']['mean_margin_gain'],v['corrected']['mean_margin_gain'],v['corrected']['removed_animals'])
print('Best triple groups:')
for row in result['best']:
    if row['count']==3:print({k:row[k] for k in ['seed','rotations','removed_animals','mean_own_gain','mean_margin_gain','risk_score']})
