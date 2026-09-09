from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
candidate, baseline = 'late_value_s32_t0_r05', 'opening_q32_b13_v1'
output = RUN / 'profiles'
output.mkdir(exist_ok=False)
binary = EXP / 'build/faace656c5bb3083d227/arena'
jobs = [(a, b) for b in [baseline, 'public_router'] for a in [candidate, baseline]]


def run(job):
    a, b = job
    path = output / f'{a}_vs_{b}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', a, '--b', b,
               '--games', '128', '--seed-start', '1850000', '--seat-mode', 'both', '--threads', '6',
               '--profile', '--validate', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    return {'a': a, 'b': b, 'command': command}


with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, jobs))
frozen = RUN / 'frozen'
command = ['conda', 'run', '-n', 'kaggriculture', str(frozen / 'arena_rebuilt'), '--a', candidate, '--b', baseline,
           '--games', '128', '--seed-start', '1853000', '--seat-mode', 'both', '--threads', '6',
           '--profile', '--validate', '--native-shops', '--output', str(frozen / 'native_parity.json')]
with (frozen / 'native_parity.log').open('w') as log:
    subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
actual = json.loads((frozen / 'native_parity.json').read_text())['games']
expected = json.loads((RUN / f'native/{candidate}_vs_{baseline}.json').read_text())['games']
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'profiles': results,
          'frozen_command': command, 'frozen_games': len(actual), 'frozen_full_records_equal': actual == expected}
(RUN / 'FINAL_AUDITS.json').write_text(json.dumps(report, indent=2) + '\n')
assert report['frozen_full_records_equal']
print('Four causal profiles complete; rebuilt frozen pair matches all256 native game records.', flush=True)
