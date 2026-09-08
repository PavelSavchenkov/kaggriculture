from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
spec = json.loads((RUN / 'FRESH_PREREGISTERED.json').read_text())
fresh = json.loads((RUN / 'FRESH_ANALYSIS.json').read_text())
native = json.loads((RUN / 'NATIVE_ANALYSIS.json').read_text())
checks = json.loads((RUN / 'CHECKS.json').read_text())
audits = json.loads((RUN / 'FINAL_AUDITS.json').read_text())
frozen = json.loads((RUN / 'frozen/FROZEN.json').read_text())
for p, digest in spec['candidate_files_sha256'].items():
    assert hashlib.sha256((EXP / p).read_bytes()).hexdigest() == digest, p
for p, digest in frozen['files_sha256'].items():
    assert hashlib.sha256((ROOT / p).read_bytes()).hexdigest() == digest, p
assert all(fresh['gates'].values()) and all(native['gates'].values())
assert checks['generic_pair_debug_thread_all_records_equal'] and checks['node_budget_deterministic']
assert audits['frozen_full_records_equal'] and audits['frozen_games'] == 64
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'candidate': spec['candidate'], 'parent': spec['baseline'],
    'promoted': False, 'decision': 'All declared gates pass. Retain pending a new independent population comparison with unguarded empty_sale_slots_m2; the guarded version loses that direct matchup and passing a stricter rule does not establish greater playing strength.',
    'fresh': fresh, 'native': native, 'operational': checks, 'frozen': audits,
    'acceptance': {'all_declared_gates_passed': True, 'selected_as_strongest': False},
    'source_hashes_unchanged': True, 'source_files': len(spec['candidate_files_sha256']), 'frozen_dependencies': len(frozen['files_sha256']),
    'unguarded_duel': fresh['metrics'][spec['candidate']]['empty_sale_slots_m2']}
output = EXP / 'results/empty_sale_floor_m1_validation.json'
with output.open('x') as out:
    json.dump(report, out, indent=2);out.write('\n')
print('Completed guard audit retained, not promoted:', report['unguarded_duel'])
