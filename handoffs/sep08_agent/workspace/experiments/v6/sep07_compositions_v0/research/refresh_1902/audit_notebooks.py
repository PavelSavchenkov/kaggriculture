"""Compare downloaded policy literals without executing notebook code."""
from pathlib import Path
from datetime import datetime, timezone
import ast
import base64
import hashlib
import json
import zlib

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
folder = RUN / 'notebooks/souvikdbiswas/kaggriculture-v5-hybrid-agent'
notebooks = list(folder.glob('*.ipynb'))
assert len(notebooks) == 1
main = None
for cell in json.loads(notebooks[0].read_text())['cells']:
    if cell['cell_type'] != 'code':
        continue
    code = ''.join(cell['source'])
    if '_MAIN_SRC =' not in code:
        continue
    for node in ast.parse(code).body:
        if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and target.id == '_MAIN_SRC' for target in node.targets):
            assert main is None
            main = ast.literal_eval(node.value)
assert isinstance(main, str)
(folder / 'extracted_main.py').write_text(main)
old_path = EXP / 'research/refresh_1112/notebooks/thomastschinkel/kaggriculture-93-8-win-rate-public-state-router/extracted_main.py'
old = old_path.read_text()


def payload(code):
    tree = ast.parse(code)
    assignments = [node for node in tree.body if isinstance(node, ast.Assign)
                   and isinstance(node.targets[0], ast.Tuple)
                   and [target.id for target in node.targets[0].elts] == ['_TAPES', '_TREES']]
    assert len(assignments) == 1
    strings = [node.value for node in ast.walk(assignments[0].value) if isinstance(node, ast.Constant) and isinstance(node.value, str)]
    assert len(strings) == 1
    return json.loads(zlib.decompress(base64.b64decode(strings[0])))


def functions(code):
    return {node.name: ast.dump(node, include_attributes=False) for node in ast.parse(code).body
            if isinstance(node, ast.FunctionDef)}


old_data, new_data = payload(old), payload(main)
old_functions, new_functions = functions(old), functions(main)
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'notebook': str(notebooks[0].relative_to(EXP)),
          'previous_policy': str(old_path.relative_to(EXP)),
          'source_bytes_equal': old == main,
          'source_sha256': hashlib.sha256(main.encode()).hexdigest(),
          'tapes_equal': old_data[0] == new_data[0], 'trees_equal': old_data[1] == new_data[1],
          'function_ast_equal': {name: new_functions.get(name) == value for name, value in old_functions.items()},
          'new_function_names': sorted(new_functions.keys() - old_functions.keys()),
          'scope': 'Only AST literal extraction and base64/zlib/JSON decoding; no notebook policy execution.'}
(RUN / 'V5_NOTEBOOK_COMPARISON.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
