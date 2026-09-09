"""Common-seed full-game comparison of complete compiled compositions."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import argparse
import json
import statistics
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('binary', type=Path)
args = parser.parse_args()
binary = args.binary.resolve()
assert binary.exists()
output = RUN / 'discovery'
output.mkdir(exist_ok=False)
agents = ['opening_q32_b13_v1', 'late_crop_control', 'late_goose_initial',
          'late_goose_optimized', 'late_cow_initial', 'late_sheep_initial']
opponents = ['opening_q32_b13_v1', 'bohann_opening_v1', 'investment_context_guarded_001_best',
             'public_router', 'public_router_v5', 'teammate_shoprouter', 'king_rc4', 'public_sixday']
jobs = [(a, b) for a in agents for b in opponents]
jobs += [(f'late_goose_optimized_{leaf}', 'public_router') for leaf in ['off', 'on']]
jobs += [(a, b) for a in ['late_goose_optimized'] for b in [a, 'pass']]
record = {'created_utc': datetime.now(timezone.utc).isoformat(), 'binary': str(binary.relative_to(ROOT)),
          'seeds': [1000, 1031], 'seat_mode': 'both', 'profile': True, 'validate': True,
          'purpose': 'Discovery and forced-leaf package parity; no fresh validation claim.', 'jobs': jobs}
(RUN / 'SCREEN_PREREGISTERED.json').write_text(json.dumps(record, indent=2) + '\n')


def run(job):
    a, b = job
    path = output / f'{a}_vs_{b}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', a, '--b', b,
               '--games', '32', '--seed-start', '1000', '--seat-mode', 'both', '--threads', '4',
               '--profile', '--validate', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    games = json.loads(path.read_text())['games']
    result = {'a': a, 'b': b, 'games': len(games), 'wins': sum(g['cash'] > g['opponent_cash'] for g in games),
              'ties': sum(g['cash'] == g['opponent_cash'] for g in games),
              'margin': statistics.mean(g['cash'] - g['opponent_cash'] for g in games)}
    print(result, flush=True)
    return result


with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, jobs))
(RUN / 'SCREEN_RESULTS.json').write_text(json.dumps(results, indent=2) + '\n')
parity = []
for leaf in ['off', 'on']:
    observed = json.loads((output / f'late_goose_optimized_{leaf}_vs_public_router.json').read_text())['games']
    expected = json.loads((RUN / f'{leaf}_terminal_no_care_audit/candidate.json').read_text())['games']
    parity.append({'leaf': leaf, 'games': len(observed), 'full_records_equal': observed == expected})
(RUN / 'PACKAGE_PARITY.json').write_text(json.dumps(parity, indent=2) + '\n')
assert all(row['full_records_equal'] for row in parity), parity
