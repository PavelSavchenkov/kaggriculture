"""Validate the integrated compiler's selected schedules and collect daily costs."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import csv
import hashlib
import json
import subprocess

RUN=Path(__file__).resolve().parent
ROOT=RUN.parents[4]
compiled=RUN/'broader_land_30s'
execution={'cases':[json.loads((compiled/'goose_pair_d10_EXECUTION.json').read_text())]}
assert execution['cases'][0]['returncode']==0
out=RUN/'broader_goose_audit';out.mkdir(exist_ok=False)
binaries={r:RUN/f'build/audit_{r}' for r in ('public','pass')}
jobs=[]
files=set()
for case in execution['cases']:
    assert case['returncode']==0
    name=case['name'];prefix=compiled/name
    days=list(csv.DictReader((prefix/'compile.csv').open()))
    final=next(x for x in reversed(days) if x['day']=='29' and x['endpoint']=='1')
    extra=int(final['extra_hires']);folder=prefix/'days/29'
    schedule=folder/f'raw_schedule_h{extra}.txt'
    problem=folder/f'problem_h{extra}.json'
    orders=folder/f'orders_h{extra}.txt'
    command=['conda','run','--no-capture-output','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),
        str(binaries['public']),str(prefix),str(schedule),str(problem),str(orders),str(case['seed']),str(out/name)]
    jobs.append({'name':name,'seed':case['seed'],'extra_hires':extra,'command':command})
    files.update([schedule,problem,orders,*prefix.glob('days/*/actions.txt'),*prefix.glob('days/*/problem.json')])
def hashes():
    return {str(p.relative_to(RUN)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
before=hashes()
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'cases':jobs,'inputs':before,'binary_sha256':{r:hashlib.sha256(b.read_bytes()).hexdigest() for r,b in binaries.items()},
    'scope':'Full719-turn action validation, all endpoints and daily economics for the integrated compiler. Completed earlygoose course selected while other compiler cases continue; not a runtime league policy.'},indent=2)+'\n')
def run(job):
    with (out/(job['name']+'.log')).open('x') as log:
        result=subprocess.run(job['command'],stdout=log,stderr=subprocess.STDOUT)
    print(job['name'],result.returncode,flush=True)
    return dict(job,returncode=result.returncode)
with ThreadPoolExecutor(max_workers=2) as pool:records=list(pool.map(run,jobs))
assert hashes()==before
(out/'EXECUTION.json').write_text(json.dumps({'cases':records,'inputs_unchanged':True},indent=2)+'\n')
assert all(x['returncode']==0 for x in records)
