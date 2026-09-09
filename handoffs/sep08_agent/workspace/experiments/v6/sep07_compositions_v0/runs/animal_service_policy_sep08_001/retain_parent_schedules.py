"""Add the original complete cow courses to the certified incumbent pool."""
from pathlib import Path
import csv
import hashlib
import json
import shutil
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
SERVICE = EXP / 'runs/animal_service_cost_sep08_001'
OUT = RUN / 'retained_parent'
OUT.mkdir(exist_ok=False)

def hire_cost(path):
    data = [int(x) for x in path.read_text().split()]
    at = 0
    count = 0
    for hour in range(24):
        units, orders = data[at:at+2]
        at += 2 + units * 3
        count += sum(data[at+i*3] == 1 for i in range(orders))
        at += orders * 3
    assert at == len(data)
    a, b, cost = 1, 1, 0
    for _ in range(count):
        cost += a
        a, b = b, a+b
    return count, cost

results = []
for seed in (1014, 1019):
    original = EXP / ('runs/animal_groups_sep08_001/integrated_audit/cow_triple' if seed == 1014
        else 'runs/animal_group_policy_sep08_001/contexts_audit/cow_triple_alternate')
    previous = SERVICE / 'incumbent_zero_budget_v2' / str(seed)
    selection = []
    for day in range(15, 30):
        before, after = [root / 'days' / str(day) for root in (original, previous)]
        old_hires, old_cost = hire_cost(before / 'actions.txt')
        new_hires, new_cost = hire_cost(after / 'actions.txt')
        same_guard = (before / 'guard.txt').read_bytes() == (after / 'guard.txt').read_bytes()
        chosen = before if same_guard and old_cost < new_cost else after
        dest = OUT / f'{seed}_incumbents/days' / str(day)
        dest.mkdir(parents=True)
        for name in ('actions.txt', 'problem.json'):
            shutil.copyfile(chosen / name, dest / name)
        selection.append({'day': day, 'original_hires': old_hires, 'previous_hires': new_hires,
            'original_cost': old_cost, 'previous_cost': new_cost, 'same_entry_guard': same_guard,
            'chosen': str(chosen.relative_to(EXP)),
            'sha256': {name: hashlib.sha256((dest / name).read_bytes()).hexdigest() for name in ('actions.txt', 'problem.json')}})
    (OUT / f'{seed}_selection.json').write_text(json.dumps(selection, indent=2) + '\n')
    prefix = OUT / str(seed)
    command = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'),
        str(SERVICE / 'build/care_minimize'), str(prefix), str(seed), str(SERVICE / f'cow_triple_{seed}.txt'),
        '0', '-', str(OUT / f'{seed}_incumbents')]
    with (OUT / f'{seed}_compile.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    retained = list(csv.DictReader((prefix / 'incumbents.csv').open()))
    assert {int(row['day']) for row in retained if row['endpoint_valid'] == '1'} == set(range(15, 30))
    rows = list(csv.DictReader((prefix / 'compile.csv').open()))
    extra = next(row['extra_hires'] for row in reversed(rows) if row['day'] == '29' and row['endpoint'] == '1')
    folder = prefix / 'days/29'
    audit = OUT / f'{seed}_audit'
    audit_command = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'),
        str(EXP / 'runs/animal_groups_sep08_001/build/final_audit'), str(prefix),
        str(folder / f'raw_schedule_h{extra}.txt'), str(folder / f'problem_h{extra}.json'),
        str(folder / f'orders_h{extra}.txt'), str(seed), str(audit)]
    with (OUT / f'{seed}_audit.log').open('w') as log:
        subprocess.run(audit_command, stdout=log, stderr=subprocess.STDOUT, check=True)
    actual = json.loads((audit / 'MATCHED_RESULT.json').read_text())
    parent = json.loads((original / 'MATCHED_RESULT.json').read_text())
    assert all(actual[k] == parent[k] for k in ('produced0', 'produced1', 'rival_cash', 'parent_cash', 'parent_rival_cash'))
    assert actual['cash'] - parent['cash'] == parent['hire_cost'] - actual['hire_cost'] == 411
    assert json.loads((audit / 'STATUS.json').read_text())['turns'] == 719
    result = {'seed': seed, 'command': command, 'audit_command': audit_command,
        'cash': actual['cash'], 'gain_vs_original': actual['cash'] - parent['cash'],
        'hire_cost': actual['hire_cost'], 'hires': actual['hires'], 'parent_hires': parent['hires'],
        'all_15_days_physically_and_economically_revalidated': True, 'zero_new_search': True,
        'both_farms_production_equal': True, 'rival_cash_equal': True}
    results.append(result)
    print(json.dumps(result), flush=True)
(OUT / 'RESULTS.json').write_text(json.dumps({'complete': True, 'cases': results,
    'scope': 'Two fixed course contexts, not a league gain. Incumbents include the original course, not just new solver runs.'}, indent=2) + '\n')
