from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
old = EXP/'runs/day_service_bank_sep08_001/fresh'
games = []
inputs = {}
for opponent in ['empty_sale_slots_m2','public_router']:
    for actor in ['service_bank_p362_m0','service_bank_p362_m2']:
        path = old/f'{actor}_vs_{opponent}.json'
        data = json.loads(path.read_text())
        assert len(data['games']) == 256
        inputs[str(path.relative_to(EXP))] = hashlib.sha256(path.read_bytes()).hexdigest()
        games += [{'actor':actor,'opponent':opponent,**g} for g in data['games']]
(RUN/'INPUTS.json').write_text(json.dumps(inputs,indent=2)+'\n')
profiles, keys, scenarios = [], [], []
for g in games:
    keys.append({k:g[k] for k in ['actor','opponent','seed','seat']})
    profiles.append(str(len(g['profile']['lives'])))
    profiles += [' '.join(map(str,l)) for l in g['profile']['lives']]
    rival = g['opponent_profile']
    flows = [f for f in rival['flows'] if f[1]<9]
    fixed = sum(f[1] for f in rival['fixed_costs'])
    scenarios.append(f"{g['seed']} {g['seat']} {g['cash']} 0 0 {len(flows)} {fixed}")
    scenarios.append(' '.join(map(str,g['shops'])))
    scenarios += [' '.join(map(str,f)) for f in flows]
(RUN/'KEYS.json').write_text(json.dumps(keys,indent=2)+'\n')
(RUN/'lifetimes.txt').write_text(str(len(games))+'\n'+'\n'.join(profiles)+'\n')
(RUN/'scenarios.txt').write_text('two_sided_v1\n'+str(len(games))+'\n'+'\n'.join(scenarios)+'\n')
binary = RUN/'diagnose'
command = ['conda','run','-n','kaggriculture','g++','-std=c++20','-O3','-DNDEBUG','-fno-exceptions','-fno-rtti','-I',str(ROOT),str(RUN/'diagnose.cpp'),'-o',str(binary)]
result = subprocess.run(command,capture_output=True,text=True)
(RUN/'build.log').write_text(result.stdout+result.stderr)
result.check_returncode()
execute = ['conda','run','-n','kaggriculture',str(binary),str(RUN/'lifetimes.txt'),str(RUN/'scenarios.txt'),str(RUN/'MODEL.json')]
subprocess.run(execute,check=True)
(RUN/'COMMANDS.json').write_text(json.dumps([command,execute],indent=2)+'\n')
model = json.loads((RUN/'MODEL.json').read_text())
planned = json.loads((EXP/'runs/dated_expansion_sep08_001/proposals/dated_expansion_p362/PLAN.json').read_text())
assert model['intended'][0]['produced'] == planned['produced']
assert model['intended'][0]['work_gap'] == planned['work_gap']
assert len(model['observed_lifetimes']) == len(games)
print('1024 matched profile models complete; original intended output/work-gap exactly reproduced.')
