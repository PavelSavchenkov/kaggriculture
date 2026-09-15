"""Check the delivery snapshot's file hashes and data-size limit."""
import hashlib
import json
from pathlib import Path

PACKAGE = Path(__file__).resolve().parents[1]


if __name__ == '__main__':
    manifest = json.loads((PACKAGE / 'MANIFEST.json').read_text())
    actual = {str(p.relative_to(PACKAGE)): p for p in PACKAGE.rglob('*') if p.is_file()}
    actual.pop('MANIFEST.json')  # A manifest cannot contain its own hash.
    if set(actual) != set(manifest['files']):
        raise ValueError({'missing': sorted(set(manifest['files']) - set(actual)),
                          'unexpected': sorted(set(actual) - set(manifest['files']))})
    for name, path in actual.items():
        expected = manifest['files'][name]
        if path.is_symlink() or path.stat().st_size != expected['bytes']:
            raise ValueError(f'File type/size changed: {name}')
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected['sha256']:
            raise ValueError(f'File content changed: {name}')
    dependencies = json.loads((PACKAGE / 'REPOSITORY_DEPENDENCIES.json').read_text())
    for name, digest in dependencies['files'].items():
        path = PACKAGE.parent / name
        if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise ValueError(f'Repository dependency differs from the measured version: {name}')
    data_bytes = sum(p.stat().st_size for name, p in actual.items()
                     if name.startswith(('data/', 'measurements/')))
    if data_bytes >= 50_000_000:
        raise ValueError('Replay data and measurements must remain below 50 MB')
    print(f"Verified {len(actual)} package files and {len(dependencies['files'])} repository headers; "
          f"replay data and measurements {data_bytes:,} bytes")
