from pathlib import Path
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
binary=RUN/'controls'
command=['conda','run','-n','kaggriculture','g++','-std=c++20','-O3','-DNDEBUG','-march=native',
         '-mtune=native','-fno-exceptions','-fno-rtti','-fno-math-errno','-fno-semantic-interposition','-fno-plt','-flto','-pthread',
         '-I',str(ROOT),str(RUN/'controls.cpp'),str(EXP/'candidates/composition_greedy_v0/source/agent.cpp'),
         str(EXP/'league/top_replay_library/source/agent.cpp'),str(EXP/'league/public_router/source/agent.cpp'),'-o',str(binary)]
with (RUN/'controls_build.log').open('w') as log:
    subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
run=['conda','run','-n','kaggriculture',str(binary),str(RUN/'controls_results')]
(RUN/'CONTROL_COMMANDS.json').write_text(json.dumps([command,run],indent=2)+'\n')
subprocess.run(run,check=True)
