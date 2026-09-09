from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
output = RUN / 'build'
output.mkdir(exist_ok=True)
command = ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O3', '-march=native', '-mtune=native',
           '-DNDEBUG', '-fno-exceptions', '-fno-rtti', '-fno-math-errno', '-fno-semantic-interposition', '-fno-plt', '-flto', '-pthread',
           '-I', str(ROOT), '-I', str(EXP / 'include'), str(RUN / 'bridge_probe.cpp'),
           str(EXP / 'league/top_replay_library/source/agent.cpp'), str(EXP / 'league/public_router/source/agent.cpp'),
           str(EXP / 'league/public_router_v52/source/agent.cpp'), '-o', str(output / 'bridge_probe')]
(output / 'BRIDGE_BUILD.json').write_text(json.dumps({'command': command}, indent=2) + '\n')
subprocess.run(command, check=True)
print(output / 'bridge_probe')
