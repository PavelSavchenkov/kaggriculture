from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
baseline='service_bank_p362_m2'
candidates=['early_melon_b98_m1','early_melon_b98_m2']
actors=[baseline,'cold_renewal_p98',*candidates]
opponents=['empty_sale_slots_m2','teammate_shoprouter','public_router','public_router_v52','joint_routes_p362_m0',baseline,'cold_renewal_p98','pass']
assert json.loads((RUN/'OPERATIONAL_CHECKS.json').read_text())['generic_pair_debug_thread_zero_budget_full_records_equal']
protocol={'created_utc':datetime.now(timezone.utc).isoformat(),'baseline':baseline,'candidates':candidates,'opponents':opponents,
    'seed_start':2500000,'seeds':128,'seat_mode':'both','games':8192,
    'scope':'Selection of a stronger early-melon composition branch only. Global reference remains unchanged unless separately challenged through its complete promotion process.',
    'gates':['Direct retained cold-parent utility >=0.5 and mean margin >0.',
        'Equal-opponent active-field utility gain seed-cluster95% lower bound >0.',
        'Equal-opponent active-field margin gain seed-cluster95% lower bound >0.',
        'No active opponent mean-margin regression greater than500 or utility regression greater than0.02.',
        'All PASS games finish above PASS cash. Report PASS cash differences but do not optimize tournament selection for PASS profit.',
        'Original operational checks plus generic/pair/debug full-record equality for both finalists; original frozen source hashes unchanged.'],
    'tie_break':'Among passing candidates, choose larger active-field utility, then larger active-field mean margin. Retain the other distinct branch with its evidence.',
    'discovery':'Seeds1000..1007 are exposed. New pool2500000 was searched in existing scripts/protocols and not present. Final900000 untouched.'}
with (RUN/'FRESH_PROTOCOL.json').open('x') as f:json.dump(protocol,f,indent=2)
roots=[EXP/'runs',EXP/'league',EXP/'include',ROOT/'fast_game_engine',ROOT/'agents/common/api',ROOT/'agents/common/runtime']
inputs={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for root in roots for p in root.rglob('*') if p.is_file() and p.suffix in {'.cpp','.hpp','.inc','.h'}}
(RUN/'FRESH_INPUTS.json').write_text(json.dumps(inputs,indent=2)+'\n')


def build(job):
    label,args=job
    command=['conda','run','-n','kaggriculture','python',str(EXP/'scripts/build_arena.py'),*args]
    result=subprocess.run(command,capture_output=True,text=True)
    (RUN/f'fresh_build_{label}.log').write_text(result.stdout+result.stderr);result.check_returncode()
    return label,{'command':command,'binary':result.stdout.strip().splitlines()[-1]}


with ThreadPoolExecutor(max_workers=2) as pool:
    binaries=dict(pool.map(build,[('generic',['--agents',*dict.fromkeys(actors+opponents)]),('pair',['--pair',*candidates])]))
binaries['debug']=json.loads((RUN/'OPERATIONAL_BINARIES.json').read_text())['debug']
(RUN/'FRESH_BINARIES.json').write_text(json.dumps(binaries,indent=2)+'\n')
out=RUN/'fresh';out.mkdir(exist_ok=False)
ops=RUN/'finalist_operations';ops.mkdir(exist_ok=False)
op_commands=[]
records=[]
for mode in ['generic','pair','debug']:
    path=ops/f'{mode}.json'
    command=['conda','run','-n','kaggriculture',binaries[mode]['binary'],'--a',candidates[0],'--b',candidates[1],
        '--games','4','--seed-start','1000','--seat-mode','both','--threads','4','--budget-expansions','100000','--validate','--profile','--output',str(path)]
    with path.with_suffix('.log').open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    op_commands.append(command);records.append(json.loads(path.read_text())['games'])
assert records[0]==records[1]==records[2]
(RUN/'FINALIST_OPERATIONS.json').write_text(json.dumps({'games':24,'generic_pair_debug_full_records_equal':True,'commands':op_commands},indent=2)+'\n')


def run(job):
    a,b=job;path=out/f'{a}_vs_{b}.json'
    command=['conda','run','-n','kaggriculture',binaries['generic']['binary'],'--a',a,'--b',b,
        '--games','128','--seed-start','2500000','--seat-mode','both','--threads','4','--budget-expansions','100000','--validate','--profile','--output',str(path)]
    with path.with_suffix('.log').open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    d=json.loads(path.read_text());print(a,b,'cash',d['mean_cash'],'margin',d['mean_margin'],flush=True);return command


with ThreadPoolExecutor(max_workers=2) as pool:
    commands=list(pool.map(run,[(a,b) for b in opponents for a in actors]))
for path,value in inputs.items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==value,path
(RUN/'FRESH_COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
print('8192 fresh early-melon composition games complete; frozen sources unchanged.',flush=True)
