"""Decode the new public router and compare its data with existing ports."""
import ast
import base64
import hashlib
import json
import re
import shutil
import zlib
from pathlib import Path

from generate_public_routes import triple

EXP = Path(__file__).resolve().parents[1]
SOURCE = EXP / 'research/refresh_1112/notebooks/thomastschinkel/kaggriculture-93-8-win-rate-public-state-router'
TARGET = EXP / 'league/public_router_v5'


def action_code(action):
    units = [triple(x) for x in [action['farmer'], *action['hands']]]
    orders = [triple(x, True) for x in action['market']]
    return [len(units), len(orders), *(v for x in units + orders for v in x)]


def main():
    source = SOURCE / 'extracted_main.py'
    syntax = ast.parse(source.read_text())
    blob = next(n.args[0].value for n in ast.walk(syntax) if isinstance(n, ast.Call)
                and isinstance(n.func, ast.Attribute) and n.func.attr == 'b64decode')
    tapes, trees = json.loads(zlib.decompress(base64.b64decode(blob)))
    assert len(tapes) == 5 and all(len(t) == 719 for t in tapes)
    assert {n[0] for tree in trees for n in tree if n[0] >= 0} == {4, 28, 35}
    target = TARGET / 'source'
    target.mkdir(parents=True, exist_ok=True)
    values, offsets, encoded = [], [], []
    for tape in tapes:
        row, course = [], []
        for action in tape:
            row.append(len(values))
            code = action_code(action)
            values.extend(code)
            course.append(code)
        offsets.append(row)
        encoded.append(course)
    code = '// Exact immutable public notebook data; see IMPORT.json.\n'
    code += 'inline constexpr int offsets[5][719]={\n' + ',\n'.join('{' + ','.join(map(str, x)) + '}' for x in offsets) + '\n};\n'
    code += 'inline constexpr int values[]={\n' + ',\n'.join(','.join(map(str, values[i:i+100])) for i in range(0, len(values), 100)) + '\n};\n'
    tree_offsets, nodes = [], []
    for tree in trees:
        tree_offsets.append(len(nodes))
        nodes.extend(tree)
    code += 'struct Node {int feature,left,right,route;double threshold;};\n'
    code += 'inline constexpr int tree_offsets[]={' + ','.join(map(str, tree_offsets)) + '};\n'
    code += 'inline constexpr Node nodes[]={' + ','.join('{' + ','.join(map(str, n)) + '}' for n in nodes) + '};\n'
    (target / 'data.inc').write_text(code)
    shutil.copyfile(source, TARGET / 'upstream_reference.py.txt')
    shutil.copyfile(SOURCE / 'kernel-metadata.json', TARGET / 'notebook-metadata.json')
    comparisons = []
    old = (EXP / 'league/teammate_sixday/source/upstream/policy.hpp').read_text()
    for block, route, count, body in re.findall(r'const char\* const kSegment(\d+)Route(\d+)\[(\d+)\] = \{(.*?)\n\};', old, re.S):
        records = [list(map(int, s.split())) for s in re.findall(r'"([0-9 ]+)"', body)]
        assert len(records) == int(count)
        for record in records:
            units, orders = record[:2]
            for i in range(orders):
                start = 2 + units * 3 + i * 3
                if record[start] <= 2:
                    record[start+1:start+3] = [0, 0]
        start = int(block) * 144
        comparisons.append({'old_block': int(block), 'old_route': int(route), 'turns': len(records),
                            'different_actions_by_new_tape': [sum(a != b for a, b in zip(records, tape[start:start+len(records)])) for tape in encoded]})
    assert len(comparisons) == 12
    library = (EXP / 'league/top_replay_library/source/tapes.inc').read_text()
    library_offsets = json.loads(re.search(r'offsets\[\d+\]\[719\]=(\{.*?\});', library, re.S)[1].replace('{', '[').replace('}', ']'))
    library_values = list(map(int, re.search(r'values\[\]=\{(.*?)\};', library, re.S)[1].replace('\n', '').split(',')))
    source_metadata = json.loads((EXP / 'league/top_replay_library/IMPORT.json').read_text())['programs']
    library_matches = [[] for _ in tapes]
    for program, row in enumerate(library_offsets):
        course = []
        for offset in row:
            n, m = library_values[offset:offset+2]
            course.append(library_values[offset:offset+2+3*(n+m)])
        for i, tape in enumerate(encoded):
            differences = sum(a != b for a, b in zip(tape, course))
            unit_differences = sum(a[:1] + a[2:2+3*a[0]] != b[:1] + b[2:2+3*b[0]] for a, b in zip(tape, course))
            library_matches[i].append({'program': program, 'different_actions': differences,
                                       'different_unit_command_rows': unit_differences,
                                       'source': source_metadata[program]})
    nearest = [sorted(x, key=lambda r: r['different_actions'])[:3] for x in library_matches]
    nearest_units = [sorted(x, key=lambda r: (r['different_unit_command_rows'], r['different_actions']))[:3] for x in library_matches]
    prefix = [[next((t for t, (x, y) in enumerate(zip(a, b)) if x != y), 719) for b in encoded] for a in encoded]
    audit = {'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
             'source': 'https://www.kaggle.com/code/thomastschinkel/kaggriculture-93-8-win-rate-public-state-router',
             'tapes': 5, 'turns_per_tape': 719, 'trees': trees, 'tape_prefix_lengths': prefix,
             'typed_tape_sha256': [hashlib.sha256(json.dumps(x, separators=(',', ':')).encode()).hexdigest() for x in encoded],
             'nearest_existing_library_courses': nearest, 'nearest_existing_library_unit_courses': nearest_units, 'older_public_sixday_segments': comparisons,
             'max_source_units': max(x[0] for tape in encoded for x in tape), 'max_source_orders': max(x[1] for tape in encoded for x in tape),
             'effective_features': {'144': 'Yarn count>0 selects4; otherwise milk demand>0 selects2, else1', '576': 'Carrot quote<=54 selects3, otherwise0', '0,288,432': 'Select0'},
             'distinct_from_public_sixday': 'Different trees, five complete tapes instead of twelve independent segments; no six-day budget guard, no market logistic wrapper, no opponent input used by a decision node',
             'reported_strength': 'Notebook reports41344/44096=93.76% against689 frozen tapes and3844/4096 stress wins; these are unverified source claims, not our league measurements',
             'lineage_limit': 'Supplied notebook claims original source and public replay tapes but omits provenance.json and original episode mapping; nearest recorded matches are measured above and do not establish authorship',
             'changes': ['Typed C++ translation, immutable decoded action data', 'Only the three referenced feature entries are computed; generation asserts the tree feature set', 'Per-instance state and active-worker normalization required by local API'],
             'reuse_permission': 'Public notebook; user explicitly authorizes borrowing controllers and schedules; no separate code license supplied in downloaded metadata'}
    (SOURCE / 'AUDIT.json').write_text(json.dumps(audit, indent=2) + '\n')
    (TARGET / 'IMPORT.json').write_text(json.dumps(audit, indent=2) + '\n')
    manifest = {'format_version': 1, 'name': 'public_router_v5', 'header': 'source/agent.hpp',
                'type': 'compositions::public_router_v5::Agent', 'sources': ['source/agent.cpp']}
    (TARGET / 'agent.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({'prefix': prefix, 'nearest_library': [[(x['program'], x['different_actions']) for x in r] for r in nearest], 'nearest_unit_courses': [[(x['program'], x['different_unit_command_rows']) for x in r] for r in nearest_units], 'old_exact_segment_matches': [x for x in comparisons if 0 in x['different_actions_by_new_tape']]}))


if __name__ == '__main__':
    main()
