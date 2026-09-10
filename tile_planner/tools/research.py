"""Verify existing evidence or restore compact reports and indexed inputs."""
import argparse
import gzip
import hashlib
import json
import shutil
import tarfile
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
    parser.add_argument('action', choices=['verify', 'reports', 'inputs'])
    parser.add_argument('--repo', type=Path, default=PACKAGE.parent)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if args.action == 'verify':
        checkpoint = json.loads((PACKAGE / 'research/CHECKPOINT.json').read_text())
        index = PACKAGE / checkpoint['index']
        if digest(index) != checkpoint['index_sha256']:
            raise ValueError('evidence index hash mismatch')
        with gzip.open(index, 'rt') as stream:
            records = json.load(stream)
        failures = []
        for name, (size, expected) in records['files'].items():
            path = args.repo / records['root'] / name
            if not path.is_file() or path.stat().st_size != size or digest(path) != expected:
                failures.append(name)
        print(json.dumps({'checked': len(records['files']), 'failed': failures}))
        if failures:
            raise ValueError('existing research data is missing or changed')
        return
    if args.output is None:
        parser.error('--output is required for restoration')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    if args.action == 'reports':
        manifest = json.loads((PACKAGE / 'research/REPORTS.json').read_text())
        archive = PACKAGE / manifest['archive']
        if digest(archive) != manifest['sha256']:
            raise ValueError('report archive hash mismatch')
        with tarfile.open(archive) as tar:
            members = tar.getmembers()
            if {member.name for member in members} != set(manifest['files']):
                raise ValueError('report archive file set mismatch')
            for member in members:
                target = (output / member.name).resolve()
                if not member.isfile() or not target.is_relative_to(output):
                    raise ValueError('unsafe report archive member')
                data = tar.extractfile(member).read()
                expected = manifest['files'][member.name]
                if len(data) != expected['bytes'] or hashlib.sha256(data).hexdigest() != expected['sha256']:
                    raise ValueError('report member hash mismatch: ' + member.name)
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
        print(json.dumps({'restored_reports': len(members), 'output': str(output)}))
        return
    manifest = json.loads((PACKAGE / 'research/INPUTS.json').read_text())
    restored = []
    for index, entry in enumerate(manifest['inputs']):
        source = args.repo / entry['plan']
        if digest(source) != entry['sha256']:
            raise ValueError('input plan hash mismatch: ' + entry['plan'])
        directory = output / f'{index:03d}'
        directory.mkdir()
        shutil.copy2(source, directory / 'INPUT.plan')
        if 'assignment' in entry:
            source = args.repo / entry['assignment']
            if digest(source) != entry['assignment_sha256']:
                raise ValueError('assignment hash mismatch: ' + entry['assignment'])
            shutil.copy2(source, directory / 'assignment.txt')
        restored.append(str(directory / 'INPUT.plan'))
    (output / 'MANIFEST.txt').write_text('\n'.join(restored) + '\n')
    print(json.dumps({'restored_inputs': len(restored), 'output': str(output)}))


if __name__ == '__main__':
    main()
