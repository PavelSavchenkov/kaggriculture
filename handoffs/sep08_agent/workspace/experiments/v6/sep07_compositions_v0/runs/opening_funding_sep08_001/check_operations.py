"""Build and test all four Opening funding component variants under the local API."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
variants = [f'opening_funding_q{q}' for q in (32,13,20,24)]
opponents = ['empty_sale_slots_m2', 'teammate_shoprouter', 'public_router', 'public_router_v52', 'ahmed_v23', 'junghoon_wool_sales', 'king_rc4', 'yusuke_sep08_m2', 'bohann_opening_v1', 'opening_q32_b13_v1', 'investment_context_guarded_001_best']

def build(item):
    name, args = item
    command = ['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/build_arena.py'), *args]
    result = subprocess.run(command, capture_output=True, text=True)
    (RUN / f'operations_build_{name}.log').write_text(result.stdout + result.stderr)
    result.check_returncode()
    return name, {'command': command, 'binary': result.stdout.strip().splitlines()[-1]}

with ThreadPoolExecutor(max_workers=2) as pool:
    binaries = dict(pool.map(build, [
        ('pair', ['--pair', 'opening_funding_q13', 'public_router']),
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

records = [run(name, binaries[name]['binary'], 'opening_funding_q13', 'public_router') for name in ('pair', 'generic', 'debug')]
records.append(run('one_thread', binaries['generic']['binary'], 'opening_funding_q13', 'public_router', 1))
assert all(r == records[0] for r in records)
for name in variants:
    for opponent in (name, 'pass'):
        run(f'{name}_vs_{opponent}', binaries['debug']['binary'], name, opponent)
report = {'games': 48, 'generic_pair_debug_thread_full_records_equal': 4,
    'all_modes_debug_self_and_pass_complete': True, 'commands': commands,
    'state': 'Inherited immutable tables; all mutable state belongs to the per-instance strongest parent and resets with it.',
    'observation': 'Policy uses AgentObservation only. No seed, simulator, future shops or private opponent inventory.',
    'scope': 'Operational tests of four components; competitive evaluation follows.'}
(RUN / 'OPERATIONAL_CHECKS.json').write_text(json.dumps(report, indent=2) + '\n')
print('Operational checks complete: 48 games.', flush=True)
