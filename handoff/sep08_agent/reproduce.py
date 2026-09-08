"""Verify or materialize the September8 source/evidence snapshot."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import argparse
import gzip
import hashlib
import json
import lzma
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
ENV = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture']


def read(record):
    path = ROOT / record['repository_path']
    data = path.read_bytes()
    if record['encoding'] == 'gzip':
        data = gzip.decompress(data)
    elif record['encoding'] == 'xz':
        data = lzma.decompress(data)
    assert len(data) == record['bytes'], record['path']
    assert hashlib.sha256(data).hexdigest() == record['sha256'], record['path']
    return data


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('command', choices=['verify', 'prepare', 'rebuild-submission'])
    parser.add_argument('--destination', type=Path, default=HERE / '_work/session')
    parser.add_argument('--evidence', action='store_true', help='Also restore full paired profiles and large diagnostic outputs.')
    parser.add_argument('--replays', action='store_true', help='Also restore selected raw replay evidence.')
    parser.add_argument('--prefix', nargs='+', help='Restore only these inventory path prefixes, useful for one large study.')
    args = parser.parse_args()
    inventory = json.loads((HERE / 'evidence/inventory.json').read_text())
    records = inventory['files']
    dependencies = json.loads((HERE / 'evidence/repository_dependencies.json').read_text())
    for path, expected in dependencies.items():
        assert hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == expected, path
    if args.command == 'verify':
        manifest = json.loads((HERE / 'MANIFEST.json').read_text())
        for path, expected in manifest['files_sha256'].items():
            assert hashlib.sha256((HERE / path).read_bytes()).hexdigest() == expected, path
        catalog = json.loads((HERE / 'evidence/catalog_sources.json').read_text())
        for path, expected in catalog.items():
            assert hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == expected, path
        unique = {r['sha256']: r for r in records}
        with ThreadPoolExecutor(max_workers=4) as pool:
            for _ in pool.map(read, unique.values()):
                pass
        print(f'Verified {len(records)} files, {len(unique)} distinct contents, and {len(dependencies)} repository dependencies.')
        return
    destination = args.destination.resolve()
    assert destination != ROOT and not (destination / '.git').exists()
    destination.mkdir(parents=True, exist_ok=True)
    marker = destination / 'SEP08_SNAPSHOT.json'
    if any(destination.iterdir()) and not marker.exists():
        raise ValueError('Destination must be empty or a previously prepared September8 workspace.')
    selected = [r for r in records if r['kind'] in {'core', 'submission'}
                or (r['kind'] == 'evidence' and args.evidence) or (r['kind'] == 'replay' and args.replays)]
    if args.command == 'rebuild-submission':
        selected = [r for r in records if r['path'].startswith('submissions/sep8-composition-adaptive-v1-repeat1/')]
    if args.prefix:
        selected = [r for r in selected if r['path'].startswith(tuple(args.prefix))]
    for record in selected:
        path = destination / record['path']
        data = read(record)
        path.parent.mkdir(parents=True, exist_ok=True)
        if not path.exists() or path.read_bytes() != data:
            path.write_bytes(data)
        if path.suffix == '.sh':
            path.chmod(0o755)
    for name in ['agents', 'day_solver', 'fast_game_engine', 'prompts', 'external']:
        target = destination / name
        if (ROOT / name).exists() and not target.exists():
            target.symlink_to(ROOT / name, target_is_directory=True)
    marker.write_text(json.dumps({'source': str(HERE), 'files': len(selected),
        'source_bytes_verified_before_write': True, 'absolute_historical_metadata': inventory['original_root'],
        'note': 'Sources remain byte-exact. Historical binary paths are evidence, not live handles. Rebuild executables and use their new paths.'}, indent=2) + '\n')
    if args.command == 'rebuild-submission':
        script = destination / 'submissions/sep8-composition-adaptive-v1-repeat1/rebuild.py'
        subprocess.run(ENV + ['python', str(script)], check=True)
    print(destination)


if __name__ == '__main__':
    main()
