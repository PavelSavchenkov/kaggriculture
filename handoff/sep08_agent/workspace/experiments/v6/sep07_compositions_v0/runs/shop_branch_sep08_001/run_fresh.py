from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import re
import shlex
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
candidate = 'shop_branch_m2'
baseline = 'service_bank_p362_m2'
incumbent = 'early_melon_b98_m1'
actors = [baseline,incumbent,candidate]
opponents = ['empty_sale_slots_m2','teammate_shoprouter','public_router','public_router_v52',
    'joint_routes_p362_m0',baseline,'cold_renewal_p98',incumbent,'pass']
operations = json.loads((RUN/'OPERATIONAL_CHECKS.json').read_text())
assert operations['generic_pair_debug_thread_full_records_equal']
assert operations['zero_budget_generic_pair_debug_thread_full_records_equal']
assert json.loads((RUN/'PARITY.json').read_text())['all_complete_records_equal']
protocol = {'created_utc':datetime.now(timezone.utc).isoformat(),'candidate':candidate,
    'baseline':baseline,'current_cold':incumbent,'actors':actors,'opponents':opponents,
    'seed_start':2600000,'seeds':128,'native_seed_start':2604000,'native_seeds':16,
    'seat_mode':'both','fresh_games':6912,'native_games':864,
    'gates':['For each comparison reference, direct utility>=0.5 and mean margin>0.',
        'Active-field equal-opponent utility and margin gains have positive seed-cluster 95% lower bounds.',
        'Every active opponent utility gain>=-0.02 and margin gain>=-500.',
        'Every PASS game is won, in both shop modes.',
        'Native active-field mean utility gain>=-0.02 and mean margin gain>=-500.',
        'Operational and complete constituent parity checks pass; source hashes remain frozen.'],
    'selection':'Select as current cold reference only if all gates versus early_melon_b98_m1 pass. Report independent adaptivity evidence versus old service-bank separately. Global reference unchanged.',
    'scope':'Faithful prior fitted rule; no new fit or early-melon substitution. Prior2400000 games are exposed;2600000 and2604000 are new audit pools; final900000 untouched.'}
with (RUN/'FRESH_PROTOCOL.json').open('x') as output:json.dump(protocol,output,indent=2)
registry = json.loads((EXP/'configs/league.json').read_text())
units = {EXP/'include/evaluation.hpp'}
for name in set(actors+opponents)-{'pass'}:
    directory = ROOT/registry[name]
    manifest = json.loads((directory/'agent.json').read_text())
    units.add((directory/manifest['header']).resolve())
    units.update((directory/p).resolve() for p in manifest['sources'])
dependency_command = ['conda','run','-n','kaggriculture','g++','-std=c++20','-I',str(ROOT),
    '-MM','-MT','closure',*[str(p) for p in sorted(units)]]
result = subprocess.run(dependency_command,capture_output=True,text=True,check=True)
dependencies = {Path(p).resolve() for p in shlex.split(re.sub(r'(?m)^closure:\s*','',result.stdout.replace('\\\n',' ')))}
dependencies.update([EXP/'src/arena.cpp',EXP/'scripts/build_arena.py',RUN/'selector.hpp'])
assert all(p.is_relative_to(ROOT) for p in dependencies)
inputs = {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(dependencies)}
(RUN/'FRESH_INPUTS.json').write_text(json.dumps(inputs,indent=2)+'\n')
(RUN/'DEPENDENCY_COMMAND.json').write_text(json.dumps(dependency_command,indent=2)+'\n')
command = ['conda','run','-n','kaggriculture','python',str(EXP/'scripts/build_arena.py'),
    '--agents',*dict.fromkeys(actors+opponents)]
result = subprocess.run(command,capture_output=True,text=True)
(RUN/'fresh_build.log').write_text(result.stdout+result.stderr)
result.check_returncode()
binary = result.stdout.strip().splitlines()[-1]
(RUN/'FRESH_BUILD.json').write_text(json.dumps({'command':command,'binary':binary},indent=2)+'\n')
for folder in ['fresh','native']:(RUN/folder).mkdir(exist_ok=False)


def run(job):
    a,b,native = job
    path = RUN/('native' if native else 'fresh')/f'{a}_vs_{b}.json'
    command = ['conda','run','-n','kaggriculture',binary,'--a',a,'--b',b,
        '--games',str(16 if native else 128),'--seed-start',str(2604000 if native else 2600000),
        '--seat-mode','both','--threads','4','--budget-expansions','100000','--validate','--profile','--output',str(path)]
    if native:command.append('--native-shops')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    data = json.loads(path.read_text())
    print(a,b,'native',native,'cash',data['mean_cash'],'margin',data['mean_margin'],flush=True)
    return command


with ThreadPoolExecutor(max_workers=2) as pool:
    commands = list(pool.map(run,[(a,b,n) for n in range(2) for b in opponents for a in actors]))
for path,value in inputs.items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==value,path
(RUN/'FRESH_COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
print('7776 shop-selector audit games complete; actual source dependencies unchanged.',flush=True)
