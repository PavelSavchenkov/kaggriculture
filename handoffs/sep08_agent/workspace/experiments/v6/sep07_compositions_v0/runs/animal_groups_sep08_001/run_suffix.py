"""Run a new compiler/budget against selected, revalidated matched prefixes."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import argparse
import hashlib
import json
import shlex
import subprocess
import time

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]
parser = argparse.ArgumentParser()
parser.add_argument('--binary', required=True)
parser.add_argument('--old', required=True)
parser.add_argument('--output', required=True)
parser.add_argument('--names', nargs='+', required=True)
parser.add_argument('--seconds', type=float, default=30)
parser.add_argument('--parallel', type=int, default=2)
args = parser.parse_args()
old = RUN / args.old
previous = json.loads((old / 'EXECUTION.json').read_text())
assert previous['sources_unchanged']
selected = {job['name']: job for job in previous['cases']}
assert set(args.names) <= selected.keys()
out = RUN / args.output
out.mkdir(exist_ok=False)
binary = RUN / 'build' / args.binary
files = set()
target = RUN / 'build' / 'CMakeFiles' / f'{args.binary}.dir'
dependencies = list(target.rglob('*.o.d')) + list((RUN / 'build/day_solver').rglob('*.o.d'))
for dep in dependencies:
    for entry in shlex.split(dep.read_text().replace('\\\n', ' ').split(':', 1)[1]):
        path = Path(entry).resolve()
        if path.is_relative_to(ROOT):
            files.add(path)
assert files and binary.exists()
files.update([target/'flags.make', target/'link.txt'])
def hashes():
    return {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
before = hashes()
jobs = []
for name in args.names:
    original = selected[name]
    resume = old / name
    command = list(original['command'])
    command[6:8] = [str(binary), str(out / name)]
    command[10:] = [str(args.seconds), str(resume)]
    prefixes = sorted((resume / 'days').glob('*/actions.txt'))
    assert prefixes
    jobs.append({'name': name, 'seed': original['seed'], 'command': command, 'estimate': original['estimate'],
        'prefix_files': {str(p.relative_to(RUN)): hashlib.sha256(p.read_bytes()).hexdigest() for p in prefixes}})
(out / 'PROTOCOL.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'cases': jobs, 'sources': before, 'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
    'scope': 'Matched exposed worlds only. Re-execute each old prefix against the live opponent; no league or runtime-policy claim.'}, indent=2) + '\n')
def run(job):
    start = time.monotonic()
    with (out / (job['name'] + '.log')).open('x') as log:
        result = subprocess.run(job['command'], stdout=log, stderr=subprocess.STDOUT)
    record = dict(job, returncode=result.returncode, seconds=time.monotonic() - start)
    (out / (job['name'] + '_EXECUTION.json')).write_text(json.dumps(record, indent=2) + '\n')
    print(job['name'], result.returncode, round(record['seconds'], 2), flush=True)
    return record
with ThreadPoolExecutor(max_workers=args.parallel) as pool:
    records = list(pool.map(run, jobs))
assert hashes() == before
(out / 'EXECUTION.json').write_text(json.dumps({'cases': records, 'sources_unchanged': True}, indent=2) + '\n')
