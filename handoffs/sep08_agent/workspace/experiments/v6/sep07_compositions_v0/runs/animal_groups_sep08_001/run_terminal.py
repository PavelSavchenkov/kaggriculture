"""Resume the six selected fixed worlds from revalidated completed day schedules."""
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
old=RUN/'later_groups_30s'
previous=json.loads((old/'EXECUTION.json').read_text())
assert previous['sources_unchanged'] and len(previous['cases'])==6
out=RUN/'terminal_groups_30s';out.mkdir(exist_ok=False)
files=set()
for dep in (RUN/'build').rglob('*.o.d'):
    for entry in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
        path=Path(entry).resolve()
        if path.is_relative_to(ROOT):files.add(path)
def hashes():
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
before=hashes();jobs=[]
for original in previous['cases']:
    name=original['name'];resume=old/name
    command=list(original['command']);command[-5]=str(RUN/'build/compile_v5');command[-4]=str(out/name);command.append(str(resume))
    prefixes=sorted((resume/'days').glob('*/actions.txt'))
    assert prefixes
    jobs.append({'name':name,'seed':original['seed'],'command':command,'estimate':original['estimate'],
        'prefix_files':{str(p.relative_to(RUN)):hashlib.sha256(p.read_bytes()).hexdigest() for p in prefixes}})
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'cases':jobs,'sources':before,'binary_sha256':hashlib.sha256((RUN/'build/compile_v5').read_bytes()).hexdigest(),
    'changes':'Correct non-input continuation stocks and omit changed animals final fertilizer pickup. Reuse only completed pre-final days, with full live-opponent endpoint revalidation.',
    'scope':'Same six exposed matched worlds. No league or runtime-policy claims.'},indent=2)+'\n')
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
