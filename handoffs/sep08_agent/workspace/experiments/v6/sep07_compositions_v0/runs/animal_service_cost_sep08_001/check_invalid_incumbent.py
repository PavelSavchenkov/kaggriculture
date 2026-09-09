"""A saved schedule with removed worker actions must fail revalidation."""
import csv
import json
import subprocess
from pathlib import Path

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]
OUT = RUN / 'invalid_incumbent'
cache = OUT / 'cache/days/15'
cache.mkdir(parents=True, exist_ok=False)
source = RUN / 'retained_incumbents/1014/days/15/actions.txt'
lines = []
for line in source.read_text().splitlines():
    values = list(map(int, line.split()))
    units, orders = values[:2]
    assert len(values) == 2 + 3 * (units + orders)
    for unit in range(units):
        values[2 + 3 * unit:5 + 3 * unit] = [0, 0, 1]
    lines.append(' '.join(map(str, values)))
(cache / 'actions.txt').write_text('\n'.join(lines) + '\n')
command = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'),
    str(RUN / 'build/care_minimize'), str(OUT / 'compiled'), '1014', str(RUN / 'cow_triple_1014.txt'),
    '0', '-', str(OUT / 'cache')]
with (OUT / 'run.log').open('w') as log:
    result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
assert result.returncode == 4
rows = list(csv.DictReader((OUT / 'compiled/incumbents.csv').open()))
matched = [row for row in rows if row['orders_match'] == '1']
assert matched and all(row['physical_valid'] == row['endpoint_valid'] == '0' for row in matched)
(OUT / 'RESULTS.json').write_text(json.dumps({'command': command, 'returncode': result.returncode,
    'same_orders': True, 'invalid_worker_schedule_rejected': True}, indent=2) + '\n')
print('Invalid saved worker schedule rejected despite matching orders.')
