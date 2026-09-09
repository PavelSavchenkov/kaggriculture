from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import json
import subprocess

RUN = Path(__file__).resolve().parent
binary = Path(json.loads((RUN / 'GENERIC_BUILD.json').read_text())['binary'])
output = RUN / 'discovery'
output.mkdir(exist_ok=False)
agents = ['family_delayed_v2', 'family_delayed_rival', 'wool_family_context_v2']
opponents = ['late_value_s32_t0_r05', 'public_router_v52', 'public_router', 'junghoon_wool_sales']
jobs = [(a, b) for b in opponents for a in agents]
(RUN / 'DISCOVERY_PLAN.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'agents': agents, 'opponents': opponents, 'seed_start': 1000, 'seeds': 512, 'seat_mode': 'both',
    'scope': 'Reused diagnosis pool; compare delayed execution separately from added opponent-dependent context.'}, indent=2) + '\n')


def run(job):
    a, b = job
    path = output / f'{a}_vs_{b}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', a, '--b', b, '--games', '512',
               '--seed-start', '1000', '--seat-mode', 'both', '--threads', '6', '--validate', '--output', str(path)]
    path.with_suffix('.command.json').write_text(json.dumps(command, indent=2) + '\n')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    result = json.loads(path.read_text())
    record = {'a': a, 'b': b, 'games': len(result['games']), 'utility': result['win_utility'], 'margin': result['mean_margin']}
    print(record, flush=True)
    return record


with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, jobs))
(RUN / 'DISCOVERY_RESULTS.json').write_text(json.dumps(results, indent=2) + '\n')
