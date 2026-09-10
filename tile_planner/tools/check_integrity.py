"""Verify package files and the declared shared repository versions."""
import argparse
import hashlib
import json
from pathlib import Path

PACKAGE = Path(__file__).resolve().parents[1]


def digest(path):
    sha = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1048576), b''):
            sha.update(chunk)
    return sha.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', type=Path, default=PACKAGE.parent)
    args = parser.parse_args()
    manifest = json.loads((PACKAGE / 'MANIFEST.json').read_text())
    dependencies = json.loads((PACKAGE / 'REPOSITORY_DEPENDENCIES.json').read_text())
    checks = [(PACKAGE / name, expected) for name, expected in manifest['sha256'].items()]
    checks += [(args.repo / name, expected) for name, expected in dependencies['source_sha256'].items()]
    checks += [(args.repo / name, entry['sha256']) for name, entry in dependencies['runtime_libraries'].items()]
    failed = [str(path) for path, expected in checks if not path.is_file() or digest(path) != expected]
    print(json.dumps({'checked': len(checks), 'failed': failed}))
    if failed:
        raise ValueError('package or shared dependency hash mismatch')


if __name__ == '__main__':
    main()
