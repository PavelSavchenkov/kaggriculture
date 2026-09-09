"""Compile second observed continuation contexts with the unchanged v8 solver."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import shlex
import subprocess
import time

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
out=RUN/'contexts_30s'
out.mkdir(exist_ok=False)
commands=[['python',str(RUN/'prepare_contexts.py')],
    ['cmake','-S',str(RUN),'-B',str(RUN/'build'),'-DCMAKE_BUILD_TYPE=Release'],
    ['cmake','--build',str(RUN/'build'),'--target','compile_public','compile_pass','inspect_context','-j','3']]
with (out/'build.log').open('x') as log:
    for cmd in commands:subprocess.run(['conda','run','-n','kaggriculture',*cmd],stdout=log,stderr=subprocess.STDOUT,check=True)
with (out/'CONTEXT_DIFFERENCES.txt').open('x') as log:
    subprocess.run(['conda','run','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),str(RUN/'build/inspect_context')],stdout=log,stderr=subprocess.STDOUT,check=True)
paths=set()
for dep in (RUN/'build').rglob('*.o.d'):
    for entry in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
        path=Path(entry).resolve()
        if path.is_relative_to(ROOT):paths.add(path)
frozen={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
specs=[('sheep_3_berry',1048,'pass','3 20 11'),('sheep_12_berry',1048,'pass','12 20 11'),
    ('sheep_13_berry',1048,'pass','13 20 11'),('sheep_triple_berry',1048,'pass','3 20 11\n12 20 11\n13 20 11'),
    ('cow_9_alternate',1019,'public','9 15 10'),('cow_triple_alternate',1019,'public','9 15 10\n90 15 10\n94 15 10')]
jobs=[]
for name,seed,rival,spec in specs:
    path=out/(name+'.txt');path.write_text(spec+'\n')
    cmd=['conda','run','--no-capture-output','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),
        str(RUN/f'build/compile_{rival}'),str(out/name),str(seed),str(path),'30']
    jobs.append({'name':name,'seed':seed,'opponent':rival,'command':cmd})
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'jobs':jobs,'sources_sha256':frozen,'build_commands':commands,
    'scope':'Exposed context construction; no runtime selection, cross-game reuse, or league promotion.'},indent=2)+'\n')
def run(job):
    start=time.monotonic()
    with (out/(job['name']+'.log')).open('x') as log:
        result=subprocess.run(job['command'],stdout=log,stderr=subprocess.STDOUT)
    record=dict(job,returncode=result.returncode,seconds=time.monotonic()-start)
    (out/(job['name']+'_EXECUTION.json')).write_text(json.dumps(record,indent=2)+'\n')
    print(job['name'],result.returncode,round(record['seconds'],2),flush=True)
    return record
with ThreadPoolExecutor(max_workers=2) as pool:records=list(pool.map(run,jobs))
assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==h for p,h in frozen.items())
(out/'EXECUTION.json').write_text(json.dumps({'cases':records,'sources_unchanged':True},indent=2)+'\n')
