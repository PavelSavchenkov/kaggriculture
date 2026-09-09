"""Extract literal source artifacts and compare them with audited ancestors."""
from pathlib import Path
import ast
import base64
import difflib
import hashlib
import io
import json
import tarfile

HERE = Path(__file__).resolve().parent
EXP = HERE.parents[1]
audit = HERE / 'notebook_audit'

def cells(directory):
    return json.loads(next(directory.glob('*.ipynb')).read_text())['cells']

tetsu = audit / 'tetsutani__market-smart-farming-kaggriculture'
tree = ast.parse(''.join(cells(tetsu)[9]['source']))
constants = {n.targets[0].id: ast.literal_eval(n.value) for n in tree.body
    if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and isinstance(n.value, ast.Constant)}
blob = base64.b64decode(constants['PACKAGE_B64'])
assert hashlib.sha256(blob).hexdigest() == constants['EXPECTED_ARCHIVE_SHA256']
files = {}
with tarfile.open(fileobj=io.BytesIO(blob), mode='r:gz') as archive:
    names = archive.getnames()
    assert names == ['main.py', 'actions.json', 'model.json', 'observation.py']
    for name in names:
        data = archive.extractfile(name).read()
        (tetsu / (name + '.txt')).write_bytes(data)
        files[name] = hashlib.sha256(data).hexdigest()
assert files['main.py'] == constants['EXPECTED_MAIN_SHA256']
parent = EXP / 'research/refresh_sep08_1108/notebook_audit/yusuke'
comparison = {name: {'sha256': digest, 'equal_yusuke': digest == hashlib.sha256((parent / (name + '.txt')).read_bytes()).hexdigest()}
    for name, digest in files.items()}
main = (tetsu / 'main.py.txt').read_text()
ast.parse(main)
(tetsu / 'YUSUKE_MAIN.diff').write_text(''.join(difflib.unified_diff((parent / 'main.py.txt').read_text().splitlines(True), main.splitlines(True), fromfile='Yusuke0908', tofile='Tetsutani1408')))
(tetsu / 'SOURCE_COMPARISON.json').write_text(json.dumps(comparison, indent=2) + '\n')

ahmed = audit / 'ahmedberatozer__notebookd5e3d21fa6'
source = ''.join(cells(ahmed)[2]['source']).split('\n', 1)[1]
ast.parse(source)
old = (EXP / 'research/refresh_sep08_1208/notebook_audit/ahmedberatozer__notebookd5e3d21fa6/reference_v24.py').read_text()
ahmed_result = {'source_sha256': hashlib.sha256(source.encode()).hexdigest(), 'byte_equal_v24': source == old,
    'ast_equal_v24': ast.dump(ast.parse(source)) == ast.dump(ast.parse(old))}
(ahmed / 'SOURCE_COMPARISON.json').write_text(json.dumps(ahmed_result, indent=2) + '\n')

dmitrii = audit / 'dmitriigluzdov__kaggriculture-last-mile-harvest-planner'
allowed = {'main.py', 'policy.py', 'terminal_planner.py', 'thomas_parent.py', 'unit_model.py', 'LICENSE.txt', 'NOTICE.txt', 'tapes.json', 'trees.json'}
extracted = {}
for cell in cells(dmitrii):
    raw = ''.join(cell['source'])
    if not raw.startswith('%%writefile '):
        continue
    first, source = raw.split('\n', 1)
    name = first.split()[1]
    if name not in allowed:
        continue
    if name.endswith('.py'):
        ast.parse(source)
    (dmitrii / (name + '.txt')).write_text(source)
    extracted[name] = hashlib.sha256(source.encode()).hexdigest()
assert set(extracted) == allowed
(dmitrii / 'SOURCE_FILES.json').write_text(json.dumps(extracted, indent=2) + '\n')
article = audit / 'yhay81__when-do-daily-top-episodes-see-your-agent'
previous = EXP / 'research/refresh_sep08_1308/notebook_audit/yhay81__when-do-daily-top-episodes-see-your-agent'
current_cells = [(c['cell_type'], ''.join(c['source'])) for c in cells(article)]
old_cells = [(c['cell_type'], ''.join(c['source'])) for c in cells(previous)]
result = {'tetsutani': comparison, 'ahmed': ahmed_result, 'dmitrii_files': extracted,
    'episode_article_source_unchanged': current_cells == old_cells, 'notebook_code_executed': False}
(HERE / 'SOURCE_INSPECTION.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps({'tetsutani': comparison, 'ahmed': ahmed_result,
    'episode_article_source_unchanged': current_cells == old_cells}, indent=2))
