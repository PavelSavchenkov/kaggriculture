"""Measure entry eligibility without changing policy actions or simulator inputs."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
BUILD = EXP / 'build/fd2766d96d27e5e9d55e'
out = RUN / 'probe'
out.mkdir(exist_ok=False)
command = json.loads((BUILD / 'build.json').read_text())['command']
command = [str(RUN / 'source/probe.cpp') if x == str(EXP / 'src/arena.cpp') else x for x in command]
command[-1] = str(out / 'probe')
prior = json.loads((EXP / 'runs/salem_port_sep08_001/discovery/freeze/FROZEN.json').read_text())['files_sha256']
paths = [ROOT / p for p in prior]
paths += [RUN / 'source/probe.cpp', Path(__file__), BUILD / 'registry.hpp']
paths += list((EXP / 'runs/animal_groups_sep08_001/integrated_30s').glob('*/days/*/guard.txt'))
frozen = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
opponents = ['empty_sale_slots_m2', 'teammate_shoprouter', 'public_router', 'public_router_v52', 'ahmed_v23', 'junghoon_wool_sales', 'king_rc4', 'pass']
protocol = {'created_utc': datetime.now(timezone.utc).isoformat(), 'command': command, 'source_sha256': frozen,
    'opponents': opponents, 'seed_start': 1000, 'seeds': 128, 'seats': 'both',
    'scope': 'Exposed entry survey only; unchanged baseline actions with independent standard-runner controls. No policy or promotion.'}
(out / 'PROTOCOL.json').write_text(json.dumps(protocol, indent=2) + '\n')
with (out / 'build.log').open('x') as log:
    subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)

def run(opponent):
    cmd = ['conda', 'run', '-n', 'kaggriculture', str(out / 'probe'), '--b', opponent,
        '--games', '128', '--seed-start', '1000', '--seat-mode', 'both', '--threads', '4',
        '--output', str(out / f'{opponent}.json')]
    with (out / f'{opponent}.log').open('x') as log:
        subprocess.run(cmd, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
    print(opponent, 'complete', flush=True)
    return cmd

with ThreadPoolExecutor(max_workers=2) as pool:
    commands = list(pool.map(run, opponents))
assert all(hashlib.sha256((ROOT / p).read_bytes()).hexdigest() == value for p, value in frozen.items())
(out / 'EXECUTION.json').write_text(json.dumps({'commands': commands, 'sources_unchanged': True, 'games': 2048, 'control_games': 64}, indent=2) + '\n')
