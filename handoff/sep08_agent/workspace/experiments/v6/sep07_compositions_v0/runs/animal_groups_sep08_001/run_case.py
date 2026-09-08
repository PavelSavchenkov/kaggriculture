"""Run one additional compile attempt without overwriting an earlier result."""
from pathlib import Path
from datetime import datetime, timezone
import argparse
import hashlib
import json
import shlex
import subprocess
import time

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]
parser = argparse.ArgumentParser()
parser.add_argument('--name',required=True)
parser.add_argument('--output',required=True)
parser.add_argument('--seconds',type=float,default=30)
parser.add_argument('--binary',default='compile_v2')
args = parser.parse_args()
previous=json.loads((RUN/'fixtures_3s/PROTOCOL.json').read_text())
case=next(c for c in previous['cases'] if c['name']==args.name)
assert not (RUN/args.output).exists()
files=set()
for dep in (RUN/'build').rglob('*.o.d'):
    for name in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
        path=Path(name).resolve()
        if path.is_relative_to(ROOT):files.add(path)
def hashes():
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
binary=RUN/'build'/args.binary
command=['conda','run','--no-capture-output','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),
    str(binary),str(RUN/args.output),str(case['seed']),str(RUN/'fixtures_3s'/(args.name+'.txt')),str(args.seconds)]
record={'created_utc':datetime.now(timezone.utc).isoformat(),'case':case,'command':command,
    'sources':hashes(),'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest()}
with (RUN/(args.output+'_PROTOCOL.json')).open('x') as f:json.dump(record,f,indent=2)
start=time.monotonic()
with (RUN/(args.output+'.log')).open('x') as log:
    result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
record.update(returncode=result.returncode,seconds=time.monotonic()-start,sources_unchanged=hashes()==record['sources'])
(RUN/(args.output+'_EXECUTION.json')).write_text(json.dumps(record,indent=2)+'\n')
assert record['sources_unchanged']
print(args.name,{k:record[k] for k in ['returncode','seconds','sources_unchanged']},flush=True)
