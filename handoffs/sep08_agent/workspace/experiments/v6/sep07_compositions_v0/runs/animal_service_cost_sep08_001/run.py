"""Build and run four full-course service/workforce comparisons."""
from datetime import datetime, timezone
from pathlib import Path
import json
import subprocess
import time

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]
lineage = json.loads((RUN / 'LINEAGE.json').read_text())
out = RUN / 'compiled_30s'
out.mkdir(exist_ok=False)
builds = [
    ['conda', 'run', '-n', 'kaggriculture', 'cmake', '-S', str(RUN), '-B', str(RUN / 'build')],
    ['conda', 'run', '-n', 'kaggriculture', 'cmake', '--build', str(RUN / 'build'), '--target', 'care_only', 'care_minimize', '-j', '2'],
]
jobs = []
for mode in ('care_only', 'care_minimize'):
    for case in lineage['cases']:
        name = f"{mode}_{case['seed']}"
        command = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'),
            str(RUN / 'build' / mode), str(out / name), str(case['seed']), case['spec'], '30']
        jobs.append({'name': name, 'command': command, 'baseline': case['baseline'], 'mode': mode, 'seed': case['seed']})
(out / 'PROTOCOL.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'builds': builds, 'jobs': jobs, 'scope': 'Compare full matched seasons and output/labor. A solver timeout is not infeasibility; no runtime promotion from fixed worlds.'}, indent=2) + '\n')
for i, command in enumerate(builds):
    with (RUN / f'build_{i}.log').open('x') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
records = []
for job in jobs:
    start = time.monotonic()
    with (out / f"{job['name']}.log").open('x') as log:
        result = subprocess.run(job['command'], stdout=log, stderr=subprocess.STDOUT)
    record = dict(job, returncode=result.returncode, seconds=time.monotonic() - start)
    records.append(record)
    (out / f"{job['name']}_EXECUTION.json").write_text(json.dumps(record, indent=2) + '\n')
    print(job['name'], result.returncode, flush=True)
(out / 'EXECUTION.json').write_text(json.dumps({'cases': records}, indent=2) + '\n')
