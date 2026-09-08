from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
binary = ROOT / json.loads((RUN / 'FRESH_BUILD.json').read_text())['binary']
output = RUN / 'native'
output.mkdir(exist_ok=False)
agents = ['rival_wool_context_v2', 'wool_family_context_v2']
opponents = ['wool_family_context_v2', 'investment_context_guarded_001_best', 'teammate_shoprouter',
             'king_rc4', 'public_router', 'public_router_v5', 'public_router_v52', 'junghoon_wool_sales', 'john_131']
jobs = [(a, b, True) for b in opponents for a in agents] + [(a, 'pass', False) for a in agents]
(RUN / 'NATIVE_PLAN.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(), 'seed_start': 1934000,
    'seeds': 128, 'seat_mode': 'both', 'jobs': jobs, 'scope': 'Frozen candidate native-stream and PASS audit; no fitting.', 'gates': {'direct_parent_paired_margin_nonnegative_every_game': True, 'minimum_mean_margin_gain': 0, 'minimum_individual_utility_gain': 0, 'minimum_pass_J_gain': 0}}, indent=2) + '\n')


def run(job):
    a, b, native = job
    path = output / f'{a}_vs_{b}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', a, '--b', b, '--games', '128',
               '--seed-start', '1934000', '--seat-mode', 'both', '--threads', '6', '--validate', '--profile', '--output', str(path)]
    if native:
        command.append('--native-shops')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    result = json.loads(path.read_text())
    report = {'a': a, 'b': b, 'native': native, 'games': len(result['games']), 'utility': result['win_utility'], 'margin': result['mean_margin']}
    print(report, flush=True)
    return report


with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, jobs))
(RUN / 'NATIVE_RESULTS.json').write_text(json.dumps(results, indent=2) + '\n')
