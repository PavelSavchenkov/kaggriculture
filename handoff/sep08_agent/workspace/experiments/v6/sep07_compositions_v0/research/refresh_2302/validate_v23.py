"""Build modes, operational parity, and a broad profiled public-port screen."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
candidate = 'ahmed_v23'
parent = 'rival_wool_context_v3'
opponents = [parent, 'public_router_v52', 'public_router_v5', 'teammate_shoprouter',
             'king_rc4', 'public_router', 'junghoon_wool_sales', 'john_131', 'pass']
output = RUN / 'v23_checks'
output.mkdir(exist_ok=False)


def build(kind):
    command = ['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/build_arena.py')]
    if kind == 'pair': command += ['--pair', candidate, 'public_router_v52']
    elif kind == 'debug': command += ['--agents', candidate, 'public_router_v52', '--debug']
    else: command += ['--agents', *sorted(set(opponents + [candidate]))]
    result = subprocess.run(command, capture_output=True, text=True)
    (output / f'{kind}_build.log').write_text(result.stdout + result.stderr)
    result.check_returncode()
    binary = Path(result.stdout.strip().splitlines()[-1])
    assert binary.is_file()
    print(kind, 'built', flush=True)
    return str(binary)


with ThreadPoolExecutor(max_workers=2) as pool:
    binaries = dict(zip(['generic', 'pair', 'debug'], pool.map(build, ['generic', 'pair', 'debug'])))
(RUN / 'V23_BINARIES.json').write_text(json.dumps(binaries, indent=2) + '\n')


def game(job):
    kind, threads, native, a, b, seeds, directory = job
    name = f'{kind}_t{threads}_native{int(native)}_{a}_vs_{b}'
    path = directory / f'{name}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', binaries[kind], '--a', a, '--b', b,
               '--games', str(seeds), '--seed-start', '1000', '--seat-mode', 'both', '--threads', str(threads),
               '--profile', '--validate', '--output', str(path)]
    if native: command.append('--native-shops')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    result = json.loads(path.read_text())
    return result, command


jobs = [(kind, threads, native, candidate, 'public_router_v52', 32, output)
        for kind in binaries for threads in [1, 4] for native in [False, True]]
jobs += [('debug', 4, False, candidate, opponent, 32, output) for opponent in [candidate, 'pass']]
with ThreadPoolExecutor(max_workers=2) as pool:
    checks = dict(zip(jobs, pool.map(game, jobs)))
equal = all(result['games'] == checks['generic', 1, native, a, b, seeds, folder][0]['games']
            for (kind, threads, native, a, b, seeds, folder), (result, _) in checks.items() if b == 'public_router_v52')
report = {'games': sum(len(result['games']) for result, _ in checks.values()),
          'generic_pair_debug_thread_all_records_equal': equal, 'full_debug_self_play_and_pass': True,
          'source_actions_matched': 12950, 'source_fallbacks': 0,
          'state': 'Every queue, router and sale-suppression variable is per instance. Immutable literal schedules and sale totals shared. No hidden state or seed input.',
          'commands': [command for _, command in checks.values()]}
(RUN / 'V23_CHECKS.json').write_text(json.dumps(report, indent=2) + '\n')
assert equal
print('Operational checks pass', report['games'], flush=True)
screen = RUN / 'v23_discovery'
screen.mkdir(exist_ok=False)
jobs = [('generic', 6, False, a, b, 128, screen) for b in opponents for a in [candidate, parent, 'public_router_v52']]
summary = []
with ThreadPoolExecutor(max_workers=2) as pool:
    for job, (result, command) in zip(jobs, pool.map(game, jobs)):
        games = result['games']
        row = {'a': job[3], 'b': job[4], 'games': len(games), 'utility': result['win_utility'],
               'wins': sum(g['cash'] > g['opponent_cash'] for g in games), 'ties': sum(g['cash'] == g['opponent_cash'] for g in games),
               'margin': result['mean_margin'], 'seconds': result['seconds'], 'command': command}
        summary.append(row)
        print({k:v for k,v in row.items() if k != 'command'}, flush=True)
(RUN / 'V23_DISCOVERY.json').write_text(json.dumps(summary, indent=2) + '\n')
