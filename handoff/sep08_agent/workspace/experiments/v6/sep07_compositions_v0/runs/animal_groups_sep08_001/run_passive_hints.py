"""Solve corrected final-day contracts with unchanged tasks fixed to their witness."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import time

RUN=Path(__file__).resolve().parent
ROOT=RUN.parents[4]
parser=argparse.ArgumentParser()
parser.add_argument('--extra',type=int,nargs='+',default=[0,2])
parser.add_argument('--output',default='passive_fixed_30s')
args=parser.parse_args()
assert set(args.extra)<=set([0,1,2])
out=RUN/args.output;out.mkdir(exist_ok=False)
protocol=json.loads((RUN/'passive_terminal_contracts/PROTOCOL.json').read_text())
binary=RUN/'build/hinted_solve_v2'
jobs=[]
for row in protocol['cases']:
    if row['extra_hires'] not in args.extra:continue
    name=row['name'];problem=RUN/row['problem']
    command=['conda','run','--no-capture-output','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),
        str(binary),str(problem),str(row['seed']),'29','30','4','2',str(out/name)]
    jobs.append(dict(row,command=command,problem_sha256=hashlib.sha256(problem.read_bytes()).hexdigest()))
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'cases':jobs,
    'scope':'Same restricted fixed-job solver; only false untouched empty/weed end targets corrected. Selected extra-hire counts on all6 matched cases.','extra_hires':args.extra},indent=2)+'\n')
def run(job):
    start=time.monotonic()
    with (out/(job['name']+'.log')).open('x') as log:
        result=subprocess.run(job['command'],stdout=log,stderr=subprocess.STDOUT)
    record=dict(job,returncode=result.returncode,seconds=time.monotonic()-start)
    (out/(job['name']+'_EXECUTION.json')).write_text(json.dumps(record,indent=2)+'\n')
    print(job['name'],result.returncode,round(record['seconds'],2),flush=True)
    return record
with ThreadPoolExecutor(max_workers=2) as pool:records=list(pool.map(run,jobs))
assert all(hashlib.sha256((RUN/j['problem']).read_bytes()).hexdigest()==j['problem_sha256'] for j in jobs)
(out/'EXECUTION.json').write_text(json.dumps({'cases':records,'inputs_unchanged':True},indent=2)+'\n')
