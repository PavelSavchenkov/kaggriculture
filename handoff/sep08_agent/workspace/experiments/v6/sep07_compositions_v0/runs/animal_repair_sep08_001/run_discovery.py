"""Compare repaired composition and opening combinations on exposed worlds."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
assert json.loads((RUN / 'FIXTURE_CHECKS.json').read_text())['games'] == 16
assert json.loads((RUN / 'OPERATIONAL_CHECKS.json').read_text())['games'] == 56
out = RUN / 'discovery'
out.mkdir(exist_ok=False)
binary = json.loads((RUN / 'OPERATIONAL_BINARIES.json').read_text())['generic']['binary']
subprocess.run(['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/freeze_arena_inputs.py'), binary, str(out / 'freeze')], check=True)
frozen = json.loads((out / 'freeze/FROZEN.json').read_text())['files_sha256']
agents = ['empty_sale_slots_m2', 'premium_sales_s216', 'opening_funding_q24', 'premium_q24_s216',
    'animal_premium_m2', 'animal_repair_premium_m2', 'animal_repair_q24_premium_m2']
opponents = ['empty_sale_slots_m2', 'teammate_shoprouter', 'public_router', 'public_router_v52', 'ahmed_v23',
    'junghoon_wool_sales', 'king_rc4', 'yusuke_sep08_m2', 'ahmed_v24', 'bohann_opening_v1', 'investment_context_guarded_001_best']
jobs = [{'a': a, 'b': b, 'seeds': 64, 'seed_start': 1000, 'native': False} for b in opponents for a in agents]
(out / 'PROTOCOL.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'jobs': jobs, 'games': 9856, 'seats': 'both', 'scope': 'Exposed combination discovery; no promotion. Attribute opening, premium sales, animal course and exact repair separately. Final900000 unused.'}, indent=2) + '\n')

def run(job):
    path = out / f"{job['a']}_vs_{job['b']}.json"
    command = ['conda', 'run', '-n', 'kaggriculture', binary, '--a', job['a'], '--b', job['b'], '--games', '64',
        '--seed-start', '1000', '--seat-mode', 'both', '--threads', '4', '--budget-expansions', '100000',
        '--validate', '--profile', '--output', str(path)]
    with path.with_suffix('.log').open('x') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    # Independently saved source controls, when the same matchup was run earlier.
    previous = EXP / 'runs/premium_sales_sep08_001/discovery' / path.name
    count = 0
    if job['a'] in ('empty_sale_slots_m2', 'premium_sales_s216') and previous.exists():
        current = json.loads(path.read_text())['games']
        assert current == json.loads(previous.read_text())['games']
        count = len(current)
    print(path.stem, 'complete', flush=True)
    return {'command': command, 'original_control_records': count}

with ThreadPoolExecutor(max_workers=2) as pool:
    records = list(pool.map(run, jobs))
assert all(hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == digest for path, digest in frozen.items())
(out / 'EXECUTION.json').write_text(json.dumps({'games': 9856, 'records': records, 'sources_unchanged': True,
    'original_control_records': sum(r['original_control_records'] for r in records)}, indent=2) + '\n')
print('Complete: 9856 games, unchanged inputs and independent source controls.')
