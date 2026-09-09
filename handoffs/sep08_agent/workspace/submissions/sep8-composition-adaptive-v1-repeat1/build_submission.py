"""Deterministically compile the frozen C++ tables and exact Python adapter."""
import base64
import gzip
import hashlib
import io
import json
import re
import tarfile
import zlib
from pathlib import Path

OUT = Path(__file__).resolve().parent
TREE = OUT / 'source_tree'
EXP = Path('experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source')
data = json.loads((OUT / 'exported_data.json').read_text())
source = TREE / EXP / 'runs/shop_herd_combinations_001/proposals/shop_herd_s6_m3_g1/source/agent.hpp'
edits = re.findall(r'AnimalEdit\{(\d+),(\d+),\{(\d+),(\d+)\},\{(\d+),(\d+)\},\{(\d+),(\d+)\},\{(\d+),(\d+)\},(\d+)\}', source.read_text())
assert len(edits) == 2
data['plans'] = []
for edit in edits:
    values = list(map(int, edit))
    assert values[:2] == [10, 10]
    data['plans'].append(dict(zip(['purchase', 'pickup', 'placement', 'structure'], [values[i:i + 2] for i in (2, 4, 6, 8)])))
assert data['wool_days'] == data.pop('wool_v2_days')
assert len(data['tape']) == len(data['wool_tape']) == 719
assert len(data['groups']) == 2 and len(data['portfolio']) == 4
plans = data['days'] + data['shop_days'] + data['context_days'] + [e['plan'] for e in data['entries']]
plans += data['fert_days'] + sum(data['rotation'], []) + sum(data['wheat'], []) + data['wool_days'] + [data['repair']]
plans += [p for c in data['portfolio'] for leaf in c for p in leaf['days']]
plans += [p for family in data['groups'] for choice in family['choices'] for leaf in choice for p in leaf['days']]
for units, orders in data['tape'] + data['wool_tape'] + [a for p in plans for a in p['actions']]:
    assert 1 <= len(units) <= 241 and len(orders) <= 10
    assert all(len(u) == 3 and 0 <= u[0] < 18 for u in units)
    assert all(len(o) == 3 and 0 <= o[0] <= 6 for o in orders)
template = (OUT / 'previous_policy_template.py').read_text()
template = template.replace('def agent(observation, configuration=None):', 'def _investment_action(observation, configuration=None):')
template = template[:template.rindex('    actions = [[_OPS[op]')] + '    return units, orders\n\n'
template += (OUT / 'policy_layers.py').read_text() + '\n' + (OUT / 'policy_animals.py').read_text()
(OUT / 'policy_template.py').write_text(template)
raw = json.dumps(data, sort_keys=True, separators=(',', ':')).encode()
(OUT / 'policy_data.json').write_bytes(raw + b'\n')
assert template.count('__POLICY_PAYLOAD__') == 1
main = template.replace('__POLICY_PAYLOAD__', base64.b85encode(zlib.compress(raw, 9)).decode()).encode()
compile(main, str(OUT / 'main.py'), 'exec')
(OUT / 'main.py').write_bytes(main)
with (OUT / 'submission.tar.gz').open('wb') as output:
    with gzip.GzipFile(filename='', mode='wb', fileobj=output, mtime=0) as compressed:
        with tarfile.open(fileobj=compressed, mode='w', format=tarfile.PAX_FORMAT) as archive:
            info = tarfile.TarInfo('main.py')
            info.size, info.mode, info.mtime = len(main), 0o644, 0
            archive.addfile(info, io.BytesIO(main))
report = {n: {'bytes': (OUT / n).stat().st_size, 'sha256': hashlib.sha256((OUT / n).read_bytes()).hexdigest()} for n in ('main.py', 'policy_data.json', 'submission.tar.gz')}
(OUT / 'ARTIFACTS.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
