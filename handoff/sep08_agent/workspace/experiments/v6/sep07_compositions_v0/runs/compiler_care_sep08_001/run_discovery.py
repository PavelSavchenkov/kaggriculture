from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
spec = json.loads((RUN / 'LINEAGE.json').read_text())
names = spec['variants']
opponents = spec['discovery']['opponents']
command = ['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP/'scripts/build_arena.py'), '--agents', *names, *opponents]
result = subprocess.run(command, capture_output=True, text=True)
(RUN/'build.log').write_text(result.stdout+result.stderr)
result.check_returncode()
binary = result.stdout.strip().splitlines()[-1]
(RUN/'BUILD.json').write_text(json.dumps({'command': command, 'binary': binary}, indent=2)+'\n')
output = RUN/'discovery'
output.mkdir(exist_ok=False)


def run(job):
    a, b = job
    path = output/f'{a}_vs_{b}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', binary, '--a', a, '--b', b,
               '--games', '8', '--seed-start', '1000', '--seat-mode', 'both', '--threads', '4',
               '--validate', '--profile', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    d = json.loads(path.read_text())
    print(a, b, 'cash', d['mean_cash'], 'margin', d['mean_margin'], flush=True)
    return command


with ThreadPoolExecutor(max_workers=2) as pool:
    commands = list(pool.map(run, [(a,b) for b in opponents for a in names]))
(RUN/'COMMANDS.json').write_text(json.dumps(commands, indent=2)+'\n')
