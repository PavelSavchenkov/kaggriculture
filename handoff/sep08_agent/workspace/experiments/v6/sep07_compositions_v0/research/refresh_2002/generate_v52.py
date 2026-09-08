"""Typed literal export of the supplied notebook schedules and routing trees."""
from pathlib import Path
import ast
import base64
import hashlib
import json
import sys
import zlib

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
sys.path.insert(0, str(EXP / 'scripts'))
from generate_public_routes import triple

folder = RUN / 'notebooks/thomastschinkel/kaggriculture-95-5-win-rate-via-replay-routing'
data = json.loads((folder / 'payload.json').read_text())
tapes, trees = data['schedules'], data['policy']
assert {n[0] for tree in trees for n in tree if n[0] >= 0} == {2, 24, 25}
target = EXP / 'league/public_router_v52'


def encode(action):
    units = [triple(x if isinstance(x, list) and x else ['PASS']) for x in [action['farmer'], *action['hands']]]
    orders = []
    for order in action['market'][:10]:
        if not isinstance(order, list) or not order or order[0] not in ['HIRE', 'BUY_LAND', 'BUY_SEED', 'BUY_PRODUCT', 'BUY_ANIMAL', 'SELL']:
            order = ['SELL', 'WHEAT', 0]
        elif order[0] not in ['HIRE', 'BUY_LAND'] and (len(order) < 3 or int(order[2]) <= 0):
            order = ['SELL', 'WHEAT', 0]
        orders.append(triple(order, True))
    return [len(units), len(orders), *(v for row in units + orders for v in row)]


encoded = [[encode(action) for action in tape] for tape in tapes]
offsets, values = [], []
for tape in encoded:
    row = []
    for action in tape:
        row.append(len(values));values.extend(action)
    offsets.append(row)
code = '// Generated from Thomas v5/2; see IMPORT.json.\n'
code += 'inline constexpr int offsets[5][719]={\n' + ',\n'.join('{' + ','.join(map(str, r)) + '}' for r in offsets) + '\n};\n'
code += 'inline constexpr int values[]={\n' + ',\n'.join(','.join(map(str, values[i:i+100])) for i in range(0, len(values), 100)) + '\n};\n'
nodes, starts = [], []
for tree in trees:
    starts.append(len(nodes));nodes.extend(tree)
code += 'struct Node{int feature,left,right,route;double threshold;};\n'
code += 'inline constexpr int tree_offsets[]={' + ','.join(map(str, starts)) + '};\n'
code += 'inline constexpr Node nodes[]={' + ','.join('{' + ','.join(map(str, n)) + '}' for n in nodes) + '};\n'
path = target / 'source/data.inc'
assert not path.exists()
path.write_text(code)
old_path = EXP / 'research/refresh_1112/notebooks/thomastschinkel/kaggriculture-93-8-win-rate-public-state-router/extracted_main.py'
syntax = ast.parse(old_path.read_text())
blob = next(n.args[0].value for n in ast.walk(syntax) if isinstance(n, ast.Call) and isinstance(n.func, ast.Attribute) and n.func.attr == 'b64decode')
old_tapes, old_trees = json.loads(zlib.decompress(base64.b64decode(blob)))
old_encoded = [[encode(action) for action in tape] for tape in old_tapes]
report = {'source': 'https://www.kaggle.com/code/thomastschinkel/kaggriculture-95-5-win-rate-via-replay-routing',
          'source_sha256': hashlib.sha256((folder / 'extracted_main.py').read_bytes()).hexdigest(),
          'source_path': str((folder / 'extracted_main.py').relative_to(EXP)), 'schedules': 5, 'turns': 719, 'block_size': 72,
          'trees': trees, 'used_features': [2, 24, 25],
          'different_actions_against_old_v5': [[sum(a != b for a, b in zip(new, old)) for old in old_encoded] for new in encoded],
          'common_prefix_lengths': [[next((i for i, (a, b) in enumerate(zip(x, y)) if a != b), 719) for y in encoded] for x in encoded],
          'typed_tape_sha256': [hashlib.sha256(json.dumps(t, separators=(',', ':')).encode()).hexdigest() for t in encoded],
          'max_units': max(a[0] for t in encoded for a in t), 'max_orders': max(a[1] for t in encoded for a in t),
          'reported_strength': 'Notebook claims95.51% of44096 held-out replay games; unverified author claim, current Kaggle rating unknown.',
          'lineage_limit': 'Original replay episode provenance is not included in the downloaded callable. Do not infer authorship from nearest matches.',
          'changes': ['C++ typed literal export', 'Per-instance state and active-worker normalization', 'Only three features referenced by trees are evaluated', 'Preserve empty market slots as zero-quantity SELL'],
          'reuse_restrictions': 'Public notebook; no separate code license supplied. User authorizes borrowing.', 'status': 'Unpromoted; source parity, operational tests and league evaluation pending.'}
(target / 'IMPORT.json').write_text(json.dumps(report, indent=2) + '\n')
(target / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': 'public_router_v52', 'header': 'source/agent.hpp', 'type': 'kag::agents::public_router_v52::Agent', 'sources': ['source/agent.cpp']}, indent=2) + '\n')
(target / 'README.md').write_text('# public_router_v52\n\nThomas v5/2: five complete schedules, three-day route decisions using observed milk/wool demand, tomato market inventory and the prior route. Preserves empty market-order slots. Exact source, hashes, claims and limitations are in IMPORT.json. Validation pending; unoptimized C++ port, not a promoted reference.\n')
p = EXP / 'configs/league.json';catalog = json.loads(p.read_text());assert 'public_router_v52' not in catalog
catalog['public_router_v52'] = str(target.relative_to(ROOT));p.write_text(json.dumps(catalog, indent=2) + '\n')
print('old tape differences', report['different_actions_against_old_v5'])
print('prefix lengths', report['common_prefix_lengths'])
