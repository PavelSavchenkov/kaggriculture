"""Frozen candidate, prior broad panel and unused seeds; no policy fitting here."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
spec = json.loads((RUN / 'FRESH_PREREGISTERED.json').read_text())
opponents = spec['opponents']
agents = [spec['candidate'], spec['baseline']]
amendment_path = RUN / 'PACKAGING_AMENDMENT.json'
amendment = json.loads(amendment_path.read_text()) if amendment_path.exists() else None
for relative, expected in spec['candidate_files_sha256'].items():
    if amendment and relative == amendment['path']:
        assert expected == amendment['before_sha256']
        expected = amendment['after_sha256']
    assert hashlib.sha256((EXP / relative).read_bytes()).hexdigest() == expected
build_command = ['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/build_arena.py'), '--agents', *sorted(set(agents + opponents))]
build = subprocess.run(build_command, capture_output=True, text=True, check=True)
(RUN / 'fresh_build.log').write_text(build.stdout + build.stderr)
binary = Path(build.stdout.strip().splitlines()[-1])
assert binary.exists()
(RUN / 'FRESH_BUILD.json').write_text(json.dumps({'command': build_command, 'binary': str(binary.relative_to(ROOT))}, indent=2) + '\n')
output = RUN / 'fresh'
output.mkdir(exist_ok=False)


def run(job):
    agent, opponent = job
    path = output / f'{agent}_vs_{opponent}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', agent, '--b', opponent,
               '--games', str(spec['seeds']), '--seed-start', str(spec['seed_start']), '--seat-mode', 'both',
               '--threads', '6', '--validate', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    print(agent, 'vs', opponent, 'complete', flush=True)
    return {'agent': agent, 'opponent': opponent, 'command': command, 'returncode': 0}


jobs = [(agent, opponent) for opponent in opponents for agent in agents]
with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, jobs))
(RUN / 'FRESH_PROCESS_RESULTS.json').write_text(json.dumps(results, indent=2) + '\n')
