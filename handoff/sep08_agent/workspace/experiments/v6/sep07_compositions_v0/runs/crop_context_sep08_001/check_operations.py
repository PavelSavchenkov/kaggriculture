from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
variants = json.loads((RUN/'LINEAGE.json').read_text())['variants']


def build(job):
    name, args = job
    command = ['conda','run','-n','kaggriculture','python',str(EXP/'scripts/build_arena.py'),*args]
    result = subprocess.run(command,capture_output=True,text=True)
    (RUN/f'operations_build_{name}.log').write_text(result.stdout+result.stderr)
    result.check_returncode()
    return name, {'command':command,'binary':result.stdout.strip().splitlines()[-1]}


with ThreadPoolExecutor(max_workers=2) as pool:
    binaries = dict(pool.map(build,[('pair',['--pair','crop_context_m2','crop_context_m1']),
        ('debug',['--agents',*variants,'crop_context_m1','pass','--debug'])]))
binaries['generic'] = json.loads((RUN/'BUILD.json').read_text())
(RUN/'OPERATIONAL_BINARIES.json').write_text(json.dumps(binaries,indent=2)+'\n')
out = RUN/'operations'
out.mkdir(exist_ok=False)
commands = []


def run(label, build, a, b, threads=2, budget=100000):
    path = out/f'{label}.json'
    command = ['conda','run','-n','kaggriculture',binaries[build]['binary'],'--a',a,'--b',b,
        '--games','2','--seed-start','1000','--seat-mode','both','--threads',str(threads),
        '--budget-expansions',str(budget),'--validate','--profile','--output',str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    commands.append(command)
    return json.loads(path.read_text())['games']


records = [run(b,b,'crop_context_m2','crop_context_m1') for b in ['pair','generic','debug']]
records.append(run('one_thread','generic','crop_context_m2','crop_context_m1',threads=1))
assert all(r==records[0] for r in records)
zero = [run('zero_'+b,b,'crop_context_m2','crop_context_m1',budget=0) for b in ['pair','generic','debug']]
assert zero[0]==zero[1]==zero[2]
for name in variants:
    for opponent in [name,'pass']:
        run(f'{name}_vs_{opponent}','debug',name,opponent)
report = {'games':len(commands)*4,'generic_pair_debug_thread_same_budget_full_records_equal':True,
    'all_variants_full_debug_self_and_pass':True,'commands':commands,
    'state':'Policy state is per-instance and reset. Crop mode is a fixed per-instance parameter; inherited day tables are immutable.',
    'observation':'Only AgentObservation and public configuration; no seed or rival-private state.',
    'budget':'Compare each budget separately across runner modes; zero budget returns complete legal actions.'}
(RUN/'OPERATIONAL_CHECKS.json').write_text(json.dumps(report,indent=2)+'\n')
print('Mainline crop operational games complete; all required comparisons exact.',flush=True)
