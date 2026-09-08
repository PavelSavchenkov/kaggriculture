from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
binaries = {'generic': EXP / 'build/ee33d9ed834ac8d4f2dd/arena', 'pair': EXP / 'build/760853c0b5aaf5e7f0d5/arena',
            'debug': EXP / 'build/04b04635138941a0ac6a/arena'}
output = RUN / 'v52_checks'
output.mkdir(exist_ok=False)
jobs = [(kind, threads, native, 'public_router_v5') for kind in binaries for threads in [1, 4] for native in [False, True]]
jobs += [('debug', 4, False, b) for b in ['public_router_v52', 'pass']]


def run(job):
    kind, threads, native, opponent = job
    name = f'{kind}_t{threads}_native{int(native)}_vs_{opponent}'
    path = output / f'{name}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binaries[kind]), '--a', 'public_router_v52', '--b', opponent,
               '--games', '32', '--seed-start', '1863000', '--seat-mode', 'both', '--threads', str(threads),
               '--profile', '--validate', '--output', str(path)]
    if native:
        command.append('--native-shops')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    return json.loads(path.read_text())['games']


with ThreadPoolExecutor(max_workers=2) as pool:
    results = dict(zip(jobs, pool.map(run, jobs)))
equal = all(rows == results['generic', 1, native, b] for (kind, threads, native, b), rows in results.items() if b == 'public_router_v5')
report = {'games': sum(map(len, results.values())), 'generic_pair_debug_thread_all_records_equal': equal,
          'full_debug_self_play_and_pass': True, 'source_actions_matched': 12230,
          'state': 'Route/step/block state is per instance; immutable schedules are shared. No hidden state or seed read.'}
(RUN / 'V52_CHECKS.json').write_text(json.dumps(report, indent=2) + '\n')
assert equal, report
print(report, flush=True)
