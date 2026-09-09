from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
commands = [['conda','run','-n','kaggriculture','g++','-std=c++20','-O3','-DNDEBUG','-march=native','-mtune=native',
    '-fno-exceptions','-fno-rtti','-fno-math-errno','-fno-semantic-interposition','-fno-plt','-flto','-pthread',
    '-I',str(ROOT),str(RUN/'coverage.cpp'),str(EXP/'runs/compiler_split_sep08_001/compiler/source/agent.cpp'),
    str(EXP/'league/public_router/source/agent.cpp'),'-o',str(RUN/'coverage')],
    ['conda','run','-n','kaggriculture',str(RUN/'coverage'),str(RUN/'coverage_results')]]
for i,command in enumerate(commands):
    with (RUN/f'coverage_command{i}.log').open('w') as log:
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
(RUN/'COVERAGE_COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
print('Coverage games completed; compare with discovery after that run finishes.')
