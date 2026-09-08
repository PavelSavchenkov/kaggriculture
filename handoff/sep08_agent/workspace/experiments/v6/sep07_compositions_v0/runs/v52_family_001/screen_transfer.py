from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
binary = EXP / 'build/afa2a6d41d4412fbca37/arena'
output = RUN / 'transfer_discovery'
output.mkdir(exist_ok=False)
agents = ['v52_transfer_wool2', 'v52_transfer_wool4', 'v52_transfer_wool0', 'v52_transfer_routes', 'late_value_s32_t0_r05']
opponents = ['late_value_s32_t0_r05', 'public_router_v52', 'public_router_v5', 'king_rc4', 'teammate_shoprouter', 'public_router']


def run(job):
    a, b = job
    path = output / f'{a}_vs_{b}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', a, '--b', b, '--games', '64',
               '--seed-start', '1000', '--seat-mode', 'both', '--threads', '6', '--validate', '--profile', '--output', str(path)]
    path.with_suffix('.command.json').write_text(json.dumps(command, indent=2) + '\n')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    result = json.loads(path.read_text());games = result['games']
    report = {'a': a, 'b': b, 'games': len(games), 'wins': sum(g['cash'] > g['opponent_cash'] for g in games),
              'ties': sum(g['cash'] == g['opponent_cash'] for g in games), 'utility': result['win_utility'], 'margin': result['mean_margin']}
    print(report, flush=True)
    return report


with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, [(a, b) for a in agents for b in opponents]))
(RUN / 'TRANSFER_DISCOVERY.json').write_text(json.dumps(results, indent=2) + '\n')
