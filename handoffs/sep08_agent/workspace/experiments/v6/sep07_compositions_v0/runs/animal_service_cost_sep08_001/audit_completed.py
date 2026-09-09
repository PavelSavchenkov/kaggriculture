"""Independently replay complete care/workforce courses and compare their parent courses."""
import csv
import hashlib
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
OUT = RUN / 'audits'
OUT.mkdir(exist_ok=True)
binary = EXP / 'runs/animal_groups_sep08_001/build/final_audit'
jobs = [json.loads(p.read_text()) for p in sorted((RUN / 'compiled_30s').glob('*_EXECUTION.json'))]


def audit(job):
    assert job['returncode'] == 0
    prefix = RUN / 'compiled_30s' / job['name']
    rows = list(csv.DictReader((prefix / 'compile.csv').open()))
    final = next(r for r in reversed(rows) if r['day'] == '29' and r['endpoint'] == '1')
    extra = final['extra_hires']
    folder = prefix / 'days/29'
    destination = OUT / job['name']
    command = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'),
        str(binary), str(prefix), str(folder / f'raw_schedule_h{extra}.txt'), str(folder / f'problem_h{extra}.json'),
        str(folder / f'orders_h{extra}.txt'), str(job['seed']), str(destination)]
    if not destination.exists():
        with (OUT / (job['name'] + '.log')).open('w') as log:
            subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    assert json.loads((destination / 'STATUS.json').read_text())['turns'] == 719
    result = json.loads((destination / 'MATCHED_RESULT.json').read_text())
    compiled = json.loads((prefix / 'MATCHED_RESULT.json').read_text())
    assert all(result[k] == compiled[k] for k in ('cash', 'rival_cash', 'produced0', 'produced1'))
    old = json.loads((EXP / job['baseline'] / 'MATCHED_RESULT.json').read_text())
    assert all(result[k] == old[k] for k in ('parent_cash', 'parent_rival_cash', 'produced0', 'produced1'))
    report = {'name': job['name'], 'command': command, 'cash': result['cash'], 'rival_cash': result['rival_cash'],
        'old_course_cash': old['cash'], 'old_course_rival_cash': old['rival_cash'], 'gain_cash': result['cash'] - old['cash'],
        'gain_margin': result['cash'] - result['rival_cash'] - old['cash'] + old['rival_cash'],
        'hires': result['hires'], 'hire_cost': result['hire_cost'], 'old_hires': old['hires'], 'old_hire_cost': old['hire_cost'],
        'output_equal_both_farms': True, 'full_game_and_all_endpoints': True}
    print(json.dumps(report), flush=True)
    return report


with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(audit, jobs))
(OUT / 'RESULTS.json').write_text(json.dumps({'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
    'cases': results, 'expected_cases': 4, 'complete': len(results) == 4}, indent=2) + '\n')
