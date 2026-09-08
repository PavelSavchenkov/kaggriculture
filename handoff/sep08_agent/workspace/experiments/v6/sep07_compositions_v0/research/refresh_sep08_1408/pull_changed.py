"""Download notebook sources for static inspection without executing cells."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import hashlib
import json
import subprocess

HERE = Path(__file__).resolve().parent
changes = json.loads((HERE / 'notebook_changes.json').read_text())

def pull(row):
    out = HERE / 'notebook_audit' / row['ref'].replace('/', '__')
    out.mkdir(parents=True, exist_ok=False)
    command = ['conda', 'run', '-n', 'kaggriculture', 'kaggle', 'kernels', 'pull', row['ref'], '-p', str(out), '-m']
    with (out / 'pull.log').open('x') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    cells = []
    for path in out.glob('*.ipynb'):
        cells = json.loads(path.read_text())['cells']
        (out / 'reference.txt').write_text('\n\n'.join(f"CELL {i} {c['cell_type']}\n" + ''.join(c['source']) for i, c in enumerate(cells)))
    (out / 'CELLS.json').write_text(json.dumps([{'index': i, 'type': c['cell_type'],
        'characters': len(''.join(c['source'])), 'preview': ''.join(c['source'])[:220]} for i, c in enumerate(cells)], indent=2) + '\n')
    return {'ref': row['ref'], 'command': command,
        'files': {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file()}}

with ThreadPoolExecutor(max_workers=2) as pool:
    records = list(pool.map(pull, changes))
(HERE / 'NOTEBOOK_DOWNLOAD.json').write_text(json.dumps({'records': records, 'code_executed': False}, indent=2) + '\n')
print('Downloaded', len(records), 'notebooks for static inspection.')
