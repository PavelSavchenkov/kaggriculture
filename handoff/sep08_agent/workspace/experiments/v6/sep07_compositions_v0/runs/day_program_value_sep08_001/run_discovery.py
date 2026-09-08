from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
names = json.loads((RUN / 'LINEAGE.json').read_text())['variants']
parents = ['joint_routes_p355_m0', 'joint_routes_p362_m0']
opponents = ['public_router', 'observed_sale_lead_start_216', *parents, 'pass']
command = ['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/build_arena.py'), '--agents', *names, *opponents]
result = subprocess.run(command, capture_output=True, text=True)
(RUN / 'build.log').write_text(result.stdout+result.stderr)
result.check_returncode()
binary = result.stdout.strip().splitlines()[-1]
(RUN / 'BUILD.json').write_text(json.dumps({'command': command, 'binary': binary}, indent=2) + '\n')
out = RUN / 'discovery'
out.mkdir(exist_ok=False)


def run(job):
    name, opponent = job
    path = out / f'{name}_vs_{opponent}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', binary, '--a', name, '--b', opponent,
        '--games', '8', '--seed-start', '1000', '--seat-mode', 'both', '--threads', '4',
        '--budget-expansions', '100000', '--validate', '--profile', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    result = json.loads(path.read_text())
    print(name, opponent, 'cash', result['mean_cash'], 'margin', result['mean_margin'], flush=True)
    return command


jobs = []
for case in ['p355', 'p362']:
    for opponent in ['public_router', 'observed_sale_lead_start_216', f'joint_routes_{case}_m0', 'pass']:
        jobs += [(f'day_value_{case}_m{mode}', opponent) for mode in range(4)]
with ThreadPoolExecutor(max_workers=2) as pool:
    commands = list(pool.map(run, jobs))
(RUN / 'COMMANDS.json').write_text(json.dumps(commands, indent=2) + '\n')
print('512 full day-program discovery games complete.', flush=True)
