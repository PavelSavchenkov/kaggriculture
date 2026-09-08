from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
variants = json.loads((RUN/'LINEAGE.json').read_text())['variants']
actors = [*variants,'empty_sale_slots_m2']
opponents = ['empty_sale_slots_m2','teammate_shoprouter','public_router',
    'public_router_v52','ahmed_v23','junghoon_78','john_131','king_rc4','pass']
command = ['conda','run','-n','kaggriculture','python',str(EXP/'scripts/build_arena.py'),
    '--agents',*dict.fromkeys(actors+opponents)]
result = subprocess.run(command,capture_output=True,text=True)
(RUN/'build.log').write_text(result.stdout+result.stderr)
result.check_returncode()
binary = result.stdout.strip().splitlines()[-1]
(RUN/'BUILD.json').write_text(json.dumps({'command':command,'binary':binary},indent=2)+'\n')
out = RUN/'discovery'
out.mkdir(exist_ok=False)


def run(job):
    name,opponent = job
    path = out/f'{name}_vs_{opponent}.json'
    command = ['conda','run','-n','kaggriculture',binary,'--a',name,'--b',opponent,
        '--games','64','--seed-start','1000','--seat-mode','both','--threads','4',
        '--budget-expansions','100000','--validate','--profile','--output',str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    report = json.loads(path.read_text())
    print(name,opponent,'cash',report['mean_cash'],'margin',report['mean_margin'],flush=True)
    return command


with ThreadPoolExecutor(max_workers=2) as pool:
    commands = list(pool.map(run,[(n,o) for o in opponents for n in actors]))
(RUN/'COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
for opponent in opponents:
    get = lambda name:json.loads((out/f'{name}_vs_{opponent}.json').read_text())['games']
    assert get('crop_context_m0') == get('empty_sale_slots_m2'),opponent
(RUN/'CONTROL_PARITY.json').write_text(json.dumps({'games':1152,'all_complete_records_equal':True},indent=2)+'\n')
print('4608 crop-context discovery games complete; 1152 exact current controls.',flush=True)
