from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime,timezone
import json,subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
cases=[]
for leaf in ['off','on']:
    source=EXP/f'runs/late_animal_rotation_001/goose_c31_d13_{leaf}_002'
    for care in [True,False]:
        name=f'{leaf}_day29_{"fixed" if care else "no_care"}_v3'
        problem=source/'days/29/problem_h0.json' if care else RUN/f'{leaf}_terminal_no_care.json'
        cases.append((name,problem,source/'days/29/actions.txt',30))
for day in [22,23]:
    source=EXP/'runs/late_animal_rotation_001/goose_c31_d13_off_002'
    cases.append((f'off_day{day}_fixed120',source/f'days/{day}/problem_h0.json',source/f'days/{day}/actions.txt',120))
def run(case):
    name,problem,actions,seconds=case
    command=['conda','run','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),str(RUN/'build/retry'),str(problem),str(actions),str(RUN/name),str(seconds)]
    spec={'command':command,'started_utc':datetime.now(timezone.utc).isoformat()}
    (RUN/(name+'.process.json')).write_text(json.dumps(spec,indent=2)+'\n')
    with (RUN/(name+'.log')).open('w') as log:
        process=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
    spec.update(returncode=process.returncode,completed_utc=datetime.now(timezone.utc).isoformat())
    (RUN/(name+'.process.json')).write_text(json.dumps(spec,indent=2)+'\n')
    print(name,process.returncode,json.loads((RUN/name/'STATUS.json').read_text()),flush=True)
    return spec
with ThreadPoolExecutor(max_workers=2) as pool:
    results=list(pool.map(run,cases))
(RUN/'SECOND_PASS_RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
