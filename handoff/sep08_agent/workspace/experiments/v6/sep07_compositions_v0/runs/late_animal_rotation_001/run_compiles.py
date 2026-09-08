from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
cases=[('goose_c31_d13_off_002','off',9),('goose_c31_d13_on_002','on',9),
       ('control_c31_d13_off_002','off',-1),('control_c31_d13_on_002','on',-1)]
freeze=RUN/'compile_v2_sources'
freeze.mkdir()
for path in (RUN/'source').glob('*.hpp'):
    (freeze/path.name).write_bytes(path.read_bytes())
(freeze/'compile.cpp').write_bytes((RUN/'source/compile.cpp').read_bytes())
record={'created_utc':datetime.now(timezone.utc).isoformat(),
        'source_sha256':{str(p.relative_to(RUN)):hashlib.sha256(p.read_bytes()).hexdigest() for p in freeze.iterdir()},
        'cases':[]}
for name,leaf,item in cases:
    command=['conda','run','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),str(RUN/f'build/compile_{leaf}'),str(RUN/name),'1000','31','13',str(item),'3']
    record['cases'].append({'name':name,'command':command})
(RUN/'COMPILE_V2.json').write_text(json.dumps(record,indent=2)+'\n')
def run(case):
    with (RUN/(case['name']+'.log')).open('w') as log:
        completed=subprocess.run(case['command'],stdout=log,stderr=subprocess.STDOUT)
    result={**case,'returncode':completed.returncode,'completed_utc':datetime.now(timezone.utc).isoformat()}
    (RUN/(case['name']+'.process.json')).write_text(json.dumps(result,indent=2)+'\n')
    print(case['name'],completed.returncode,flush=True)
    return result
with ThreadPoolExecutor(max_workers=2) as pool:
    results=list(pool.map(run,record['cases']))
(RUN/'COMPILE_V2_RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
