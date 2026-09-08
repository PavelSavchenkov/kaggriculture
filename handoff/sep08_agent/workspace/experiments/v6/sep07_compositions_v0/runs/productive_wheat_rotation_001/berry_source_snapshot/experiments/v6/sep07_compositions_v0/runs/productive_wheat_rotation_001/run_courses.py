"""Record source inputs and compile four complete C++ wheat controls."""
import hashlib
import argparse
import json
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--berry',action='store_true');args=parser.parse_args()
    kind='compile_wheat_berry'if args.berry else'compile_wheat'
    files = {RUN / 'CMakeLists.txt', RUN / 'prepare_compiler.py', Path(__file__).resolve()}
    for dependency in (RUN / f'build/CMakeFiles/{kind}.dir').rglob('*.o.d'):
        for name in dependency.read_text().replace('\\\n', ' ').split()[1:]:
            path = Path(name).resolve()
            if path.is_relative_to(ROOT) and path.is_file():
                files.add(path)
    hashes = {}
    for path in sorted(files):
        relative = path.relative_to(ROOT)
        target = RUN / ('berry_source_snapshot'if args.berry else'source_snapshot') / relative
        target.parent.mkdir(parents=True, exist_ok=True);shutil.copy2(path, target)
        hashes[str(relative)] = hashlib.sha256(path.read_bytes()).hexdigest()
    binary = RUN / 'build' / kind
    record = {'started_utc': datetime.now(timezone.utc).isoformat(), 'source_sha256': hashes,
              'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(), 'source_parent': 'crop_value_m2_t4'}
    (RUN / ('BERRY_COMPILER_INPUTS.json'if args.berry else'COMPILER_INPUTS.json')).write_text(json.dumps(record, indent=2) + '\n')
    def run(job):
        name, cells, fertilizer = job
        command = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture', str(binary),
                   str(RUN / name), '1000', cells, '2']
        if not fertilizer:
            command.append('--no-fertilizer')
        started = datetime.now(timezone.utc).isoformat()
        with (RUN / f'{name}.log').open('w') as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
        return {'name': name, 'command': command, 'returncode': result.returncode,
                'started_utc': started, 'finished_utc': datetime.now(timezone.utc).isoformat()}
    jobs = [('one_fert', '22', True), ('one_plain', '22', False),
            ('three_fert', '21,22,31', True), ('three_plain', '21,22,31', False)]
    if args.berry:jobs=[(name+'_berry',cells,fertilizer)for name,cells,fertilizer in jobs]
    assert all(not (RUN / name).exists() for name, cells, fertilizer in jobs)
    with ThreadPoolExecutor(max_workers=2) as pool:
        commands = list(pool.map(run, jobs))
    (RUN / ('BERRY_COMPILE_COMMANDS.json'if args.berry else'COMPILE_COMMANDS.json')).write_text(json.dumps(commands, indent=2) + '\n')
    print(json.dumps(commands, indent=2))


if __name__ == '__main__':
    main()
