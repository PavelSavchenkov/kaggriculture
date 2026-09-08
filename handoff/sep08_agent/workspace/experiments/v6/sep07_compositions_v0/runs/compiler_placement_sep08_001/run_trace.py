from pathlib import Path
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
command=['conda','run','-n','kaggriculture','g++','-std=c++20','-O3','-DNDEBUG','-march=native','-mtune=native',
    '-fno-exceptions','-fno-rtti','-fno-math-errno','-fno-semantic-interposition','-fno-plt','-flto','-pthread','-I',str(ROOT),
    str(RUN/'trace.cpp'),str(RUN/'compiler/source/agent.cpp'),str(EXP/'league/public_router/source/agent.cpp'),'-o',str(RUN/'trace')]
with (RUN/'trace_build.log').open('w') as log:
    subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
run=['conda','run','-n','kaggriculture',str(RUN/'trace'),str(RUN/'traces')]
subprocess.run(run,check=True)
for mode in [0,1]:
    a=json.loads((RUN/f'traces/mode{mode}_game.json').read_text())['games'][0]
    old=json.loads((RUN/f'discovery/compiler_placement_mixed_m{mode}_vs_public_router.json').read_text())['games']
    b=next(g for g in old if g['seed']==1000 and g['seat']==0)
    assert a==b,mode
(RUN/'TRACE_CHECKS.json').write_text(json.dumps({'commands':[command,run],'complete_records_exact':2},indent=2)+'\n')
print('Both instrumented full games equal discovery records.')
