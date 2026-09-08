from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
variants = json.loads((RUN / 'LINEAGE.json').read_text())['variants']
commands = []


def build(item):
    name, args = item
    command = ['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/build_arena.py'), *args]
    result = subprocess.run(command, capture_output=True, text=True)
    (RUN / f'operations_build_{name}.log').write_text(result.stdout+result.stderr)
    result.check_returncode()
    return name, {'command': command, 'binary': result.stdout.strip().splitlines()[-1]}


with ThreadPoolExecutor(max_workers=2) as pool:
    binaries = dict(pool.map(build, [('pair', ['--pair', 'day_program_p355_m3', 'public_router']),
        ('debug', ['--agents', *variants, 'public_router', 'pass', '--debug'])]))
binaries['generic'] = json.loads((RUN / 'BUILD.json').read_text())
(RUN / 'OPERATIONAL_BINARIES.json').write_text(json.dumps(binaries, indent=2)+'\n')
out = RUN / 'operations'
out.mkdir(exist_ok=False)


def run(label, binary, a, b, threads=2, expansions=100000):
    path = out / f'{label}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', binary, '--a', a, '--b', b,
        '--games', '2', '--seed-start', '1000', '--seat-mode', 'both', '--threads', str(threads),
        '--budget-expansions', str(expansions), '--validate', '--profile', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    commands.append(command)
    return json.loads(path.read_text())['games']


records = [run(name, binaries[name]['binary'], 'day_program_p355_m3', 'public_router') for name in ['pair', 'generic', 'debug']]
records += [run('one_thread', binaries['generic']['binary'], 'day_program_p355_m3', 'public_router', threads=1)]
assert all(r == records[0] for r in records)
for name in variants:
    for opponent in [name, 'pass']:
        run(f'{name}_vs_{opponent}', binaries['debug']['binary'], name, opponent)
for case in ['p355', 'p362']:
    original = run(f'{case}_budget0_control', binaries['generic']['binary'], f'day_program_{case}_m0', 'public_router', expansions=0)
    for mode in [2, 3]:
        actual = run(f'{case}_m{mode}_budget0', binaries['generic']['binary'], f'day_program_{case}_m{mode}', 'public_router', expansions=0)
        assert actual == original
report = {'games': len(commands)*4, 'generic_pair_debug_thread_full_records_equal': 4,
    'all_modes_debug_self_and_pass_complete': True, 'zero_budget_fallback_equal_games': 16, 'commands': commands,
    'state': 'Base controller, observation model, schedules and counters are per-instance and reset.',
    'observation': 'Only AgentObservation and public config. Rival private inventory is excluded; forecast assumes rival PASS and no random weeds.',
    'scope': 'Operational checks do not establish competitive strength. Timing is checked separately.'}
(RUN / 'OPERATIONAL_CHECKS.json').write_text(json.dumps(report, indent=2)+'\n')
print('Operational checks complete:', report['games'], 'games.')
