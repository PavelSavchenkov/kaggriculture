from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
commands = [['conda','run','-n','kaggriculture','g++','-std=c++20','-O3','-DNDEBUG','-march=native','-mtune=native',
    '-fno-exceptions','-fno-rtti','-fno-math-errno','-fno-semantic-interposition','-fno-plt','-flto','-pthread',
    '-I',str(ROOT),str(RUN/'trace.cpp'),str(RUN/'source/agent.cpp'),
    str(EXP/'league/public_router_v52/source/agent.cpp'),'-o',str(RUN/'trace')],
    ['conda','run','-n','kaggriculture',str(RUN/'trace'),str(RUN/'trace_results')]]
for i,command in enumerate(commands):
    with (RUN/f'trace_command{i}.log').open('w') as log:
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
(RUN/'TRACE_COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
print('Eight full clamp diagnostic traces completed.')
