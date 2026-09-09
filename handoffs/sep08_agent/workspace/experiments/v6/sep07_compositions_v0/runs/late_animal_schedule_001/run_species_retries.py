"""Give cow/sheep the same initial 30-second fixed-workforce retry as goose."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import json
import statistics
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
COURSES = EXP / 'runs/late_animal_rotation_001'
output = RUN / 'species30'
output.mkdir(exist_ok=False)
cases = []
for species in ['cow', 'sheep']:
    for leaf in ['off', 'on']:
        source = COURSES / f'{species}_c31_d13_{leaf}_002'
        for day in range(13, 30):
            path = source / f'days/{day}'
            problem = json.loads((path / 'problem.json').read_text())
            control = json.loads((COURSES / f'control_c31_d13_{leaf}_002/days/{day}/problem.json').read_text())
            if problem['worker_count'] > control['worker_count']:
                target = path / 'problem_h0.json'
                assert json.loads(target.read_text())['worker_count'] == control['worker_count']
                cases.append({'species': species, 'leaf': leaf, 'day': day, 'problem': str(target),
                              'actions': str(path / 'actions.txt'), 'worker_count': control['worker_count'],
                              'problem_sha256': hashlib.sha256(target.read_bytes()).hexdigest()})
(output / 'PREREGISTERED.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'budget_seconds': 30, 'cases': cases, 'scope': 'Fixed existing biological and sales contract; UNKNOWN is not an infeasibility proof.'}, indent=2) + '\n')


def run(case):
    name = f"{case['species']}_{case['leaf']}_day{case['day']}"
    path = output / name
    command = ['conda', 'run', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'),
               str(RUN / 'build/retry'), case['problem'], case['actions'], str(path), '30']
    with (output / f'{name}.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    status = json.loads((path / 'STATUS.json').read_text())
    print(name, status, flush=True)
    return {**case, 'result': str(path), 'status': status, 'command': command}


with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, cases))
(output / 'RESULTS.json').write_text(json.dumps(results, indent=2) + '\n')
audits = []
for species in ['cow', 'sheep']:
    for leaf in ['off', 'on']:
        selected = [r for r in results if r['species'] == species and r['leaf'] == leaf
                    and r['status'].get('requirements') and r['status'].get('invariants')]
        mapping = output / f'{species}_{leaf}.txt'
        mapping.write_text(''.join(f"{r['day']} {r['result']}/actions.txt\n" for r in selected))
        folder = output / f'{species}_{leaf}_audit'
        command = ['conda', 'run', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'),
                   str(RUN / f'build/audit_{leaf}'), str(COURSES / f'{species}_c31_d13_{leaf}_002'), str(mapping), str(folder)]
        with (output / f'{species}_{leaf}_audit.log').open('w') as log:
            subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
        old = json.loads((folder / 'original.json').read_text())['games']
        new = json.loads((folder / 'candidate.json').read_text())['games']
        source = json.loads((COURSES / f'{species}_c31_d13_{leaf}_002/exact/candidate.json').read_text())['games']
        assert old == source
        report = {'species': species, 'leaf': leaf, 'games': len(new), 'days': [r['day'] for r in selected],
                  'production_equal': all(a['produced'] == b['produced'] for a, b in zip(new, old)),
                  'sales_equal': all(a['sold'] == b['sold'] for a, b in zip(new, old)),
                  'rival_actions_equal': all(a['opponent_action_hash'] == b['opponent_action_hash'] for a, b in zip(new, old)),
                  'cash_gain': statistics.mean(a['cash'] - b['cash'] for a, b in zip(new, old)),
                  'hire_cost_saving': statistics.mean(b['profile']['hire_cost'] - a['profile']['hire_cost'] for a, b in zip(new, old))}
        audits.append(report)
        assert report['production_equal'] and report['sales_equal'] and report['rival_actions_equal']
        assert report['cash_gain'] == report['hire_cost_saving']
        print(report, flush=True)
(output / 'AUDITS.json').write_text(json.dumps(audits, indent=2) + '\n')
