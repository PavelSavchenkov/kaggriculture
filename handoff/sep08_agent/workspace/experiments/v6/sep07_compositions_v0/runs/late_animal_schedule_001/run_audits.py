from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import json
import statistics
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
cases=json.loads((RUN/'PREREGISTERED.json').read_text())['cases']
def run(leaf):
    chosen=[]
    for case in cases:
        if case['leaf']!=leaf or case['kind']!='unchanged_contract':continue
        name=case['alias_of'] or case['name']
        status=json.loads((RUN/name/'STATUS.json').read_text())
        if status.get('requirements') and status.get('invariants'):
            chosen.append((case['day'],RUN/name/'actions.txt'))
    mapping=RUN/f'{leaf}_combined30.txt'
    mapping.write_text(''.join(f'{day} {path}\n' for day,path in chosen))
    source=EXP/f'runs/late_animal_rotation_001/goose_c31_d13_{leaf}_002'
    output=RUN/f'{leaf}_combined30_audit'
    command=['conda','run','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),str(RUN/f'build/audit_{leaf}'),str(source),str(mapping),str(output)]
    with (RUN/f'{leaf}_combined30_audit.log').open('w') as log:
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    original=json.loads((output/'original.json').read_text())['games']
    frozen=json.loads((source/'exact/candidate.json').read_text())['games']
    candidate=json.loads((output/'candidate.json').read_text())['games']
    assert original==frozen
    result={'leaf':leaf,'command':command,'days':[day for day,path in chosen],'original_records_exact':len(original),
            'produced_equal':sum(a['produced']==b['produced'] for a,b in zip(candidate,original)),
            'sold_equal':sum(a['sold']==b['sold'] for a,b in zip(candidate,original)),
            'rival_actions_equal':sum(a['opponent_action_hash']==b['opponent_action_hash'] for a,b in zip(candidate,original))}
    for key in ['cash','worker_days','unit_faults']:
        result[key+'_gain']=statistics.mean(a[key]-b[key] for a,b in zip(candidate,original))
    for key in ['hires','hire_cost']:
        result[key+'_gain']=statistics.mean(a['profile'][key]-b['profile'][key] for a,b in zip(candidate,original))
    activated=[(a,b) for a,b in zip(candidate,original) if any(l[0]==9 and l[1:3]==[1,3] and l[5]==13 for l in b['profile']['lives'])]
    result['activated_games']=len(activated)
    result['activated_cash_gain']=statistics.mean(a['cash']-b['cash'] for a,b in activated)
    result['activated_hires_gain']=statistics.mean(a['profile']['hires']-b['profile']['hires'] for a,b in activated)
    result['activated_hire_cost_gain']=statistics.mean(a['profile']['hire_cost']-b['profile']['hire_cost'] for a,b in activated)
    result['candidate_vs_crop_control_margin']=statistics.mean(a['cash']-a['opponent_cash']-b['cash']+b['opponent_cash'] for a,b in zip(candidate,json.loads((EXP/f'runs/late_animal_rotation_001/control_c31_d13_{leaf}_002/exact/candidate.json').read_text())['games']))
    result['completed_utc']=datetime.now(timezone.utc).isoformat()
    (RUN/f'{leaf}_COMBINED30_AUDIT.json').write_text(json.dumps(result,indent=2)+'\n')
    print(result,flush=True)
    return result
with ThreadPoolExecutor(max_workers=2) as pool:
    results=list(pool.map(run,['off','on']))
(RUN/'COMBINED30_AUDIT.json').write_text(json.dumps(results,indent=2)+'\n')
