from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
names = json.loads((RUN / 'LINEAGE.json').read_text())['variants']
opponents = ['empty_sale_slots_m2','public_router','joint_routes_p362_m0','day_program_p362_m3','pass']
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
        '--games', '16', '--seed-start', '1000', '--seat-mode', 'both', '--threads', '4',
        '--budget-expansions', '100000', '--validate', '--profile', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    result = json.loads(path.read_text())
    print(name, opponent, 'cash', result['mean_cash'], 'margin', result['mean_margin'], flush=True)
    return command


jobs = [(name,opponent) for opponent in opponents for name in names]
with ThreadPoolExecutor(max_workers=2) as pool:
    commands=list(pool.map(run,jobs))
(RUN/'COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
print('640 full actual-state service-bank discovery games complete.',flush=True)
