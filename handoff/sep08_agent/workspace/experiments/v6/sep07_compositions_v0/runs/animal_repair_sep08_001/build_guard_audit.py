"""Build an offline guard inspector without altering any frozen runtime policy."""
import json
import subprocess
from pathlib import Path

RUN = Path(__file__).resolve().parent
metadata = json.loads((RUN / 'broad_build/build.json').read_text())
command = metadata['command'][:]
source = str(RUN / 'source/broad.cpp')
command[command.index(source)] = str(RUN / 'source/guard_audit.cpp')
command[-1] = str(RUN / 'guard_audit')
with (RUN / 'guard_audit_build.log').open('w') as log:
    subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
(RUN / 'GUARD_AUDIT_BUILD.json').write_text(json.dumps({'command': command}, indent=2) + '\n')
print('Guard inspector built.')
