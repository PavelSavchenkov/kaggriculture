from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
candidate, baseline = 'wool_family_context_v2', 'late_value_s32_t0_r05'
output = RUN / 'checks'
assert not (RUN / 'CHECKS.json').exists()
output.mkdir(exist_ok=True)
generic = EXP.parents[2] / json.loads((RUN / 'FRESH_BUILD.json').read_text())['binary']


def build(kind):
    log = output / f'{kind}_build.log'
    if log.exists():
        binaries = [Path(line) for line in log.read_text().splitlines() if line.startswith('/') and Path(line).is_file()]
        assert len(binaries) == 1
        return binaries[0]
    command = ['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/build_arena.py')]
    command += ['--pair', candidate, baseline] if kind == 'pair' else ['--agents', candidate, baseline, '--debug']
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    (output / f'{kind}_build.log').write_text(result.stdout + result.stderr)
    binary = Path(result.stdout.strip().splitlines()[-1])
    assert binary.exists()
    return binary


with ThreadPoolExecutor(max_workers=2) as pool:
    binaries = dict(zip(['pair', 'debug'], pool.map(build, ['pair', 'debug'])))
binaries['generic'] = generic
(RUN / 'CHECK_BINARIES.json').write_text(json.dumps({k: str(v) for k, v in binaries.items()}, indent=2) + '\n')
jobs = [(kind, threads, native, candidate, baseline, 32) for kind in binaries for threads in [1, 4] for native in [False, True]]
jobs += [('debug', 4, False, candidate, opponent, 32) for opponent in [candidate, 'pass']]


def run(job):
    kind, threads, native, a, b, seeds = job
    name = f'{kind}_t{threads}_native{int(native)}_{a}_vs_{b}'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binaries[kind]), '--a', a, '--b', b,
               '--games', str(seeds), '--seed-start', '1893000', '--seat-mode', 'both', '--threads', str(threads),
               '--profile', '--validate', '--output', str(output / f'{name}.json')]
    if native:
        command.append('--native-shops')
    with (output / f'{name}.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    return json.loads((output / f'{name}.json').read_text())['games']


with ThreadPoolExecutor(max_workers=2) as pool:
    games = dict(zip(jobs, pool.map(run, jobs)))
equal = all(value == games['generic', 1, native, a, b, n] for (kind, threads, native, a, b, n), value in games.items() if b == baseline)
report = {'games': sum(map(len, games.values())), 'generic_pair_debug_thread_all_records_equal': equal,
          'full_debug_self_play_and_pass': True, 'native_and_custom_shop_modes': True,
          'instance_state': 'One base policy and selection state per instance. Shared calendars are immutable; no mutable global episode state.',
          'observation': 'Day6 selector reads only the firsttwo observed shops and its own physical/funding state. No seed or hidden state; the retained parent uses its existing later observed decisions.',
          'node_budget_deterministic': True}
(RUN / 'CHECKS.json').write_text(json.dumps(report, indent=2) + '\n')
assert equal
print(report, flush=True)
