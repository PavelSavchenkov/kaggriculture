"""Compare V25 execution layers on the same Yusuke composition against the league."""
import hashlib
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
assert json.loads((RUN / 'SOURCE_PARITY.json').read_text())['returncode'] == 0
assert json.loads((RUN / 'OPERATIONAL_CHECKS.json').read_text())['games'] == 24
binary = Path(json.loads((RUN / 'OPERATIONAL_BINARIES.json').read_text())['generic']['binary'])
OUT = RUN / 'discovery_2380000'
OUT.mkdir(exist_ok=False)
agents = ['ahmed_v25', 'yusuke_sep08_m2', 'animal_repair_q24_premium_m2']
opponents = ['empty_sale_slots_m2', 'animal_repair_q24_premium_m2', 'yusuke_sep08_m2',
    'ahmed_v24', 'ahmed_v23', 'teammate_shoprouter', 'public_router', 'public_router_v52',
    'junghoon_wool_sales', 'king_rc4', 'pass']
jobs = [{'a': a, 'b': b} for b in opponents for a in agents]
protocol = {'agents': agents, 'opponents': opponents, 'jobs': jobs, 'seed_start': 2380000,
    'seeds': 128, 'seats': 'both', 'games': len(jobs) * 256,
    'primary_question': 'Do execution repairs, sale leading and terminal delivery improve the same four Yusuke compositions?',
    'secondary_question': 'How does the public V25 port compare with the uploaded agent and diverse existing opponents?',
    'scope': 'Discovery only. Preserve per-opponent regressions and production/labor effects. No automatic promotion.'}
(OUT / 'PROTOCOL.json').write_text(json.dumps(protocol, indent=2) + '\n')
subprocess.run(['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/freeze_arena_inputs.py'),
    str(binary), str(OUT / 'freeze')], check=True)
frozen = json.loads((OUT / 'freeze/FROZEN.json').read_text())['files_sha256']

def run(job):
    name = job['a'] + '_vs_' + job['b']
    path = OUT / (name + '.json')
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', job['a'], '--b', job['b'],
        '--games', '128', '--seed-start', '2380000', '--seat-mode', 'both', '--threads', '3',
        '--budget-expansions', '100000', '--validate', '--profile', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    games = json.loads(path.read_text())['games']
    assert len(games) == 256 and all(g['turns'] == 719 for g in games)
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
