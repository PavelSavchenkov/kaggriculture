"""Compare the same exact solver with and without soft source task hints."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import shlex
import subprocess
import time

RUN=Path(__file__).resolve().parent
ROOT=RUN.parents[4]
out=RUN/'hint_comparison_30s'
out.mkdir(exist_ok=False)
cases=[('original',1000,RUN/'physical_source_controls_v2/1000_10/problem.json'),
    ('cow_h0',1000,RUN/'funded_cow_pair_30s/days/10/problem_h0.json'),
    ('cow_h2',1000,RUN/'funded_cow_pair_30s/days/10/problem_h2.json'),
    ('goose_h0',1001,RUN/'goose_pair_30s/days/10/problem_h0.json'),
    ('goose_h2',1001,RUN/'goose_pair_30s/days/10/problem_h2.json')]
assert all(p.is_file() for _,_,p in cases)
files=set()
for dep in (RUN/'build').rglob('*.o.d'):
    for entry in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
        p=Path(entry).resolve()
        if p.is_relative_to(ROOT):files.add(p)
def hashes():
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
before=hashes()
jobs=[]
for name,seed,path in cases:
    for mode in [0,1]:
        key=f'{name}_m{mode}'
        command=['conda','run','--no-capture-output','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),
            str(RUN/'build/hinted_solve'),str(path),str(seed),'10','30','4',str(mode),str(out/key)]
        jobs.append({'name':key,'problem':str(path.relative_to(RUN)),
            'problem_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'command':command})
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'sources':before,'binary_sha256':hashlib.sha256((RUN/'build/hinted_solve').read_bytes()).hexdigest(),
    'cases':jobs,'purpose':'Diagnose day10 compilation; no claims of policy economics or agent strength.',
    'change':'Same light-exact tuning2,30seconds,4workers; mode1 supplies soft assignments for unchanged tile tasks.',
    'selection':'Five already exposed day10 fixtures, including the exact known-feasible source control.'},indent=2)+'\n')
def run(job):
    start=time.monotonic()
    with (out/(job['name']+'.log')).open('x') as log:
        result=subprocess.run(job['command'],stdout=log,stderr=subprocess.STDOUT)
    record=dict(job,returncode=result.returncode,seconds=time.monotonic()-start)
    (out/(job['name']+'_EXECUTION.json')).write_text(json.dumps(record,indent=2)+'\n')
    print(job['name'],result.returncode,round(record['seconds'],2),flush=True)
    return record
with ThreadPoolExecutor(max_workers=2) as pool:
    results=list(pool.map(run,jobs))
assert hashes()==before
(out/'EXECUTION.json').write_text(json.dumps({'cases':results,'sources_unchanged':True},indent=2)+'\n')
