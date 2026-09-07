"""Verify packaged hashes and resolve ELF dependencies without the experiment."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

PACKAGE = Path(__file__).resolve().parents[1]


def main():
    count = 0
    for line in (PACKAGE / 'SHA256SUMS').read_text().splitlines():
        expected, path = line.split('  ', 1)
        actual = hashlib.sha256((PACKAGE / path).read_bytes()).hexdigest()
        if actual != expected:
            raise RuntimeError('Checksum mismatch: ' + path)
        count += 1
    env = dict(os.environ, LD_LIBRARY_PATH=str(PACKAGE / 'runtime/lib'))
    for name in ('fast_solver_cli', 'reference_solver_cli', 'audit_schedule'):
        result = subprocess.run(['ldd', str(PACKAGE / 'runtime' / name)],
                                env=env, text=True, capture_output=True, check=True)
        if 'not found' in result.stdout:
            raise RuntimeError(result.stdout)
        for path in re.findall(r'=> (/\S+)', result.stdout):
            p = Path(path)
            if not p.is_relative_to(PACKAGE) and not p.is_relative_to('/lib') and not p.is_relative_to('/usr/lib'):
                raise RuntimeError('Dependency outside package/system libraries: ' + path)
    print(json.dumps(dict(files_checked=count, checksums_match=True, runtime_dependencies_contained=True)))


if __name__ == '__main__':
    main()
