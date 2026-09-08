"""Reconstruct the audited public source and export its literal route data."""
from copy import deepcopy
from pathlib import Path
import base64
import hashlib
import json
import sys
import urllib.request
import zlib

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
sys.path.insert(0, str(EXP / 'scripts'))
from generate_public_routes import triple

target = EXP / 'league/ahmed_v23'
source_dir = target / 'source'
source_dir.mkdir(parents=True, exist_ok=True)
donor = EXP / 'research/refresh_2002/notebooks/thomastschinkel/kaggriculture-95-5-win-rate-via-replay-routing'
assert hashlib.sha256((donor / 'extracted_main.py').read_bytes()).hexdigest() == '8241246765098c50223d897739cb32076b71a303284d2e5e1c5f0fb495bb5a97'
tapes = json.loads((donor / 'payload.json').read_text())['schedules']
payload = {'base': deepcopy(tapes[0]), 'patches': {}}
for branch in range(1, 5):
    tapes[branch][:144] = deepcopy(tapes[0][:144])
    if branch in (3, 4): tapes[branch][144:288] = deepcopy(tapes[2][144:288])
    payload['patches'][str(branch)] = [[i, a] for i, a in enumerate(tapes[branch]) if a != tapes[0][i]]
blob = base64.b85encode(zlib.compress(json.dumps(payload, separators=(',', ':')).encode(), 9)).decode()
template = RUN / 'notebook_audit/ahmed/v23_template.py'
assert hashlib.sha256(template.read_bytes()).hexdigest() == '985d07220622767f8f90351c1dfd82c4b56c43407b3168692672e0435c26d841'
with urllib.request.urlopen('https://www.apache.org/licenses/LICENSE-2.0.txt', timeout=30) as response:
    license_text = response.read().decode().replace('\r\n', '\n')
assert hashlib.sha256(license_text.encode()).hexdigest() == 'cfc7749b96f63bd31c3c42b5c471bf756814053e847c10f3eb003417bc523d30'
notice = '# SPDX-License-Identifier: Apache-2.0\n'
notice += '# v23 modifications: public production router, audited Python chassis, and build tooling.\n'
notice += '# Credits: thomastschinkel, yhay81, tetsutani; offline simulator: destbreso/nikital7.\n'
notice += ''.join('# ' + line + '\n' for line in license_text.splitlines())
reference = notice + template.read_text().replace('__V23_ROUTE_BLOB__', repr(blob))
assert hashlib.sha256(reference.encode()).hexdigest() == '6eb728a40cc55f7e497add6ea2946ce38b0389c24531a208219d48587435838e'
(RUN / 'notebook_audit/ahmed/reference_v23.py').write_text(reference)
(target / 'LICENSE.txt').write_text(license_text)

offsets, values, future = [], [], []
for tape in tapes:
    starts = []
    sells = [[0] * 9 for _ in range(720)]
    for i in range(718, -1, -1):
        sells[i] = sells[i + 1].copy()
        for order in tape[i]['market']:
            if order[0] == 'SELL':
                _, item, count = triple(order, True)
                sells[i][item] += max(0, count)
    for action in tape:
        starts.append(len(values))
        units = [triple(a) for a in [action['farmer'], *action['hands']]]
        orders = [triple(a, True) for a in action['market']]
        values += [len(units), len(orders), *(v for a in units + orders for v in a)]
    offsets.append(starts)
    future.append(sells)
code = '// Literal Ahmed V23 routes derived from Thomas V5/2. See IMPORT.json.\n'
code += 'inline constexpr int offsets[5][719]={' + ',\n'.join('{' + ','.join(map(str, r)) + '}' for r in offsets) + '};\n'
code += 'inline constexpr int values[]={' + ',\n'.join(','.join(map(str, values[i:i+100])) for i in range(0, len(values), 100)) + '};\n'
code += 'inline constexpr int future[5][720][9]={' + ',\n'.join('{' + ','.join('{' + ','.join(map(str, s)) + '}' for s in r) + '}' for r in future) + '};\n'
(source_dir / 'data.inc').write_text(code)
(target / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': 'ahmed_v23', 'header': 'source/agent.hpp', 'type': 'kag::agents::ahmed_v23::Agent', 'sources': ['source/agent.cpp']}, indent=2) + '\n')
(target / 'IMPORT.json').write_text(json.dumps({'source': 'https://www.kaggle.com/code/ahmedberatozer/notebook865729c24e',
    'source_notebook': str((RUN / 'notebook_audit/ahmed/notebook865729c24e.ipynb').relative_to(EXP)),
    'reference': str((RUN / 'notebook_audit/ahmed/reference_v23.py').relative_to(EXP)),
    'reference_sha256': hashlib.sha256(reference.encode()).hexdigest(),
    'donor': 'Thomas V5/2, exact SHA8241246765098c50223d897739cb32076b71a303284d2e5e1c5f0fb495bb5a97',
    'credits': ['thomastschinkel production routes', 'yhay81 sale lead, budget and terminal ideas', 'tetsutani weed, room, clamp and dead-stock ideas', 'destbreso/nikital7 offline simulator'],
    'local_changes': ['Typed C++ port with immutable suffix-sale tables', 'Per-instance fixed queues, reset state and source-order market operations', 'Normalize invalid or zero-quantity final market orders to zero SELL slots'],
    'rating': 'Unknown. Author claims106W14Ltop,145W5Lopponents,104W4Lreactive; not verified.',
    'parity': 'Pending fixed-observation and full-game comparison.', 'optimization': 'Literal runtime port; not a promoted reference.',
    'reuse_restrictions': 'Notebook embeds Apache-2.0; retain LICENSE and attribution. Donor original replay episode IDs absent.'}, indent=2) + '\n')
(target / 'README.md').write_text('# ahmed_v23\n\nPublic V23 reactive controller on five Thomas V5/2 routes. Shop and tomato-stock selection; weed repair, sale lead, three-day budget, shed room, sale clamping, dead-stock sales and terminal liquidation. Front-run hook disabled in the original settings. Source hashes, credits, license and unverified strength claims are in IMPORT.json. C++ source parity and league validation pending.\n')
p = EXP / 'configs/league.json'
catalog = json.loads(p.read_text())
assert 'ahmed_v23' not in catalog
catalog['ahmed_v23'] = str(target.relative_to(ROOT))
p.write_text(json.dumps(catalog, indent=2) + '\n')
print('Exact original V23 source hash verified; five typed routes exported.')
