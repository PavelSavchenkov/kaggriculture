"""Independently audit the complete funded mixed course and retain daily economics."""
from pathlib import Path
import csv
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
assert json.loads((RUN / 'RESUME_EXECUTION.json').read_text())['returncode'] == 0
prefix = RUN / 'season_30s'
days = list(csv.DictReader((prefix / 'compile.csv').open()))
assert {int(row['day']) for row in days if row['endpoint'] == '1'} == set(range(9,30))
extra = next(row['extra_hires'] for row in reversed(days) if row['day']=='29' and row['endpoint']=='1')
folder = prefix / 'days/29'
out = RUN / 'audit'
binary = EXP / 'runs/animal_group_policy_sep08_001/build/audit_public'
command = ['conda','run','--no-capture-output','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),
    str(binary),str(prefix),str(folder/f'raw_schedule_h{extra}.txt'),str(folder/f'problem_h{extra}.json'),
    str(folder/f'orders_h{extra}.txt'),'1008',str(out)]
paths = [*prefix.glob('days/*/actions.txt'),*prefix.glob('days/*/problem.json'),binary]
before = {str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
(RUN/'AUDIT_PROTOCOL.json').write_text(json.dumps({'command':command,'source_sha256':before},indent=2)+'\n')
with (RUN/'audit.log').open('w') as log:
    subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
assert all(hashlib.sha256((EXP/p).read_bytes()).hexdigest()==h for p,h in before.items())
actual = json.loads((out/'MATCHED_RESULT.json').read_text())
compiled = json.loads((prefix/'MATCHED_RESULT.json').read_text())
assert all(actual[key]==compiled[key] for key in ('cash','rival_cash','produced0','produced1','parent_cash','parent_rival_cash'))
assert json.loads((out/'STATUS.json').read_text())['turns']==719
daily = list(csv.DictReader((out/'daily.csv').open()))
assert len(daily)==30 and all(row['exact']=='1' for row in daily)
result = {'complete':True,'turns':719,'days':21,'all_30_endpoints_exact':True,
    'cash':actual['cash'],'parent_cash':actual['parent_cash'],'rival_cash':actual['rival_cash'],
    'parent_rival_cash':actual['parent_rival_cash'],
    'own_gain':actual['cash']-actual['parent_cash'],
    'margin_gain':actual['cash']-actual['rival_cash']-actual['parent_cash']+actual['parent_rival_cash'],
    'hire_cost':actual['hire_cost'],'parent_hire_cost':sum(int(row['parent_hire_cost']) for row in daily),
    'hires':actual['hires'],'produced_gain':[a-b for a,b in zip(actual['produced0'],actual['parent_produced0'])],
    'independent_matches_compiler':True,'source_unchanged':True,
    'scope':'One fixed-calendar mixed course. No general policy or league improvement claim.'}
(RUN/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
