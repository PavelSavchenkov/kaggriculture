from pathlib import Path
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
commands=[['conda','run','-n','kaggriculture','g++','-std=c++20','-O3','-DNDEBUG','-march=native','-mtune=native',
    '-fno-exceptions','-fno-rtti','-fno-math-errno','-fno-semantic-interposition','-fno-plt','-flto','-pthread','-I',str(ROOT),
    str(RUN/'trace.cpp'),str(EXP/'runs/compiler_placement_sep08_001/compiler/source/agent.cpp'),
    str(EXP/'league/public_router/source/agent.cpp'),'-o',str(RUN/'trace')],
    ['conda','run','-n','kaggriculture',str(RUN/'trace'),str(RUN/'traces')]]
for i,command in enumerate(commands):
    with (RUN/f'command{i}.log').open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
(RUN/'COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
count=0
for path in (RUN/'traces').glob('mode*.json'):
    actual=json.loads(path.read_text());name={-1:'control',0:'full',2:'establish'}[actual['mode']]
    data=json.loads((EXP/f'runs/cold_day_tasks_sep08_001/discovery/cold_day_tasks_{name}_vs_public_router.json').read_text())
    expected=next(g for g in data['games'] if g['seed']==actual['seed'] and g['seat']==actual['seat'])
    for key in ['cash','opponent_cash','action_hash','opponent_action_hash']:assert actual[key]==expected[key],(path,key)
    count+=1
(RUN/'TRACE_CHECKS.json').write_text(json.dumps({'games':count,'both_action_hashes_and_final_cash_equal':True},indent=2)+'\n')
print('Verified',count,'instrumented full games against saved records.')
