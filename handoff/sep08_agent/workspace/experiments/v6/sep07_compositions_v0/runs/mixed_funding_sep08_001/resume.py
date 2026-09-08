"""Continue the exact mixed course after funding its originally planned second goose."""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
prior = EXP / 'runs/animal_forecast_error_sep08_001'
compiler = EXP / 'runs/animal_group_policy_sep08_001'
prefix = RUN / 'prefix'
prefix.mkdir(exist_ok=False)
sources = [prior / 'mixed_certificate/days/9', RUN / 'day10/hour10/days/10']
hashes = {}
for source in sources:
    assert json.loads((source / 'STATUS.json').read_text())['physical_and_live_endpoint']
    dest = prefix / 'days' / source.name
    dest.mkdir(parents=True)
    for name in ('actions.txt', 'problem.json'):
        shutil.copyfile(source / name, dest / name)
        hashes[str((source / name).relative_to(EXP))] = hashlib.sha256((source / name).read_bytes()).hexdigest()
out = RUN / 'season_30s'
assert not out.exists()
command = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'),
    str(compiler / 'build/compile_flexible'), str(out), '1008',
    str(compiler / 'flexible_mixed_30s/mixed_d9.txt'), '30', str(prefix)]
(RUN / 'RESUME_PROTOCOL.json').write_text(json.dumps({'command': command, 'source_sha256': hashes,
    'compiler_sha256': hashlib.sha256((compiler / 'source/compile_flexible.cpp').read_bytes()).hexdigest(),
    'binary_sha256': hashlib.sha256((compiler / 'build/compile_flexible').read_bytes()).hexdigest(),
    'change': 'Day9 retains the certified two-wheat late refill. Day10 splits inherited two-goose purchase across hours1/10; unchanged worker actions now complete the second placement/feed/care.',
    'scope': 'Fixed-calendar full-course construction, not runtime policy evidence.'}, indent=2) + '\n')
with (RUN / 'season.log').open('w') as log:
    result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
(RUN / 'RESUME_EXECUTION.json').write_text(json.dumps({'command': command, 'returncode': result.returncode}, indent=2) + '\n')
result.check_returncode()
print('Mixed season compiled; independent719-turn audit remains necessary.')
