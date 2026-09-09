from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('panel', choices=['fresh', 'native'])
args = parser.parse_args()
spec = json.loads((RUN / 'PREREGISTERED.json').read_text())
for relative, digest in spec['candidate_files_sha256'].items():
    assert hashlib.sha256((EXP / relative).read_bytes()).hexdigest() == digest, relative
for relative, digest in spec['frozen_root_files_sha256'].items():
    assert hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() == digest, relative
binary = ROOT / spec['binary']
assert hashlib.sha256(binary.read_bytes()).hexdigest() == spec['binary_sha256']
panel = spec[args.panel]
output = RUN / args.panel
output.mkdir(exist_ok=False)


def run(job):
    agent, opponent = job
    path = output / f'{agent}_vs_{opponent}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', agent, '--b', opponent,
        '--games', str(panel['seeds']), '--seed-start', str(panel['seed_start']), '--seat-mode', 'both',
        '--threads', '6' if args.panel == 'fresh' else '3', '--validate', '--output', str(path)]
    if args.panel == 'native':
        command += ['--profile']
        if opponent != 'pass': command += ['--native-shops']
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    print(args.panel, agent, 'vs', opponent, 'complete', flush=True)
    return command


with ThreadPoolExecutor(max_workers=2) as pool:
    commands = list(pool.map(run, [(a, o) for o in panel['opponents'] for a in [spec['candidate'], spec['baseline']]]))
(RUN / f'{args.panel.upper()}_COMMANDS.json').write_text(json.dumps(commands, indent=2) + '\n')
