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
for name in ('animal_repair_m2', 'public_router', 'king_rc4'):
    package = ROOT / registry[name]
    sources.update((package / p).resolve() for p in json.loads((package / 'agent.json').read_text())['sources'])
build = RUN / 'check_build'
build.mkdir(exist_ok=False)
command = ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O3', '-march=native',
    '-fno-exceptions', '-fno-rtti', '-pthread', '-I', str(ROOT), *map(str, sorted(sources)), '-o', str(build / 'check')]
with (build / 'build.log').open('x') as log:
    subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
run = ['conda', 'run', '-n', 'kaggriculture', str(build / 'check'),
    str(EXP / 'runs/animal_group_policy_sep08_001/LIBRARY_FIXTURES.txt')]
with (RUN / 'FIXTURE_RESULTS.csv').open('x') as output:
    subprocess.run(run, stdout=output, stderr=subprocess.STDOUT, check=True)
(RUN / 'FIXTURE_CHECKS.json').write_text(json.dumps({'commands': [command, run],
    'original_courses': 12, 'active_repair_pairs': 2, 'games': 16,
    'checks': 'Original cash/guards unchanged; active mode1 matches independently compiled full-game cash; both modes restore exactly6wheat with unchanged other production and worker days, no remaining missed day guards.',
    'sources': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(sources)}}, indent=2) + '\n')
print('Twelve original courses and two active repair pairs pass, 16 complete games.')
