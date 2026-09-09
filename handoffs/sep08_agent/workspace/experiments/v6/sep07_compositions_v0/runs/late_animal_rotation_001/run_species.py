from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import json
import subprocess

RUN=Path(__file__).resolve().parent
ROOT=RUN.parents[4]
cases=[]
for species,item in [('cow',10),('sheep',11)]:
    for leaf in ['off','on']:
        name=f'{species}_c31_d13_{leaf}_002'
        cases.append({'name':name,'command':['conda','run','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),str(RUN/f'build/compile_{leaf}'),str(RUN/name),'1000','31','13',str(item),'3']})
(RUN/'SPECIES_COMMANDS.json').write_text(json.dumps(cases,indent=2)+'\n')
def run(case):
    with (RUN/(case['name']+'.log')).open('w') as log:
        result=subprocess.run(case['command'],stdout=log,stderr=subprocess.STDOUT)
    record={**case,'returncode':result.returncode,'completed_utc':datetime.now(timezone.utc).isoformat()}
    (RUN/(case['name']+'.process.json')).write_text(json.dumps(record,indent=2)+'\n')
    print(case['name'],result.returncode,flush=True)
    return record
with ThreadPoolExecutor(max_workers=2) as pool:
    results=list(pool.map(run,cases))
(RUN/'SPECIES_RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
