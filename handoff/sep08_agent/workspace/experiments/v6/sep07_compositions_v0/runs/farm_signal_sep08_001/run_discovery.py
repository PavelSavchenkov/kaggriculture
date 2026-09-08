from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
assert not json.loads((RUN/'GUARD_PARITY.json').read_text())['errors']
names = ['public_router_v52','farm_signal_v52_m0','farm_signal_v52_m1']
opponents = ['empty_sale_slots_m2','teammate_shoprouter','public_router','public_router_v52','ahmed_v23','pass']
command = ['conda','run','-n','kaggriculture','python',str(EXP/'scripts/build_arena.py'),'--agents',*dict.fromkeys(names+opponents)]
result = subprocess.run(command,capture_output=True,text=True)
(RUN/'build.log').write_text(result.stdout+result.stderr)
result.check_returncode()
binary = result.stdout.strip().splitlines()[-1]
(RUN/'BUILD.json').write_text(json.dumps({'command':command,'binary':binary},indent=2)+'\n')
out = RUN/'discovery'
out.mkdir(exist_ok=False)


def run(job):
    name, opponent = job
    path = out/f'{name}_vs_{opponent}.json'
    command = ['conda','run','-n','kaggriculture',binary,'--a',name,'--b',opponent,
        '--games','32','--seed-start','1000','--seat-mode','both','--threads','4',
        '--budget-expansions','100000','--validate','--profile','--output',str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    data = json.loads(path.read_text())
    print(name,opponent,'cash',data['mean_cash'],'margin',data['mean_margin'],flush=True)
    return command


with ThreadPoolExecutor(max_workers=2) as pool:
    commands = list(pool.map(run,[(n,o) for o in opponents for n in names]))
(RUN/'COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
print('1152 full capacity discovery/control games complete.',flush=True)
