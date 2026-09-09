"""Test restricted repair of final-day schedules, retaining an unrestricted control."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import shlex
import subprocess
import time

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]
binary = RUN / 'build/hinted_solve_v2'
out = RUN / 'fixed_hints_30s'
out.mkdir(exist_ok=False)
cases = [('source_1016', 1016, RUN / 'terminal_source_controls/1016_29/problem.json', [0,2])]
for name, seed, parent in [('sheep_3',1016,'terminal_groups_30s'),('sheep_triple',1016,'terminal_groups_30s'),
                          ('cow_9',1014,'market_timing_30s'),('cow_triple',1014,'market_timing_30s')]:
    for extra in (0,2):
        cases.append((f'{name}_h{extra}',seed,RUN/parent/name/'days/29'/f'problem_h{extra}.json',[2]))
files=set()
for dep in (RUN / 'build/CMakeFiles/hinted_solve_v2.dir').rglob('*.o.d'):
    for entry in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
        p=Path(entry).resolve()
        if p.is_relative_to(ROOT):files.add(p)
def hashes():
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
before=hashes();jobs=[]
assert before
for name, seed, path, modes in cases:
    for mode in modes:
        key=f'{name}_m{mode}'
        command=['conda','run','--no-capture-output','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),
            str(binary),str(path),str(seed),'29','30','4',str(mode),str(out/key)]
        jobs.append({'name':key,'problem':str(path.relative_to(RUN)),
            'problem_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'command':command})
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'sources':before,'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'cases':jobs,
    'scope':'Mode2 fixes unchanged original task worker/hour assignments; new tasks remain free. Failure is only for this restriction, not unrestricted infeasibility. Mode0 uses the same30seconds/4threads on a known-feasible original final day.'},indent=2)+'\n')
def run(job):
    start=time.monotonic()
    with (out/(job['name']+'.log')).open('x') as log:
        result=subprocess.run(job['command'],stdout=log,stderr=subprocess.STDOUT)
    record=dict(job,returncode=result.returncode,seconds=time.monotonic()-start)
    (out/(job['name']+'_EXECUTION.json')).write_text(json.dumps(record,indent=2)+'\n')
    print(job['name'],result.returncode,round(record['seconds'],2),flush=True)
    return record
with ThreadPoolExecutor(max_workers=2) as pool:
    records=list(pool.map(run,jobs))
assert hashes()==before
(out/'EXECUTION.json').write_text(json.dumps({'cases':records,'sources_unchanged':True},indent=2)+'\n')
