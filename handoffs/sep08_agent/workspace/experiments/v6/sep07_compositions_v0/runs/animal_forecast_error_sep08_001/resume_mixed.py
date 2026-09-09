"""Continue all remaining days from the independently certified funded entry."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
compiler = EXP / 'runs/animal_group_policy_sep08_001'
prefix = RUN / 'mixed_certificate'
assert json.loads((prefix / 'days/9/STATUS.json').read_text())['physical_and_live_endpoint']
out = RUN / 'mixed_season_30s'
assert not out.exists()
command = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'),
    str(compiler / 'build/compile_flexible'), str(out), '1008',
    str(compiler / 'flexible_mixed_30s/mixed_d9.txt'), '30', str(prefix)]
files = [prefix / 'days/9/actions.txt', prefix / 'days/9/problem.json', compiler / 'source/compile_flexible.cpp']
(RUN / 'MIXED_SEASON_PROTOCOL.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'command': command, 'inputs': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in files},
    'scope': 'Reuse the independently certified mixedday9 prefix, then compile all remaining days. No runtime policy or economic improvement is implied by the repaired entry.'}, indent=2) + '\n')
with (RUN / 'mixed_season.log').open('x') as log:
    result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
(RUN / 'MIXED_SEASON_EXECUTION.json').write_text(json.dumps({'command': command, 'returncode': result.returncode}, indent=2) + '\n')
result.check_returncode()
print('Complete mixedday9 season compiled; independent final audit is next.')
