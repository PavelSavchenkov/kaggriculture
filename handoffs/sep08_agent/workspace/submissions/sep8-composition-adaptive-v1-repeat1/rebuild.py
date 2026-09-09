"""Rebuild tables and deployment archive from the retained source snapshot."""
import hashlib
import json
import subprocess
from pathlib import Path

OUT = Path(__file__).resolve().parent
frozen = json.loads((OUT / 'FROZEN.json').read_text())
for name, digest in frozen['files'].items():
    assert hashlib.sha256((OUT / 'source_tree' / name).read_bytes()).hexdigest() == digest, name
expected = json.loads((OUT / 'ARTIFACTS.json').read_text()) if (OUT / 'ARTIFACTS.json').exists() else None
commands = []
for script in ('build_export.py', 'build_submission.py'):
    command = ['conda', 'run', '-n', 'kaggriculture', 'python', str(OUT / script)]
    subprocess.run(command, check=True)
    commands.append(command)
actual = json.loads((OUT / 'ARTIFACTS.json').read_text())
if expected:
    assert actual == expected, 'Rebuilding changed the deployable artifacts.'
report = {'commands': commands, 'artifacts': actual, 'source_hashes_verified': len(frozen['files']),
          'scope': 'Exporter includes only source_tree. Adapter templates and scripts are retained locally. No live experiment or previous submission is read.'}
(OUT / 'REBUILD.json').write_text(json.dumps(report, indent=2) + '\n')
print('Deterministic source-only deployment rebuild passed.')
