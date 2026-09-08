"""Compile promising later groups and single-tile controls after lifecycle correction."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import csv
import hashlib
import json
import shlex
import subprocess
import time

RUN=Path(__file__).resolve().parent
ROOT=RUN.parents[4]
out=RUN/'later_groups_30s'
out.mkdir(exist_ok=False)
specifications=[(1016,'sheep_3','3:20:11'),(1016,'sheep_12','12:20:11'),(1016,'sheep_13','13:20:11'),
    (1016,'sheep_triple','3:20:11;12:20:11;13:20:11'),
    (1014,'cow_9','9:15:10'),(1014,'cow_triple','9:15:10;90:15:10;94:15:10')]
wanted={(s,r):n for s,n,r in specifications};estimates={}
with (RUN/'estimates_corrected_1000/proposals.csv').open() as file:
    for row in csv.DictReader(file):
        key=int(row['seed']),row['rotations']
        if key in wanted:estimates[wanted[key]]=row
assert len(estimates)==len(specifications)
files=set()
for dep in (RUN/'build').rglob('*.o.d'):
    for entry in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
        path=Path(entry).resolve()
        if path.is_relative_to(ROOT):files.add(path)
def hashes():
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
before=hashes();jobs=[]
for seed,name,rotations in specifications:
    spec=out/(name+'.txt');spec.write_text('\n'.join(p.replace(':',' ') for p in rotations.split(';'))+'\n')
    command=['conda','run','--no-capture-output','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),
        str(RUN/'build/compile_v4'),str(out/name),str(seed),str(spec),'30']
    jobs.append({'name':name,'seed':seed,'estimate':estimates[name],'command':command})
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'cases':jobs,'sources':before,'binary_sha256':hashlib.sha256((RUN/'build/compile_v4').read_bytes()).hexdigest(),
    'selection':'Two later high-scoring groups from exposed corrected18-seed screen, with single-tile controls. Early day8/10 cases remain unresolved; this does not replace their investigation.',
    'scope':'Matched fixed-calendar compiler study. No deployable agent, league comparison or promotion.'},indent=2)+'\n')
def run(job):
    start=time.monotonic()
    with (out/(job['name']+'.log')).open('x') as log:
        result=subprocess.run(job['command'],stdout=log,stderr=subprocess.STDOUT)
    record=dict(job,returncode=result.returncode,seconds=time.monotonic()-start)
    (out/(job['name']+'_EXECUTION.json')).write_text(json.dumps(record,indent=2)+'\n')
    print(job['name'],result.returncode,round(record['seconds'],2),flush=True)
    return record
with ThreadPoolExecutor(max_workers=3) as pool:
    results=list(pool.map(run,jobs))
assert hashes()==before
(out/'EXECUTION.json').write_text(json.dumps({'cases':results,'sources_unchanged':True},indent=2)+'\n')
