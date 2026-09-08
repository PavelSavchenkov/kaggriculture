"""Test mixed entries with explicit removal of young crops when the date changes."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime,timezone
from pathlib import Path
import hashlib
import json
import shlex
import subprocess
import time

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
old=RUN/'broader_land_30s'
previous=json.loads((old/'EXECUTION.json').read_text());assert previous['sources_unchanged']
out=RUN/'flexible_mixed_30s';out.mkdir(exist_ok=False)
specs=[(1008,'mixed_d9','5:9:9;7:9:11;8:9:11',None),(1008,'mixed_d10','5:10:9;7:10:11;8:10:11',None)]
files={Path(__file__)}
for dep in (RUN/'build').rglob('*.o.d'):
    for entry in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
        p=Path(entry).resolve()
        if p.is_relative_to(ROOT):files.add(p)
def hashes():return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
before=hashes();jobs=[]
for seed,name,rotations,resume in specs:
    spec=out/(name+'.txt');spec.write_text('\n'.join(r.replace(':',' ') for r in rotations.split(';'))+'\n')
    command=['conda','run','--no-capture-output','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),
        str(RUN/'build/compile_flexible'),str(out/name),str(seed),str(spec),'30']
    prefix={}
    if resume is not None:
        command.append(str(resume));prefix={str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in resume.glob('days/*/actions.txt')}
        assert prefix
    jobs.append({'name':name,'seed':seed,'rotations':rotations,'command':command,'prefix_sha256':prefix})
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),'cases':jobs,'source_sha256':before,
    'binary_sha256':hashlib.sha256((RUN/'build/compile_flexible').read_bytes()).hexdigest(),
    'selection':'After failed day8funding, release replanted wheat explicitly onday9/day10. Price actual discarded crop course and schedule removal; do not reuse day8valuation.',
    'scope':'Fixed-world dated composition and compiler search; no online agent/promotion.'},indent=2)+'\n')
def run(job):
    start=time.monotonic()
    with (out/(job['name']+'.log')).open('x') as log:result=subprocess.run(job['command'],stdout=log,stderr=subprocess.STDOUT)
    record=dict(job,returncode=result.returncode,seconds=time.monotonic()-start)
    (out/(job['name']+'_EXECUTION.json')).write_text(json.dumps(record,indent=2)+'\n')
    print(job['name'],record['returncode'],round(record['seconds'],2),flush=True);return record
with ThreadPoolExecutor(max_workers=2) as pool:records=list(pool.map(run,jobs))
assert hashes()==before
(out/'EXECUTION.json').write_text(json.dumps({'cases':records,'sources_unchanged':True},indent=2)+'\n')
