from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
binary = RUN / 'build/probe'
assert binary.exists()
output = RUN / 'counterfactuals'
output.mkdir(exist_ok=False)
jobs = [(force, opponent) for opponent in ['opening_q32_b13_v1', 'public_router'] for force in range(4)]
(RUN / 'COUNTERFACTUAL_PLAN.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'seed_start': 1790000, 'seeds': 512, 'seat_mode': 'both', 'jobs': jobs,
    'scope': 'Previously used discovery/diagnosis seeds. Each legal-observation prefix is identical across the four choices. Record complete outcomes to evaluate selector rules cheaply; validate the selected actual policy separately.'}, indent=2) + '\n')


def run(job):
    force, opponent = job
    path = output / f'force{force}_vs_{opponent}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', str(force), '--b', opponent,
               '--games', '512', '--seed-start', '1790000', '--seat-mode', 'both', '--threads', '6',
               '--validate', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    data = json.loads(path.read_text())
    summary = {'force': force, 'opponent': opponent, 'games': len(data['games']), 'utility': data['win_utility'], 'margin': data['mean_margin']}
    print(summary, flush=True)
    return summary


with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, jobs))
(RUN / 'COUNTERFACTUAL_RESULTS.json').write_text(json.dumps(results, indent=2) + '\n')
