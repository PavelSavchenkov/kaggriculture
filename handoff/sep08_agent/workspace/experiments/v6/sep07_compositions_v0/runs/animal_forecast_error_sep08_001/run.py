"""Build the offline diagnostic with the existing day-contract library."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
registry = json.loads((EXP / 'configs/league.json').read_text())
sources = set()
for name in ('empty_sale_slots_m2', 'public_router'):
    package = ROOT / registry[name]
    sources.update((package / p).resolve() for p in json.loads((package / 'agent.json').read_text())['sources'])
cmake = '''cmake_minimum_required(VERSION 3.18)
project(animal_forecast_error LANGUAGES CXX)
get_filename_component(EXPERIMENT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(REPOSITORY "${EXPERIMENT}/../../.." ABSOLUTE)
add_subdirectory("${REPOSITORY}/day_solver" day_solver)
add_executable(diagnostic source/diagnostic.cpp
'''
cmake += ''.join(f'    "${{REPOSITORY}}/{p.relative_to(ROOT)}"\n' for p in sorted(sources)) + ')\n'
cmake += '''target_include_directories(diagnostic BEFORE PRIVATE "${REPOSITORY}")
target_include_directories(diagnostic SYSTEM PRIVATE "${REPOSITORY}/day_solver/vendor/include")
target_compile_definitions(diagnostic PRIVATE OR_PROTO_DLL= PROTOBUF_USE_DLLS)
target_compile_options(diagnostic PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(diagnostic PRIVATE DaySolver::scheduler)
'''
(RUN / 'CMakeLists.txt').write_text(cmake)
commands = [
    ['conda', 'run', '-n', 'kaggriculture', 'cmake', '-S', str(RUN), '-B', str(RUN / 'build')],
    ['conda', 'run', '-n', 'kaggriculture', 'cmake', '--build', str(RUN / 'build'), '--target', 'diagnostic', '-j', '2'],
    ['conda', 'run', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'), str(RUN / 'build/diagnostic'), str(RUN / 'RESULTS.csv')],
]
inputs = [RUN / 'source/diagnostic.cpp', EXP / 'runs/animal_groups_sep08_001/source/season_v2.hpp',
    EXP / 'runs/animal_groups_sep08_001/source/estimate_v2.cpp', EXP / 'include/animal_investment_value.hpp']
(RUN / 'PROTOCOL.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'commands': commands, 'source_sha256': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
    'scope': 'Offline matched forecast error decomposition on two already-exposed complete courses. Actual future shops, own and rival flows used only for diagnostic attribution, never as policy inputs.'}, indent=2) + '\n')
for i, command in enumerate(commands):
    with (RUN / f'command_{i}.log').open('x') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
print('Forecast diagnostic complete: two cases, original hash/cash and candidate cash controls pass.')
