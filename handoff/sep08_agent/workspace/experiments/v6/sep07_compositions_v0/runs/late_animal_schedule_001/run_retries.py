from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import csv
import hashlib
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
SOURCE=EXP/'runs/late_animal_rotation_001'
cases=[]
seen={}
for leaf in ['off','on']:
    folder=SOURCE/f'goose_c31_d13_{leaf}_002'
    for row in csv.DictReader((folder/'compile.csv').open()):
        if row['endpoint']!='1' or row['extra_hires']=='0':continue
        day=int(row['day']);problem=folder/f'days/{day}/problem_h0.json';actions=folder/f'days/{day}/actions.txt'
        # Same physical problem plus same market orders is a reusable query.
        orders=[]
        for line in actions.read_text().splitlines():
            values=list(map(int,line.split()));units,count=values[:2]
            orders.append(values[2+units*3:2+units*3+count*3])
        signature=hashlib.sha256(problem.read_bytes()+json.dumps(orders).encode()).hexdigest()
        name=f'{leaf}_day{day}_fixed30'
        record={'name':name,'leaf':leaf,'day':day,'kind':'unchanged_contract','problem':str(problem),
                'actions':str(actions),'signature':signature,'alias_of':seen.get(signature)}
        if signature not in seen:seen[signature]=name
        cases.append(record)
    day=29;problem=folder/f'days/{day}/problem_h0.json';spec=json.loads(problem.read_text())
    assert not any(job['op']=='feed' for work in spec['tile_work'] for job in work['actions'])
    removed=0
    for work in spec['tile_work']:
        before=len(work['actions']);work['actions']=[job for job in work['actions'] if job['op']!='care'];removed+=before-len(work['actions'])
    spec['tile_work']=[work for work in spec['tile_work'] if work['actions']]
    path=RUN/f'{leaf}_terminal_no_care.json';path.write_text(json.dumps(spec,indent=2)+'\n')
    cases.append({'name':f'{leaf}_day29_no_care30','leaf':leaf,'day':29,'kind':'remove_terminal_care','removed_jobs':removed,
                  'problem':str(path),'actions':str(folder/'days/29/actions.txt'),'alias_of':None})
record={'created_utc':datetime.now(timezone.utc).isoformat(),'budget_seconds':30,'fallback_workers':4,
        'purpose':'Distinguish short search failure from actual extra labor. Physical solutions remain provisional until full-game integration.',
        'cases':cases,'source_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [Path(__file__),RUN/'retry.cpp',RUN/'CMakeLists.txt']}}
(RUN/'PREREGISTERED.json').write_text(json.dumps(record,indent=2)+'\n')
def run(case):
    if case['alias_of']:return case
    command=['conda','run','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),str(RUN/'build/retry'),case['problem'],case['actions'],str(RUN/case['name']),'30']
    with (RUN/(case['name']+'.log')).open('w') as log:
        result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
    answer={**case,'command':command,'returncode':result.returncode,'completed_utc':datetime.now(timezone.utc).isoformat()}
    (RUN/(case['name']+'.process.json')).write_text(json.dumps(answer,indent=2)+'\n')
    status_path=RUN/case['name']/'STATUS.json'
    status=json.loads(status_path.read_text()) if status_path.exists() else {}
    print(case['name'],result.returncode,status,flush=True)
    return answer
with ThreadPoolExecutor(max_workers=2) as pool:
    results=list(pool.map(run,cases))
(RUN/'PROCESS_RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
