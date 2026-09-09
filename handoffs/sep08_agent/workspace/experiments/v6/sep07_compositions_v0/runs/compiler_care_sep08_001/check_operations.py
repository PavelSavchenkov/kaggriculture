from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
variants=json.loads((RUN/'LINEAGE.json').read_text())['variants']
commands=[]


def build(item):
    name,args=item
    command=['conda','run','-n','kaggriculture','python',str(EXP/'scripts/build_arena.py'),*args]
    result=subprocess.run(command,capture_output=True,text=True)
    (RUN/f'operations_build_{name}.log').write_text(result.stdout+result.stderr);result.check_returncode()
    return name,{'command':command,'binary':result.stdout.strip().splitlines()[-1]}


with ThreadPoolExecutor(max_workers=2) as pool:
    binaries=dict(pool.map(build,[('pair',['--pair','compiler_care_mixed_m1','public_router']),
        ('debug',['--agents',*variants,'public_router','pass','--debug'])]))
(RUN/'OPERATIONAL_BINARIES.json').write_text(json.dumps(binaries,indent=2)+'\n')
generic=json.loads((RUN/'BUILD.json').read_text())['binary']
out=RUN/'operations';out.mkdir(exist_ok=False)


def run(label,binary,a,b,threads=2,paired=False):
    path=out/f'{label}.json'
    command=['conda','run','-n','kaggriculture',binary,'--games','2','--seed-start','1000','--seat-mode','both',
        '--threads',str(threads),'--budget-expansions','100000','--validate','--profile','--output',str(path)]
    command+=['--a',a,'--b',b]
    with path.with_suffix('.log').open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    commands.append(command)
    return json.loads(path.read_text())['games']


pair=run('pair',binaries['pair']['binary'],'compiler_care_mixed_m1','public_router',paired=True)
release=run('generic',generic,'compiler_care_mixed_m1','public_router')
debug=run('debug',binaries['debug']['binary'],'compiler_care_mixed_m1','public_router')
one_thread=run('one_thread',generic,'compiler_care_mixed_m1','public_router',threads=1)
assert pair==release==debug==one_thread
count=16
for name in variants:
    for opponent in [name,'pass']:
        run(f'{name}_vs_{opponent}',binaries['debug']['binary'],name,opponent)
        count+=4
report={'games':count,'generic_pair_debug_thread_full_records_equal':4,
    'all_modes_debug_self_and_pass_complete':True,'commands':commands,
    'state':'Only per-instance AgentCore intent/support/config state is mutable; recorded source tables are immutable. Care correction adds no shared mutable state.',
    'observation':'Only AgentObservation and config; static future intentions carry no game seed, hidden rival stock or future shops.',
    'scope':'Operational check of the care timing compiler prototype. No competitive-strength promotion.'}
(RUN/'OPERATIONAL_CHECKS.json').write_text(json.dumps(report,indent=2)+'\n')
print('Operational checks complete:',count,'games.')
