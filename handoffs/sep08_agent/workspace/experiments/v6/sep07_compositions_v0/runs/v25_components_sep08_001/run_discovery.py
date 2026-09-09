"""Compare V25 execution layers on the same Yusuke composition against the league."""
import hashlib
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
assert json.loads((RUN / 'OPERATIONAL_CHECKS.json').read_text())['games'] == 84
binary = Path(json.loads((RUN / 'OPERATIONAL_BINARIES.json').read_text())['generic']['binary'])
OUT = RUN / 'discovery_2400000'
OUT.mkdir(exist_ok=False)
agents = [f'v25_components_m{mask}' for mask in range(8)]
opponents = ['teammate_shoprouter', 'public_router', 'public_router_v52',
    'ahmed_v23', 'ahmed_v24', 'junghoon_wool_sales', 'king_rc4',
    'animal_repair_q24_premium_m2', 'yusuke_sep08_m2']
jobs = [{'a': a, 'b': b} for b in opponents for a in agents]
protocol = {'agents': agents, 'opponents': opponents, 'jobs': jobs, 'seed_start': 2400000,
    'seeds': 64, 'seats': 'both', 'games': len(jobs) * 128,
    'primary_question': 'Which of weed repair(bit1), sale leading(bit2), terminal delivery(bit4), and their interactions improve the fixed four-tape policy?',
    'secondary_question': 'Fullmask7 versus leave-one-out masks6/5/3 estimates the added value of each component in its real context; all8 combinations expose interactions.',
    'scope': 'Discovery only. Preserve per-opponent regressions and production/labor effects. No automatic promotion.'}
(OUT / 'PROTOCOL.json').write_text(json.dumps(protocol, indent=2) + '\n')
subprocess.run(['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/freeze_arena_inputs.py'),
    str(binary), str(OUT / 'freeze')], check=True)
frozen = json.loads((OUT / 'freeze/FROZEN.json').read_text())['files_sha256']

def run(job):
    name = job['a'] + '_vs_' + job['b']
    path = OUT / (name + '.json')
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', job['a'], '--b', job['b'],
        '--games', '64', '--seed-start', '2400000', '--seat-mode', 'both', '--threads', '3',
        '--budget-expansions', '100000', '--validate', '--profile', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    games = json.loads(path.read_text())['games']
    assert len(games) == 128 and all(g['turns'] == 719 for g in games)
    row = dict(job, command=command, games=len(games),
        wtl=[sum(g['cash'] > g['opponent_cash'] for g in games), sum(g['cash'] == g['opponent_cash'] for g in games), sum(g['cash'] < g['opponent_cash'] for g in games)],
        mean_margin=sum(g['cash'] - g['opponent_cash'] for g in games) / len(games))
    print(name, row['wtl'], round(row['mean_margin'], 2), flush=True)
    return row

with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, jobs))
assert all(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == digest for name, digest in frozen.items())
(OUT / 'EXECUTION.json').write_text(json.dumps({'games': protocol['games'], 'sources_unchanged': True,
    'results': results}, indent=2) + '\n')
