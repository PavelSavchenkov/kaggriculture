from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
binary = EXP / 'build/ee33d9ed834ac8d4f2dd/arena'
output = RUN / 'v52_discovery'
output.mkdir(exist_ok=False)
opponents = ['public_router_v5', 'late_value_s32_t0_r05', 'opening_q32_b13_v1', 'teammate_shoprouter',
             'king_rc4', 'public_sixday', 'public_router_v52', 'pass']
jobs = [(a, b) for a in ['public_router_v52', 'public_router_v5'] for b in opponents]


def run(job):
    a, b = job
    path = output / f'{a}_vs_{b}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', a, '--b', b, '--games', '128',
               '--seed-start', '1000', '--seat-mode', 'both', '--threads', '6', '--validate', '--profile', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    result = json.loads(path.read_text());games = result['games']
    report = {'a': a, 'b': b, 'games': len(games), 'wins': sum(g['cash'] > g['opponent_cash'] for g in games),
              'ties': sum(g['cash'] == g['opponent_cash'] for g in games), 'utility': result['win_utility'], 'margin': result['mean_margin']}
    print(report, flush=True)
    return report


with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, jobs))
(RUN / 'V52_DISCOVERY.json').write_text(json.dumps(results, indent=2) + '\n')
