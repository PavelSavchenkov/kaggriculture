from pathlib import Path
import hashlib
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
commands=[['conda','run','-n','kaggriculture','cmake','-S',str(RUN),'-B',str(RUN/'build'),'-DCMAKE_BUILD_TYPE=Release'],
    ['conda','run','-n','kaggriculture','cmake','--build',str(RUN/'build'),'--target','compile_day','-j2'],
    ['conda','run','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),str(RUN/'build/compile_day'),str(RUN/'compiled'),'20']]
(RUN/'COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
for i,command in enumerate(commands):
    with (RUN/f'command{i}.log').open('w') as log:
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    print('Stage',i,'complete',flush=True)
(RUN/'SOURCE_HASHES.json').write_text(json.dumps({str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest()
    for p in [RUN/'compile_day.cpp',RUN/'run.py',RUN/'CMakeLists.txt',EXP/'include/day_contract.hpp',EXP/'include/estimate.hpp',
              EXP/'runs/compiler_labor_sep08_001/cold_farm.hpp',EXP/'runs/compiler_placement_sep08_001/compiler/source/agent.cpp']},indent=2)+'\n')
print((RUN/'compiled/RESULTS.json').read_text())
