"""Apply the corrected compiler to earlier cow/goose and mixed-species courses."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime,timezone
from pathlib import Path
import csv
import hashlib
import json
import shlex
import subprocess
import time

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
out=RUN/'broader_courses_30s';out.mkdir(exist_ok=False)
specs=[(1000,'cow_pair_d8','7:8:10;8:8:10'),(1001,'goose_pair_d10','23:10:9;32:10:9')]
estimates={};best=None
with (EXP/'runs/animal_groups_sep08_001/estimates_corrected_1000/proposals.csv').open() as f:
    for row in csv.DictReader(f):
        for seed,name,rotations in specs:
            if int(row['seed'])==seed and row['rotations']==rotations:estimates[name]=row
        parts=[list(map(int,r.split(':'))) for r in row['rotations'].split(';')]
        if len(set(r[2] for r in parts))<2 or max(r[1] for r in parts)>15 or float(row['field_operations_delta'])>250:continue
        if best is None or float(row['risk_score'])>float(best['risk_score']):best=row
assert best is not None and len(estimates)==2
specs.append((int(best['seed']),'mixed_early',best['rotations']));estimates['mixed_early']=best
files={Path(__file__)}
for dep in (RUN/'build').rglob('*.o.d'):
    for entry in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
        p=Path(entry).resolve()
        if p.is_relative_to(ROOT):files.add(p)
def hashes():return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
before=hashes();jobs=[]
for seed,name,rotations in specs:
    spec=out/(name+'.txt');spec.write_text('\n'.join(p.replace(':',' ') for p in rotations.split(';'))+'\n')
    command=['conda','run','--no-capture-output','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),
        str(RUN/'build/compile_public'),str(out/name),str(seed),str(spec),'30']
    jobs.append({'name':name,'seed':seed,'estimate':estimates[name],'command':command})
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),'cases':jobs,'source_sha256':before,
    'binary_sha256':hashlib.sha256((RUN/'build/compile_public').read_bytes()).hexdigest(),
    'selection':'Revisit unresolved early cow/goose pairs with funded sales/passive-weed/fixed-job repairs; add highest corrected risk score among mixed species ending all placements byday15 and addedfieldoperations<=250.',
    'scope':'Exposed fixed-world construction, no deployable agent or promotion. Cheap estimates still omit labor until schedules are solved.'},indent=2)+'\n')
def run(job):
    start=time.monotonic()
    with (out/(job['name']+'.log')).open('x') as log:result=subprocess.run(job['command'],stdout=log,stderr=subprocess.STDOUT)
    record=dict(job,returncode=result.returncode,seconds=time.monotonic()-start)
    (out/(job['name']+'_EXECUTION.json')).write_text(json.dumps(record,indent=2)+'\n')
    print(job['name'],record['returncode'],round(record['seconds'],2),flush=True);return record
with ThreadPoolExecutor(max_workers=2) as pool:records=list(pool.map(run,jobs))
assert hashes()==before
(out/'EXECUTION.json').write_text(json.dumps({'cases':records,'sources_unchanged':True},indent=2)+'\n')
