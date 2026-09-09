"""Check the shared pre-branch route against the independently compiled on leaf."""
from pathlib import Path
from datetime import datetime, timezone
import json
import subprocess
import argparse

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--report-only', action='store_true')
args = parser.parse_args()
changes = {}
for line in (RUN / 'on_terminal_no_care_audit.txt').read_text().splitlines():
    day, path = line.split()
    changes[int(day)] = Path(path)
off = {}
for line in (RUN / 'off_terminal_no_care_audit.txt').read_text().splitlines():
    day, path = line.split()
    off[int(day)] = Path(path)
for day in range(13, 20):
    changes[day] = off.get(day, EXP / f'runs/late_animal_rotation_001/goose_c31_d13_off_002/days/{day}/actions.txt')
mapping = RUN / 'on_shared_prefix_audit.txt'
mapping.write_text(''.join(f'{day} {path}\n' for day, path in sorted(changes.items())))
output = RUN / 'on_shared_prefix_audit'
command = ['conda', 'run', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'),
           str(RUN / 'build/audit_on'), str(EXP / 'runs/late_animal_rotation_001/goose_c31_d13_on_002'),
           str(mapping), str(output)]
if not args.report_only:
    with (RUN / 'on_shared_prefix_audit.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
expected = json.loads((output / 'candidate.json').read_text())['games']
actual = json.loads((RUN / 'discovery/late_goose_optimized_on_vs_public_router.json').read_text())['games']
old = json.loads((RUN / 'on_terminal_no_care_audit/candidate.json').read_text())['games']
economic_keys = ['cash', 'opponent_cash', 'produced', 'sold', 'discarded', 'unit_faults', 'worker_days', 'opponent_action_hash']
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'games': len(actual),
          'full_records_equal': actual == expected,
          'original_on_leaf_economics_equal': all(a[k] == b[k] for a, b in zip(actual, old) for k in economic_keys),
          'original_on_leaf_changed_fields': [k for k in actual[0] if any(a[k] != b[k] for a, b in zip(actual, old))],
          'original_on_leaf_changed_profile_fields': [k for k in actual[0]['profile'] if any(a['profile'][k] != b['profile'][k] for a, b in zip(actual, old))],
          'explanation': 'The complete adaptive policy uses off routes on days13..19, then selects off/on at day20. The independent forced-on compiler used equivalent but different unit routes before day20. The new audit uses the actual shared prefix. No policy change was made.',
          'command': command}
(RUN / 'PACKAGE_PREFIX_PARITY.json').write_text(json.dumps(report, indent=2) + '\n')
assert report['full_records_equal'] and report['original_on_leaf_economics_equal'], report
print('Shared-prefix on leaf: all64 full records exact. Original on leaf differs in route hash and action/stock-flow profiles; economic totals are equal.')
