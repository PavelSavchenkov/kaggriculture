from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
commands = [['conda','run','-n','kaggriculture','cmake','-S',str(RUN),'-B',str(RUN/'build'),'-DCMAKE_BUILD_TYPE=Release'],
    ['conda','run','-n','kaggriculture','cmake','--build',str(RUN/'build'),'--target','compile_days','-j2'],
    ['conda','run','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),str(RUN/'build/compile_days'),str(RUN/'compiled'),'8']]
(RUN/'COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
for i,command in enumerate(commands):
    with (RUN/f'command{i}.log').open('w') as log:
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    print('Stage',i,'complete',flush=True)
source = json.loads((RUN/'compiled/SOURCE_GAME.json').read_text())
expected = json.loads((EXP/'runs/dated_expansion_sep08_001/discovery/dated_expansion_p362_vs_public_router.json').read_text())['games']
expected = next(g for g in expected if g['seed']==1000 and g['seat']==0)
for key in ['cash','opponent_cash','action_hash','opponent_action_hash']:
    assert source[key] == expected[key],key
rows = [json.loads(line) for line in (RUN/'compiled/results.jsonl').read_text().splitlines()]
report = {'source_full_game_parity':True,'cases':rows,
    'all_solved_cases_full_engine_exact':all(r['full_endpoint_equal'] and r['cash_equal'] for r in rows if r['solved']),
    'scope':'Offline day contract from a newly constructed cold farm, with missed FEED and useful CARE added from preserved wheat reserves. Both full-game hashes/cash checked before day experiments. A solved day is not yet a deployable full-season policy.'}
(RUN/'RESULTS.json').write_text(json.dumps(report,indent=2)+'\n')
(RUN/'SOURCE_HASHES.json').write_text(json.dumps({str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest()
    for p in [RUN/'compile.cpp',RUN/'run.py',RUN/'CMakeLists.txt',EXP/'include/day_contract.hpp',
              EXP/'runs/compiler_care_sep08_001/compiler/source/agent.cpp',
              EXP/'runs/dated_expansion_sep08_001/proposals/dated_expansion_p362/source/plan.inc']},indent=2)+'\n')
print(json.dumps(report,indent=2),flush=True)
