from pathlib import Path
import csv
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
command=['conda','run','-n','kaggriculture','g++','-std=c++20','-O3','-DNDEBUG','-march=native','-mtune=native',
    '-fno-exceptions','-fno-rtti','-fno-math-errno','-fno-semantic-interposition','-fno-plt','-flto','-pthread','-I',str(ROOT),
    str(RUN/'coverage.cpp'),str(EXP/'runs/compiler_placement_sep08_001/compiler/source/agent.cpp'),
    str(EXP/'league/public_router/source/agent.cpp'),'-o',str(RUN/'coverage')]
with (RUN/'coverage_build.log').open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
run=['conda','run','-n','kaggriculture',str(RUN/'coverage'),str(RUN/'coverage_results')]
subprocess.run(run,check=True)
checked=0
for path in (RUN/'coverage_results').glob('*.json'):
    actual=json.loads(path.read_text())['games']
    source=json.loads((RUN/'discovery'/path.name).read_text())['games']
    expected={(g['seed'],g['seat']):g for g in source}
    for g in actual:assert g==expected[(g['seed'],g['seat'])],path.name
    checked+=len(actual)
rows=list(csv.DictReader((RUN/'coverage_results/coverage.csv').open()))
summary={}
for row in rows:
    key=f"mode{row['mode']}_vs_{row['opponent']}"
    summary.setdefault(key,{'games':0,'complete_day_schedule':0,'mismatch_hours':[]})
    summary[key]['games']+=1
    summary[key]['complete_day_schedule']+=int(row['scheduled_hours'])==24
    if int(row['first_mismatch'])>=0:summary[key]['mismatch_hours'].append(int(row['first_mismatch']))
(RUN/'COVERAGE.json').write_text(json.dumps({'commands':[command,run],'full_records_exact':checked,'summary':summary},indent=2)+'\n')
print('Full record checks:',checked,'coverage:',summary)
