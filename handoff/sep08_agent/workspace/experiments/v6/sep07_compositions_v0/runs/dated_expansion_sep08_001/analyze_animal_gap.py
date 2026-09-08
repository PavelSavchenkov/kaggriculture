"""Separate requested birth dates, actual births/service, and harvesting losses."""
from pathlib import Path
import json
import statistics
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
plans = {p['id']:p for p in json.loads((RUN/'estimated/proposals.json').read_text())}
names = ['dated_expansion_p0','dated_expansion_p355','dated_expansion_p356','dated_expansion_p361','dated_expansion_p362']
cases = []
for name in names:
    plan = plans[int(name.removeprefix('dated_expansion_p'))]
    games = json.loads((RUN/f'discovery/{name}_vs_public_router.json').read_text())['games']
    for g in games:
        cases.append((f"{name}_{g['seed']}_s{g['seat']}",name,plan,g))
with (RUN/'animal_gap_input.txt').open('w') as f:
    f.write(f'{len(cases)}\n')
    for label,name,plan,g in cases:
        planned = [l for l in plan['lives'] if l[0]>=9]
        actual = [l for l in g['profile']['lives'] if l[0]>=9]
        f.write(f'{label} {len(planned)} {len(actual)}\n')
        for l in planned:
            f.write(f'{l[0]} {l[1]//24} {min(30,(l[2]+23)//24)}\n')
        for l in actual:
            f.write(f'{l[0]} {l[5]} {min(30,(l[4]+23)//24)} {l[8]} {l[9]} {l[10]}\n')
commands = [['conda','run','-n','kaggriculture','g++','-std=c++20','-O3','-DNDEBUG','-march=native','-fno-exceptions','-fno-rtti',
    '-I',str(ROOT),str(RUN/'animal_gap.cpp'),'-o',str(RUN/'animal_gap')],
    ['conda','run','-n','kaggriculture',str(RUN/'animal_gap'),str(RUN/'animal_gap_input.txt'),str(RUN/'animal_gap_outputs.json')]]
for i,command in enumerate(commands):
    with (RUN/f'animal_gap_command{i}.log').open('w') as log:
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
outputs = {r['case']:r['outputs'] for r in json.loads((RUN/'animal_gap_outputs.json').read_text())}
rows = []
for name in names:
    subset = [c for c in cases if c[1]==name]
    values = [[statistics.mean(outputs[c[0]][mode][i] for c in subset) for i in range(5,9)] for mode in range(3)]
    values.append([statistics.mean(c[3]['produced'][i] for c in subset) for i in range(5,9)])
    rows.append({'name':name,'outputs':values,'birth_survival_gap':[a-b for a,b in zip(values[0],values[1])],
                 'service_gap':[a-b for a,b in zip(values[1],values[2])],
                 'harvest_model_gap':[a-b for a,b in zip(values[2],values[3])]})
report = {'games':len(cases),'products':['egg','milk','wool','fertilizer'],
    'modes':['requested_births_full_service','actual_birth_survival_full_service','actual_birth_survival_feed_care_collection_daily_harvest','actual_harvested'],
    'rows':rows,'commands':commands,
    'limits':['Daily-harvest biology is a diagnostic potential, not an executable schedule. Its difference from actual output includes held-cap losses and any day-mask timing/model error.',
              'Birth/survival changes can arise from cash, construction, routes, or escape. This is a decomposition, not yet a causal allocation to finance alone.']}
(RUN/'ANIMAL_GAPS.json').write_text(json.dumps(report,indent=2)+'\n')
for row in rows:
    print(row)
