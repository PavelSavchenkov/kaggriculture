from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
variants = json.loads((RUN / 'LINEAGE.json').read_text())['variants']
parent = 'observed_sale_lead_start_216'
candidate = 'empty_sale_floor_m1'
commands = []


def build(item):
    label, args = item
    command = ['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/build_arena.py'), *args]
    result = subprocess.run(command, capture_output=True, text=True)
    (RUN / f'operations_build_{label}.log').write_text(result.stdout + result.stderr)
    result.check_returncode()
    return label, {'command': command, 'binary': result.stdout.strip().splitlines()[-1]}


with ThreadPoolExecutor(max_workers=2) as pool:
    binaries = dict(pool.map(build, [('pair', ['--pair', candidate, parent]),
        ('debug', ['--agents', *variants, parent, 'pass', '--debug'])]))
binaries['generic'] = json.loads((RUN / 'BUILD.json').read_text())
(RUN / 'OPERATIONAL_BINARIES.json').write_text(json.dumps(binaries, indent=2) + '\n')
out = RUN / 'operations'
out.mkdir(exist_ok=False)


def run(label, binary, a, b, threads=2):
    path = out / f'{label}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', binary, '--a', a, '--b', b,
        '--games', '2', '--seed-start', '1000', '--seat-mode', 'both', '--threads', str(threads),
        '--budget-expansions', '100000', '--validate', '--profile', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    commands.append(command)
    return json.loads(path.read_text())['games']


pair = run('pair', binaries['pair']['binary'], candidate, parent)
release = run('generic', binaries['generic']['binary'], candidate, parent)
debug = run('debug', binaries['debug']['binary'], candidate, parent)
one_thread = run('one_thread', binaries['generic']['binary'], candidate, parent, threads=1)
assert pair == release == debug == one_thread
for name in variants:
    for opponent in [name, 'pass']:
        run(f'{name}_vs_{opponent}', binaries['debug']['binary'], name, opponent)
report = {'games': 48, 'generic_pair_debug_thread_full_records_equal': 4,
    'all_modes_debug_self_and_pass_complete': True, 'commands': commands,
    'state': 'Inherited policy state and removal/protection counters are per-instance and reset. Floor arrays are stack-local; inherited tables are immutable.',
    'observation': 'Floor guard reads current public market inventory and its own returned orders only. No seed, future randomness, simulator or rival-private stock reaches the policy.',
    'scope': 'Operational checks for four floor-guard/control policies. Competitive acceptance requires separate frozen audits.'}
(RUN / 'OPERATIONAL_CHECKS.json').write_text(json.dumps(report, indent=2) + '\n')
print('Operational checks complete:', report['games'], 'games.')
