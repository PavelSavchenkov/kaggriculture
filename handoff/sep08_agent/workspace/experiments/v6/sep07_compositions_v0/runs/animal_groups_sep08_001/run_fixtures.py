"""Compile two animal pairs and their single-animal controls in matched worlds."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import csv
import hashlib
import json
import subprocess
import time

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]
out = RUN/'fixtures_3s'
out.mkdir(exist_ok=False)
rows = list(csv.DictReader((RUN/'estimates_1000/proposals.csv').open()))
cases = [(1000,'cow_7','7:8:10'),(1000,'cow_8','8:8:10'),
    (1000,'cow_pair','7:8:10;8:8:10'),(1001,'goose_23','23:10:9'),
    (1001,'goose_32','32:10:9'),(1001,'goose_pair','23:10:9;32:10:9')]
frozen = json.loads((RUN/'estimates_1000_PROTOCOL.json').read_text())['sources']
def unchanged():
    return all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==digest for p,digest in frozen.items())
assert unchanged()
records = []
for seed,name,rotations in cases:
    estimate = next(r for r in rows if int(r['seed'])==seed and r['rotations']==rotations)
    spec = out/(name+'.txt')
    spec.write_text('\n'.join(r.replace(':',' ') for r in rotations.split(';'))+'\n')
    command = ['conda','run','--no-capture-output','-n','kaggriculture',
        str(ROOT/'day_solver/with_runtime.sh'),str(RUN/'build/compile'),
        str(out/name),str(seed),str(spec),'3']
    records.append({'name':name,'seed':seed,'estimate':estimate,'command':command})
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'scope':'Two matched pair-versus-single cost decompositions, chosen from the initial exposed two-seed screen.',
    'sources':frozen,'binary_sha256':hashlib.sha256((RUN/'build/compile').read_bytes()).hexdigest(),
    'cases':records,'seconds_per_day_worker_attempt':3,'threads':3},indent=2)+'\n')
def run(record):
    start = time.monotonic()
    with (out/(record['name']+'.log')).open('x') as log:
        result = subprocess.run(record['command'],stdout=log,stderr=subprocess.STDOUT)
    record = dict(record,returncode=result.returncode,seconds=time.monotonic()-start)
    (out/(record['name']+'_EXECUTION.json')).write_text(json.dumps(record,indent=2)+'\n')
    print(record['name'],record['returncode'],round(record['seconds'],2),flush=True)
    return record
with ThreadPoolExecutor(max_workers=3) as pool:
    results = list(pool.map(run,records))
assert unchanged()
(out/'EXECUTION.json').write_text(json.dumps({'cases':results,'sources_unchanged':True},indent=2)+'\n')
