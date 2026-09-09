"""Test each solved final-day suffix inside the complete matched game."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import argparse
import hashlib
import json
import shlex
import subprocess

RUN=Path(__file__).resolve().parent
ROOT=RUN.parents[4]
parser=argparse.ArgumentParser()
parser.add_argument('--solves',default='passive_fixed_30s')
parser.add_argument('--extra',type=int,default=2)
parser.add_argument('--output',default='matched_h2')
args=parser.parse_args()
solves=RUN/args.solves
execution=json.loads((solves/'EXECUTION.json').read_text())
assert execution['inputs_unchanged']
out=RUN/args.output;out.mkdir(exist_ok=False)
binary=RUN/'build/final_audit'
files=set()
for dep in (RUN/'build/CMakeFiles/final_audit.dir').rglob('*.o.d'):
    for entry in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
        p=Path(entry).resolve()
        if p.is_relative_to(ROOT):files.add(p)
def hashes():
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
before=hashes();jobs=[]
assert before
for row in execution['cases']:
    if row['extra_hires']!=args.extra:continue
    status=json.loads((solves/row['name']/'STATUS.json').read_text())
    if not status['schedule']:continue
    assert status['requirements'] and status['invariants'] and status['clean']
    command=['conda','run','--no-capture-output','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),
        str(binary),str(RUN/row['prefix']),str(solves/row['name']/'actions.txt'),
        str(RUN/row['problem']),str(RUN/row['orders']),str(row['seed']),str(out/row['agent_case'])]
    jobs.append(dict(row,command=command))
assert jobs
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'cases':jobs,'sources':before,'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),
    'scope':'719 real transitions, all day endpoints checked, full live opponent. Fixed known calendar remains retrospective; no runtime generalization claim.'},indent=2)+'\n')
def run(job):
    with (out/(job['agent_case']+'.log')).open('x') as log:
        result=subprocess.run(job['command'],stdout=log,stderr=subprocess.STDOUT)
    print(job['agent_case'],result.returncode,flush=True)
    return dict(job,returncode=result.returncode)
with ThreadPoolExecutor(max_workers=2) as pool:records=list(pool.map(run,jobs))
assert hashes()==before
(out/'EXECUTION.json').write_text(json.dumps({'cases':records,'sources_unchanged':True},indent=2)+'\n')
