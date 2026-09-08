"""Compare the fixed purchase retry with its parent on exposed scenarios."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess
import sys

RUN = Path(__file__).resolve().parent
binary = Path(sys.argv[1]).resolve()
assert binary.is_file()
(RUN / 'DISCOVERY_BUILD.json').write_text(json.dumps({'binary': str(binary)}, indent=2) + '\n')
output = RUN / 'discovery'
output.mkdir(exist_ok=False)
candidate, parent = 'rival_wool_purchase_repair_v1', 'rival_wool_context_v3'
jobs = [(a, native, seed, count) for a in [candidate, parent]
        for native, seed, count in [(False, 1000, 512), (True, 1954000, 128)]]


def run(job):
    a, native, seed, count = job
    path = output / f'{a}_native{int(native)}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', a, '--b', 'public_router_v52',
               '--games', str(count), '--seed-start', str(seed), '--seat-mode', 'both', '--threads', '6',
               '--profile', '--validate', '--output', str(path)]
    if native: command.append('--native-shops')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
    print(a, 'native', native, 'complete', flush=True)
    return {'command': command, 'returncode': 0}


with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, jobs))
(RUN / 'DISCOVERY_PROCESSES.json').write_text(json.dumps(results, indent=2) + '\n')
