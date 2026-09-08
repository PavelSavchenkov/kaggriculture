"""Compare both repair components across a diverse exposed panel."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
binary = EXP / 'build/829d31f28900630354ba/arena'
assert binary.is_file()
output = RUN / 'discovery'
output.mkdir(exist_ok=False)
agents = ['wool_contract_repair_v2', 'rival_wool_purchase_repair_v1', 'rival_wool_context_v3']
opponents = ['rival_wool_context_v3', 'public_router_v52', 'teammate_shoprouter', 'public_router',
             'junghoon_wool_sales', 'john_131', 'public_router_v5']
jobs = [(a, b, False, 1000, 64) for b in opponents for a in agents]
jobs += [(a, 'public_router_v52', True, 1954000, 128) for a in agents]
commands = []


def run(job):
    a, b, native, seed, count = job
    path = output / f'{a}_vs_{b}_native{int(native)}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', a, '--b', b,
               '--games', str(count), '--seed-start', str(seed), '--seat-mode', 'both', '--threads', '6',
               '--profile', '--validate', '--output', str(path)]
    if native: command.append('--native-shops')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
    print(a, 'vs', b, 'native', native, 'complete', flush=True)
    return {'command': command, 'returncode': 0}


with ThreadPoolExecutor(max_workers=2) as pool:
    commands = list(pool.map(run, jobs))
(RUN / 'DISCOVERY_PROCESSES.json').write_text(json.dumps(commands, indent=2) + '\n')
