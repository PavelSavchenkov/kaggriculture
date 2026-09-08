from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import json
import subprocess

RUN = Path(__file__).resolve().parent
output = RUN / 'junghoon_counterfactuals'
output.mkdir(exist_ok=False)
plan = {'created_utc': datetime.now(timezone.utc).isoformat(), 'seed_start': 1000, 'seeds': 1024, 'seats': 'both',
        'scope': 'Diagnosis after failed1870000 fresh panel. Add the missing distinct Junghoon opponent; never call these data fresh validation.',
        'policies': ['late_value_s32_t0_r05', 'v52_transfer_wool0_h18'], 'opponent': 'junghoon_wool_sales'}
(RUN / 'JUNGHOON_COUNTERFACTUAL_PLAN.json').write_text(json.dumps(plan, indent=2) + '\n')


def run(a):
    path = output / f'{a}_vs_junghoon_wool_sales.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(RUN / 'build/economic_probe_v2'), '--a', a, '--b', 'junghoon_wool_sales',
               '--games', '1024', '--seed-start', '1000', '--seat-mode', 'both', '--threads', '6', '--validate', '--profile', '--output', str(path)]
    path.with_suffix('.command.json').write_text(json.dumps(command, indent=2) + '\n')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    data = json.loads(path.read_text())
    report = {'a': a, 'games': len(data['games']), 'utility': data['win_utility'], 'margin': data['mean_margin'], 'seconds': data['seconds']}
    print(report, flush=True)
    return report


with ThreadPoolExecutor(max_workers=2) as pool:
    rows = list(pool.map(run, ['baseline', 'force']))
(RUN / 'JUNGHOON_COUNTERFACTUAL_RESULTS.json').write_text(json.dumps(rows, indent=2) + '\n')
