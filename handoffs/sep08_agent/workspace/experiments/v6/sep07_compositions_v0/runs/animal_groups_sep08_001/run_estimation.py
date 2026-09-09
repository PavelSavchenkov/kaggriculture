"""Freeze the compiled dependency closure and run a bounded retrospective screen."""
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
parser.add_argument('--seed-start', type=int, default=1000)
parser.add_argument('--seeds', type=int, default=2)
parser.add_argument('--output', default='estimates_1000')
parser.add_argument('--binary', default='estimate')
args = parser.parse_args()
output = RUN/args.output
assert not output.exists()
binary = RUN/'build'/args.binary
assert binary.is_file()
files = set()
for dep in (RUN/'build').rglob('*.o.d'):
    paths = shlex.split(dep.read_text().replace('\\\n', ' ').split(':', 1)[1])
    for entry in paths:
        path = Path(entry).resolve()
        if path.is_relative_to(ROOT):
            files.add(path)
assert RUN/'source/estimate.cpp' in files
assert RUN/'source/compile.cpp' in files
def hashes():
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
before = hashes()
command = ['conda','run','--no-capture-output','-n','kaggriculture',
    str(ROOT/'day_solver/with_runtime.sh'),str(binary),str(output),str(args.seed_start),str(args.seeds)]
record = {'created_utc':datetime.now(timezone.utc).isoformat(),'command':command,
    'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'sources':before,
    'scope':'Offline fixed-calendar comparison against public_router, seat0; not a deployable-agent or promotion test.'}
protocol = RUN/(args.output+'_PROTOCOL.json')
with protocol.open('x') as f:
    json.dump(record,f,indent=2)
begin = time.monotonic()
with (RUN/(args.output+'.log')).open('x') as log:
    result = subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
record.update(returncode=result.returncode,seconds=time.monotonic()-begin,sources_unchanged=hashes()==before)
(RUN/(args.output+'_EXECUTION.json')).write_text(json.dumps(record,indent=2)+'\n')
print(json.dumps({k:record[k] for k in ['returncode','seconds','sources_unchanged']}),flush=True)
assert result.returncode == 0 and record['sources_unchanged']
