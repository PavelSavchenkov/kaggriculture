from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import json
import subprocess

RUN = Path(__file__).resolve().parent
output = RUN / 'counterfactuals'
output.mkdir(exist_ok=False)
jobs = [(a, b) for a in ['baseline', 'force'] for b in ['late_value_s32_t0_r05', 'public_router_v52', 'public_router']]
plan = {'created_utc': datetime.now(timezone.utc).isoformat(), 'seed_start': 1000, 'seeds': 1024, 'seats': 'both',
        'scope': 'Reused diagnosis seeds, not fresh validation. Same incumbent prefix; force the complete optimized wool course only behind its physical entry guard.',
        'policies': ['late_value_s32_t0_r05', 'v52_transfer_wool0_h18'], 'opponents': sorted({b for a, b in jobs}),
        'features': ['shop0', 'shop1', *[f'market_{p}' for p in range(9)], 'own_cash', 'rival_cash', 'rival_land', 'rival_units',
                     'rival_goose', 'rival_cow', 'rival_sheep', *[f'rival_crop_{p}' for p in range(5)], 'own_wheat', 'own_fertilizer', 'n_shops']}
(RUN / 'COUNTERFACTUAL_PLAN.json').write_text(json.dumps(plan, indent=2) + '\n')


def run(job):
    a, b = job
    path = output / f'{a}_vs_{b}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(RUN / 'build/economic_probe'), '--a', a, '--b', b,
               '--games', '1024', '--seed-start', '1000', '--seat-mode', 'both', '--threads', '6', '--validate', '--profile', '--output', str(path)]
    path.with_suffix('.command.json').write_text(json.dumps(command, indent=2) + '\n')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    data = json.loads(path.read_text())
    report = {'a': a, 'b': b, 'games': len(data['games']), 'utility': data['win_utility'], 'margin': data['mean_margin'], 'seconds': data['seconds']}
    print(report, flush=True)
    return report


with ThreadPoolExecutor(max_workers=2) as pool:
    rows = list(pool.map(run, jobs))
(RUN / 'COUNTERFACTUAL_RESULTS.json').write_text(json.dumps(rows, indent=2) + '\n')
