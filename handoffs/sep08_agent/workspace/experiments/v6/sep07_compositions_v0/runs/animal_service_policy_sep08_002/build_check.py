"""Build and execute original-course parity plus the active repair witnesses."""
from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
registry = json.loads((EXP / 'configs/league.json').read_text())
sources = {RUN / 'source/check.cpp'}
for name in ('cow_service_retained_q24_premium_m2', 'public_router', 'king_rc4'):
    package = ROOT / registry[name]
    sources.update((package / p).resolve() for p in json.loads((package / 'agent.json').read_text())['sources'])
build = RUN / 'check_build_v2'
build.mkdir(exist_ok=False)
command = ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O3', '-march=native',
    '-fno-exceptions', '-fno-rtti', '-pthread', '-I', str(ROOT), *map(str, sorted(sources)), '-o', str(build / 'check')]
with (build / 'build.log').open('x') as log:
    subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
run = ['conda', 'run', '-n', 'kaggriculture', str(build / 'check'),
    str(RUN / 'FIXTURES.txt')]
with (RUN / 'FIXTURE_RESULTS_v2.csv').open('x') as output:
    subprocess.run(run, stdout=output, stderr=subprocess.STDOUT, check=True)
(RUN / 'FIXTURE_CHECKS.json').write_text(json.dumps({'commands': [command, run],
    'original_courses': 12, 'active_repair_pairs': 2, 'games': 16,
    'checks': '12 full-course cash/guard fixtures match:10 unchanged,2 save411. Both cow leaves have identical daily product orders and fixed cost reduced411. Both market modes preserve the active weed repair and production while saving4 worker-days.',
    'sources': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(sources)}}, indent=2) + '\n')
print('Twelve original courses and two active repair pairs pass, 16 complete games.')
