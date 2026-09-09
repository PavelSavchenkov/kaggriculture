from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
binary = EXP / 'build/d7e3f45536cd98865206/arena'
output = RUN / 'initial'
output.mkdir(exist_ok=False)
agents = ['late_force_wait', 'late_force_goose', 'late_force_cow', 'late_force_sheep',
          'late_value_s32_t0_r0', 'late_value_s32_t100_r0', 'late_value_s32_t0_r1',
          'opening_q32_b13_v1', 'late_goose_wheat_context']
jobs = [(a, b) for a in agents for b in ['opening_q32_b13_v1', 'public_router']]


def run(job):
    a, b = job
    path = output / f'{a}_vs_{b}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', a, '--b', b,
               '--games', '32', '--seed-start', '1000', '--seat-mode', 'both', '--threads', '4',
               '--profile', '--validate', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    result = json.loads(path.read_text())
    summary = {'a': a, 'b': b, 'games': len(result['games']), 'utility': result['win_utility'], 'margin': result['mean_margin']}
    print(summary, flush=True)
    return summary


with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, jobs))
(RUN / 'INITIAL_RESULTS.json').write_text(json.dumps(results, indent=2) + '\n')
parity = []
for a, expected in [('late_force_wait', 'opening_q32_b13_v1'), ('late_force_goose', 'late_goose_wheat_context')]:
    for b in ['opening_q32_b13_v1', 'public_router']:
        x = json.loads((output / f'{a}_vs_{b}.json').read_text())['games']
        y = json.loads((output / f'{expected}_vs_{b}.json').read_text())['games']
        parity.append({'a': a, 'expected': expected, 'b': b, 'games': len(x), 'full_records_equal': x == y})
(RUN / 'INITIAL_PARITY.json').write_text(json.dumps(parity, indent=2) + '\n')
assert all(row['full_records_equal'] for row in parity), parity
