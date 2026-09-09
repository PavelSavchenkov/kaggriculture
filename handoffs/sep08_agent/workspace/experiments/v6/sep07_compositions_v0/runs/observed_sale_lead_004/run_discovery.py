from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
variants=list(json.loads((RUN/'LINEAGE.json').read_text())['variants'])
opponents=['rival_wool_context_v3','ahmed_v23','public_router_v52','public_router_v5','teammate_shoprouter','public_router','junghoon_wool_sales','john_131','king_rc4','investment_context_guarded_001_best']
command=['conda','run','-n','kaggriculture','python',str(EXP/'scripts/build_arena.py'),'--agents',*sorted(set(variants+opponents))]
result=subprocess.run(command,capture_output=True,text=True)
(RUN/'build.log').write_text(result.stdout+result.stderr);result.check_returncode()
binary=result.stdout.strip().splitlines()[-1]
(RUN/'BUILD.json').write_text(json.dumps({'command':command,'binary':binary},indent=2)+'\n')
output=RUN/'discovery';output.mkdir(exist_ok=False)


def run(job):
    a,b=job;path=output/f'{a}_vs_{b}.json'
    command=['conda','run','-n','kaggriculture',binary,'--a',a,'--b',b,'--games','32','--seed-start','1000','--seat-mode','both','--threads','6','--profile','--validate','--output',str(path)]
    with path.with_suffix('.log').open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    print(a,'vs',b,'complete',flush=True)
    return command


jobs=[(a,b) for b in opponents for a in variants]
with ThreadPoolExecutor(max_workers=3) as pool:commands=list(pool.map(run,jobs))
(RUN/'PROCESSES.json').write_text(json.dumps(commands,indent=2)+'\n')
