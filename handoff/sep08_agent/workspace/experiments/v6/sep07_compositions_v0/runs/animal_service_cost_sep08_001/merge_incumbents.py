"""Retain the cheaper verified daily incumbent, then independently check the merged season."""
import csv
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
assert json.loads((RUN / 'audits/RESULTS.json').read_text())['complete']
OUT = RUN / 'retained_incumbents'
OUT.mkdir(exist_ok=False)
binary = EXP / 'runs/animal_groups_sep08_001/build/final_audit'
results = []
for seed in (1014, 1019):
    names = [f'care_only_{seed}', f'care_minimize_{seed}']
    daily = {name: list(csv.DictReader((RUN / 'audits' / name / 'daily.csv').open())) for name in names}
    prefix = OUT / str(seed)
    selection = []
    for day in range(15, 30):
        name = min(names, key=lambda n: int(daily[n][day]['hire_cost']))
        source = RUN / 'compiled_30s' / name
        trials = list(csv.DictReader((source / 'compile.csv').open()))
        final = next(row for row in reversed(trials) if int(row['day']) == day and row['endpoint'] == '1')
        extra = final['extra_hires']
        dest = prefix / 'days' / str(day)
        dest.mkdir(parents=True)
        files = ['actions.txt', 'problem.json']
        if day == 29:
            files += [f'raw_schedule_h{extra}.txt', f'problem_h{extra}.json', f'orders_h{extra}.txt']
        hashes = {}
        for filename in files:
            path = source / 'days' / str(day) / filename
            shutil.copyfile(path, dest / filename)
            hashes[filename] = hashlib.sha256(path.read_bytes()).hexdigest()
        selection.append({'day': day, 'source': name, 'extra_hires': int(extra),
            'hire_cost': int(daily[name][day]['hire_cost']), 'files_sha256': hashes})
    (prefix / 'SELECTION.json').write_text(json.dumps(selection, indent=2) + '\n')
    final = prefix / 'days/29'
    extra = selection[-1]['extra_hires']
    audited = OUT / f'{seed}_audit'
    command = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'),
        str(binary), str(prefix), str(final / f'raw_schedule_h{extra}.txt'), str(final / f'problem_h{extra}.json'),
        str(final / f'orders_h{extra}.txt'), str(seed), str(audited)]
    with (OUT / f'{seed}.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    actual = json.loads((audited / 'MATCHED_RESULT.json').read_text())
    parent = json.loads((RUN / 'audits' / names[0] / 'MATCHED_RESULT.json').read_text())
    assert all(actual[k] == parent[k] for k in ('produced0', 'produced1', 'rival_cash', 'parent_cash', 'parent_rival_cash'))
    assert actual['cash'] - parent['cash'] == parent['hire_cost'] - actual['hire_cost'] == 178
    result = {'seed': seed, 'command': command, 'cash': actual['cash'], 'parent_course_cash': parent['cash'],
        'gain_cash': actual['cash'] - parent['cash'], 'gain_margin': actual['cash'] - parent['cash'],
        'hire_cost': actual['hire_cost'], 'parent_course_hire_cost': parent['hire_cost'],
        'both_farms_production_equal': True, 'full_game_and_all_day_endpoints': True}
    print(json.dumps(result), flush=True)
    results.append(result)
(OUT / 'RESULTS.json').write_text(json.dumps({'cases': results, 'scope': 'Two fixed worlds. Keep verified incumbents when search times out; no runtime league promotion.'}, indent=2) + '\n')
