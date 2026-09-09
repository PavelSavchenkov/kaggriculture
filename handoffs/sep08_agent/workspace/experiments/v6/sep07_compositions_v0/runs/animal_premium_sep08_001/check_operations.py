"""Build and test premium-sale ports and ablations under the local API."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
variants = ['animal_premium_m1', 'animal_premium_m2']
opponents = ['empty_sale_slots_m2', 'teammate_shoprouter', 'public_router', 'public_router_v52', 'ahmed_v23', 'junghoon_wool_sales', 'king_rc4', 'yusuke_sep08_m2', 'investment_context_guarded_001_best', 'ahmed_v24', 'premium_sales_s216', 'animal_groups_m1', 'animal_groups_m2']

def build(item):
    name, args = item
    command = ['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/build_arena.py'), *args]
    result = subprocess.run(command, capture_output=True, text=True)
    (RUN / f'operations_build_{name}.log').write_text(result.stdout + result.stderr)
    result.check_returncode()
    return name, {'command': command, 'binary': result.stdout.strip().splitlines()[-1]}

with ThreadPoolExecutor(max_workers=2) as pool:
    binaries = dict(pool.map(build, [
        ('pair', ['--pair', 'animal_premium_m2', 'public_router']),
        ('debug', ['--agents', *variants, 'public_router', 'pass', '--debug']),
        ('generic', ['--agents', *variants, *opponents, 'pass'])]))
(RUN / 'OPERATIONAL_BINARIES.json').write_text(json.dumps(binaries, indent=2) + '\n')
out = RUN / 'operations'
out.mkdir(exist_ok=False)
commands = []

def run(label, binary, a, b, threads=2):
    path = out / f'{label}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', binary, '--games', '2', '--seed-start', '1000',
        '--seat-mode', 'both', '--threads', str(threads), '--budget-expansions', '100000',
        '--validate', '--profile', '--output', str(path), '--a', a, '--b', b]
    with path.with_suffix('.log').open('x') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    commands.append(command)
    games = json.loads(path.read_text())['games']
    assert len(games) == 4
    return games

records = [run(name, binaries[name]['binary'], 'animal_premium_m2', 'public_router') for name in ('pair', 'generic', 'debug')]
records.append(run('one_thread', binaries['generic']['binary'], 'animal_premium_m2', 'public_router', 1))
assert all(r == records[0] for r in records)
for name in variants:
    for opponent in (name, 'pass'):
        run(f'{name}_vs_{opponent}', binaries['debug']['binary'], name, opponent)
report = {'games': 32, 'generic_pair_debug_thread_full_records_equal': 4,
    'all_modes_debug_self_and_pass_complete': True, 'commands': commands,
    'state': 'Stateless stable order transform; parent retains per-instance reset state.',
    'observation': 'Policy uses AgentObservation only. No seed, simulator, future shops or private opponent inventory.',
    'scope': 'Operational tests of four components; competitive evaluation follows.'}
(RUN / 'OPERATIONAL_CHECKS.json').write_text(json.dumps(report, indent=2) + '\n')
print('Operational checks complete: 32 games.', flush=True)
