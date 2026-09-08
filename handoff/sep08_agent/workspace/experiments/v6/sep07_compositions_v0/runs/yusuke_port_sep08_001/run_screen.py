"""Common-seed C++ discovery for the public tape and both controller components."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import subprocess
import time

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
assert (RUN / 'SOURCE_PARITY.json').exists() and (RUN / 'OPERATIONAL_CHECKS.json').exists()
binary = json.loads((RUN / 'OPERATIONAL_BINARIES.json').read_text())['generic']['binary']
out = RUN / 'discovery'
out.mkdir(exist_ok=False)
freeze = ['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/freeze_arena_inputs.py'), binary, str(out / 'freeze')]
subprocess.run(freeze, check=True)
frozen = json.loads((out / 'freeze/FROZEN.json').read_text())['files_sha256']
variants = [f'yusuke_sep08_m{i}' for i in range(4)]
opponents = ['empty_sale_slots_m2', 'teammate_shoprouter', 'public_router', 'public_router_v52', 'ahmed_v23', 'junghoon_wool_sales', 'king_rc4']
jobs = [(a, b) for b in opponents for a in variants]
jobs += [('yusuke_sep08_m2', a) for a in variants if a != 'yusuke_sep08_m2']
(out / 'PROTOCOL.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'seed_start': 1000, 'seeds': 64, 'seat_mode': 'both', 'jobs': jobs,
    'scope': 'Exposed discovery seeds. Frozen inputs; full-game component ablations and direct comparisons. No promotion gate.'}, indent=2) + '\n')

def run(job):
    a, b = job
    path = out / f'{a}_vs_{b}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', binary, '--a', a, '--b', b, '--games', '64',
        '--seed-start', '1000', '--seat-mode', 'both', '--threads', '4', '--budget-expansions', '100000',
        '--validate', '--profile', '--output', str(path)]
    start = time.monotonic()
    with path.with_suffix('.log').open('x') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    print(a, 'vs', b, 'complete', flush=True)
    return {'command': command, 'seconds': time.monotonic() - start}

with ThreadPoolExecutor(max_workers=2) as pool:
    records = list(pool.map(run, jobs))
assert all(hashlib.sha256((ROOT / p).read_bytes()).hexdigest() == value for p, value in frozen.items())
(out / 'EXECUTION.json').write_text(json.dumps({'commands': records, 'sources_unchanged': True, 'games': len(jobs)*128}, indent=2) + '\n')
print('Yusuke discovery complete:', len(jobs)*128, 'games.', flush=True)
