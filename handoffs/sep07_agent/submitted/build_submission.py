import base64
import gzip
import hashlib
import io
import json
import re
import subprocess
import tarfile
import zlib
from pathlib import Path

OUT = Path(__file__).resolve().parent
EXP = Path('experiments/v6/sep07_compositions_v0')
data = json.loads(subprocess.check_output(['conda', 'run', '-n', 'kaggriculture', str(OUT / 'export_policy')], text=True))
source = OUT / 'source_tree' / EXP / 'runs/shop_herd_combinations_001/proposals/shop_herd_s6_m3_g1/source/agent.hpp'
edits = re.findall(r'AnimalEdit\{(\d+),(\d+),\{(\d+),(\d+)\},\{(\d+),(\d+)\},\{(\d+),(\d+)\},\{(\d+),(\d+)\},(\d+)\}', source.read_text())
assert len(edits) == 2
data['plans'] = []
for edit in edits:
    values = list(map(int, edit))
    assert values[:2] == [10, 10]
    data['plans'].append(dict(zip(['purchase', 'pickup', 'placement', 'structure'], [values[i:i + 2] for i in (2, 4, 6, 8)])))
assert len(data['tape']) == 719 and len(data['days']) == 22
for action in data['tape'] + [a for d in data['days'] for a in d['actions']]:
    assert len(action) == 2 and 0 < len(action[0]) <= 241 and len(action[1]) <= 10
    assert all(len(u) == 3 and 0 <= u[0] < 18 for u in action[0])
    assert all(len(o) == 3 and 0 <= o[0] <= 6 for o in action[1])
raw = json.dumps(data, separators=(',', ':'), sort_keys=True).encode()
(OUT / 'policy_data.json').write_bytes(raw + b'\n')
payload = base64.b85encode(zlib.compress(raw, 9)).decode()
template = (OUT / 'policy_template.py').read_text()
assert template.count('__POLICY_PAYLOAD__') == 1
main = template.replace('__POLICY_PAYLOAD__', payload).encode()
compile(main, str(OUT / 'main.py'), 'exec')
(OUT / 'main.py').write_bytes(main)
with (OUT / 'submission.tar.gz').open('wb') as target:
    with gzip.GzipFile(filename='', mode='wb', fileobj=target, mtime=0) as compressed:
        with tarfile.open(fileobj=compressed, mode='w', format=tarfile.PAX_FORMAT) as archive:
            info = tarfile.TarInfo('main.py')
            info.size = len(main)
            info.mode = 0o644
            info.mtime = 0
            archive.addfile(info, io.BytesIO(main))
report = {name: {'bytes': (OUT / name).stat().st_size, 'sha256': hashlib.sha256((OUT / name).read_bytes()).hexdigest()} for name in ('main.py', 'submission.tar.gz', 'policy_data.json')}
(OUT / 'ARTIFACTS.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
