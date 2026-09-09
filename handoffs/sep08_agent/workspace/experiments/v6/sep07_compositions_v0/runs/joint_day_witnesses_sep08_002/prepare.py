from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
OLD = EXP / 'runs/joint_day_witnesses_sep08_001'
report = json.loads((OLD / 'RESULTS.json').read_text())
failed = [r for r in report['cases'] if not r['solved']]
assert len(failed) == 3 and all(r['agent'] == 'joint_routes_p362_m1' and r['day'] == 16 for r in failed)
text = (OLD / 'compile.cpp').read_text().replace('for(int day:{14,15,16})', 'for(int day:{16})')
text = text.replace('for(int augment:{0,1})for(int remove:{0,2}) {',
                    'for(int augment:{0,1})for(int remove:{0,2}) {\n            if(augment==1 && remove==2)continue;')
for name in ['joint_routes_p355_m0', 'joint_routes_p355_m1', 'joint_routes_p362_m0']:
    text = text.replace(f'    study<{name}::Agent>(out,"{name}",seconds);\n', '')
(RUN / 'compile.cpp').write_text(text)
(RUN / 'CMakeLists.txt').write_text((OLD / 'CMakeLists.txt').read_text().replace('project(joint_day_witnesses ', 'project(joint_day_witnesses_retry '))
(RUN / 'LINEAGE.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'reason': 'Retry only the three2-second timeouts with8seconds. All45 solved cases remain untouched. The augmented/two-fewer case on this same day already solved.',
    'failed_cases': failed, 'source': 'runs/joint_day_witnesses_sep08_001/compile.cpp',
    'source_sha256': hashlib.sha256((OLD / 'compile.cpp').read_bytes()).hexdigest(),
    'retry_sha256': hashlib.sha256((RUN / 'compile.cpp').read_bytes()).hexdigest()}, indent=2) + '\n')
(RUN / 'README.md').write_text('# Retry three unresolved day contracts\n\nprepare.py selects only the three p362 joint-route day16 cases that timed out at2seconds. run.py gives them8seconds, preserves old results and verifies each returned schedule in the full engine. No agent source or accepted strategy changes. See LINEAGE.json and ../joint_day_witnesses_sep08_001/README.md for scope.\n')
print('Prepared exactly three timeout retries.')
