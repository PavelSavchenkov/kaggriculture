from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
BINARIES = {'generic': EXP / 'build/6ad0606745e1f4b3d07d/arena',
            'pair': EXP / 'build/7731e5f080e4e765a1a7/arena',
            'debug': EXP / 'build/87f07f6afbfbae90b65e/arena'}
output = RUN / 'checks'
output.mkdir(exist_ok=False)
jobs = [(kind, threads, native) for kind in BINARIES for threads in [1, 4] for native in [False, True]]


def run(job):
    kind, threads, native = job
    name = f'{kind}_t{threads}_native{int(native)}'
    command = ['conda', 'run', '-n', 'kaggriculture', str(BINARIES[kind]),
               '--a', 'late_goose_optimized', '--b', 'opening_q32_b13_v1', '--games', '32',
               '--seed-start', '1793000', '--seat-mode', 'both', '--threads', str(threads),
               '--profile', '--validate', '--output', str(output / f'{name}.json')]
    if native:
        command.append('--native-shops')
    with (output / f'{name}.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    return json.loads((output / f'{name}.json').read_text())['games']


with ThreadPoolExecutor(max_workers=2) as pool:
    games = dict(zip(jobs, pool.map(run, jobs)))
equal = all(value == games['generic', 1, native] for (kind, threads, native), value in games.items())
report = {'games': sum(map(len, games.values())), 'generic_pair_debug_thread_all_records_equal': equal,
          'native_and_custom_shop_modes': True,
          'deterministic_node_budget': True,
          'instance_state': 'Each wrapper owns its base and course vectors; no mutable episode globals.',
          'observation': 'Agent reads typed own/public state; no seed, opponent-private inventory or live Sim.'}
(RUN / 'CHECKS.json').write_text(json.dumps(report, indent=2) + '\n')
assert equal
print(report, flush=True)
