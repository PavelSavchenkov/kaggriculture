"""Freeze a built arena's transitive C++ inputs and exact rebuild command."""
import argparse
import hashlib
import json
import shlex
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    binary, out = args.binary.resolve(), args.output.resolve()
    assert binary.is_relative_to(EXP) and out.is_relative_to(EXP)
    snapshot = out / 'source_snapshot'
    assert not snapshot.exists()
    out.mkdir(parents=True, exist_ok=True)
    build = json.loads((binary.parent / 'build.json').read_text())
    command = build['command']
    index = command.index('-o')
    assert index == len(command) - 2
    dependency_command = command[:index] + ['-MM']
    dependencies = subprocess.check_output(dependency_command, text=True)
    paths = {binary.parent / 'build.json', Path(__file__)}
    for line in dependencies.replace('\\\n', ' ').splitlines():
        if line.strip():
            paths.update(Path(p).resolve() for p in shlex.split(line.split(':', 1)[1]))
    hashes = {}
    for path in sorted(paths):
        assert path.is_relative_to(ROOT), path
        relative = path.relative_to(ROOT)
        target = snapshot / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
        hashes[str(relative)] = hashlib.sha256(target.read_bytes()).hexdigest()
    rebuild = [str(snapshot) + value[len(str(ROOT)):] if value.startswith(str(ROOT) + '/') or value == str(ROOT) else value for value in command[:index]]
    rebuild += ['-o', str(out / 'arena_rebuilt')]
    report = {'frozen_utc': datetime.now(timezone.utc).isoformat(), 'binary': str(binary),
              'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
              'compiler': build['compiler'], 'original_command': command,
              'dependency_command': dependency_command, 'rebuild_command': rebuild,
              'files_sha256': hashes, 'scope': 'Only actual C++ dependencies from compiler -MM, plus build metadata and this freezer. Rebuild uses the frozen tree.'}
    (out / 'FROZEN.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Frozen arena dependencies:', len(hashes), 'files.')


if __name__ == '__main__':
    main()
