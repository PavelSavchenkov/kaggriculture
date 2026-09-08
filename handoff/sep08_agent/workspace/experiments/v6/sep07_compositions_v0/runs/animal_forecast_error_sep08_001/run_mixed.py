"""Trace partial purchases and test a funded day-end stock repair."""
from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]
cmake = RUN / 'CMakeLists.txt'
base = cmake.read_text()
assert 'add_executable(mixed' not in base
cmake.write_text(base + '\n' + base[base.index('add_executable'):].replace('diagnostic', 'mixed'))
commands = [
    ['conda', 'run', '-n', 'kaggriculture', 'cmake', '-S', str(RUN), '-B', str(RUN / 'build')],
    ['conda', 'run', '-n', 'kaggriculture', 'cmake', '--build', str(RUN / 'build'), '--target', 'mixed', '-j', '2'],
    ['conda', 'run', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'), str(RUN / 'build/mixed'), str(RUN / 'mixed_trace')],
]
(RUN / 'MIXED_PROTOCOL.json').write_text(json.dumps({'commands': commands,
    'source_sha256': hashlib.sha256((RUN / 'source/mixed.cpp').read_bytes()).hexdigest(),
    'scope': 'Source prefix plus saved day9 plan. Compare actual input buys with requests; test a two-wheat hour23 order. This alone does not validate a full mixed season.'}, indent=2) + '\n')
for i, command in enumerate(commands):
    with (RUN / f'mixed_command_{i}.log').open('x') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
print('Mixed day transaction and endpoint comparison complete.')
