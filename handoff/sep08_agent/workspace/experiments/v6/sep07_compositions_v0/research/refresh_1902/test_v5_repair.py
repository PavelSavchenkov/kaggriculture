from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import json
import statistics
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
binary = EXP / 'build/87f1f5ee4183423f0b29/arena'
output = RUN / 'v5_repair_discovery'
output.mkdir(exist_ok=False)
agents = ['public_router_v5', 'public_router_v5_repair']
opponents = ['public_router_v5', 'public_router_v5_repair', 'opening_q32_b13_v1', 'teammate_shoprouter', 'king_rc4', 'public_router', 'pass']


def run(job):
    agent, opponent = job
    path = output / f'{agent}_vs_{opponent}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', agent, '--b', opponent,
               '--games', '32', '--seed-start', '1000', '--seat-mode', 'both', '--threads', '4',
               '--profile', '--validate', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    return json.loads(path.read_text())['games']


jobs = [(a, b) for b in opponents for a in agents]
with ThreadPoolExecutor(max_workers=2) as pool:
    games = dict(zip(jobs, pool.map(run, jobs)))
rows = []
for opponent in opponents:
    old, new = [games[a, opponent] for a in agents]
    rows.append({'opponent': opponent, 'games': len(new), 'full_records_equal': old == new,
                 'wins_before': sum(g['cash'] > g['opponent_cash'] for g in old),
                 'wins_after': sum(g['cash'] > g['opponent_cash'] for g in new),
                 'cash_gain': statistics.mean(a['cash'] - b['cash'] for a, b in zip(new, old)),
                 'margin_gain': statistics.mean(a['cash'] - a['opponent_cash'] - b['cash'] + b['opponent_cash'] for a, b in zip(new, old)),
                 'faults_gain': statistics.mean(a['unit_faults'] - b['unit_faults'] for a, b in zip(new, old))})
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'games': sum(map(len, games.values())),
          'scope': 'Discovery only, custom shop mode; normalizer inherited from source-validated V5 port.', 'opponents': rows}
(RUN / 'V5_REPAIR_DISCOVERY.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2), flush=True)
