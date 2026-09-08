from pathlib import Path
import ast
import base64
import hashlib
import json
import zlib

RUN = Path(__file__).resolve().parent
folder = RUN / 'notebooks/thomastschinkel/kaggriculture-95-5-win-rate-via-replay-routing'
notebooks = list(folder.glob('*.ipynb'))
assert len(notebooks) == 1
cells = json.loads(notebooks[0].read_text())['cells']
sources = [''.join(cell['source']) for cell in cells if cell['cell_type'] == 'code' and ''.join(cell['source']).startswith('%%writefile main.py')]
assert len(sources) == 1
main = sources[0].split('\n', 1)[1]
(folder / 'extracted_main.py').write_text(main)
tree = ast.parse(main)
node = next(n for n in tree.body if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Tuple)
            and [t.id for t in n.targets[0].elts] == ['SCHEDULES', 'POLICY'])
strings = [n.value for n in ast.walk(node.value) if isinstance(n, ast.Constant) and isinstance(n.value, str)]
assert len(strings) == 1
schedules, policy = json.loads(zlib.decompress(base64.b85decode(strings[0])))
(folder / 'payload.json').write_text(json.dumps({'schedules': schedules, 'policy': policy}, separators=(',', ':')) + '\n')
report = {'source_sha256': hashlib.sha256(main.encode()).hexdigest(), 'schedules': len(schedules),
          'schedule_lengths': sorted(set(map(len, schedules))), 'blocks': len(policy),
          'tree_sizes': list(map(len, policy)), 'used_features': sorted(set(n[0] for t in policy for n in t)),
          'scheduled_routes': sorted(set(n[3] for t in policy for n in t if n[0] < 0)),
          'has_previous_route_tests': any(n[0] == -2 for t in policy for n in t),
          'scope': 'Static AST and JSON extraction only; no notebook execution.'}
(RUN / 'THOMAS_PAYLOAD.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
