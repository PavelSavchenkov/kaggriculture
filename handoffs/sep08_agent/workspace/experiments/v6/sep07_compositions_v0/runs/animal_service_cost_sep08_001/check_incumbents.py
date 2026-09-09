"""Recompile both known seasons with zero new search, then audit the whole games."""
import csv
import hashlib
import json
import subprocess
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
OUT = RUN / 'incumbent_zero_budget_v2'
OUT.mkdir(exist_ok=False)
records = []
for seed in (1014, 1019):
    prefix = OUT / str(seed)
    command = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'),
        str(RUN / 'build/care_minimize'), str(prefix), str(seed), str(RUN / f'cow_triple_{seed}.txt'),
        '0', '-', str(RUN / 'retained_incumbents' / str(seed))]
    with (OUT / f'{seed}_compile.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    retained = list(csv.DictReader((prefix / 'incumbents.csv').open()))
    days = {int(row['day']) for row in retained if row['endpoint_valid'] == '1'}
    assert days == set(range(15, 30)), days
    rows = list(csv.DictReader((prefix / 'compile.csv').open()))
    final = next(row for row in reversed(rows) if row['day'] == '29' and row['endpoint'] == '1')
    extra = final['extra_hires']
    folder = prefix / 'days/29'
    audit = OUT / f'{seed}_audit'
    audit_command = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'),
        str(EXP / 'runs/animal_groups_sep08_001/build/final_audit'), str(prefix),
        str(folder / f'raw_schedule_h{extra}.txt'), str(folder / f'problem_h{extra}.json'),
        str(folder / f'orders_h{extra}.txt'), str(seed), str(audit)]
    with (OUT / f'{seed}_audit.log').open('w') as log:
        subprocess.run(audit_command, stdout=log, stderr=subprocess.STDOUT, check=True)
    actual = json.loads((audit / 'MATCHED_RESULT.json').read_text())
    expected = json.loads((RUN / 'retained_incumbents' / f'{seed}_audit/MATCHED_RESULT.json').read_text())
    assert actual == expected, (actual, expected)
    assert json.loads((audit / 'STATUS.json').read_text())['turns'] == 719
    record = {'seed': seed, 'command': command, 'audit_command': audit_command, 'retained_days': len(days),
        'zero_new_search': True, 'full_result_equal': True, 'cash': actual['cash'],
        'hire_cost': actual['hire_cost'], 'source_sha256': hashlib.sha256((RUN / 'source/compile.cpp').read_bytes()).hexdigest()}
    records.append(record)
    print(seed, 'all 15 incumbents retained, full game identical', flush=True)
(OUT / 'RESULTS.json').write_text(json.dumps({'cases': records, 'complete': True}, indent=2) + '\n')
