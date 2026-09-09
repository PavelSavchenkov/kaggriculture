from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import json
import statistics
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
cases=[(leaf,kind) for leaf in ['off','on'] for kind in ['fixed','no_care']]
def run(case):
    leaf,kind=case
    mapping=(RUN/f'{leaf}_combined30.txt').read_text()
    if leaf=='off':mapping+=f'23 {RUN}/off_day23_fixed120/actions.txt\n'
    mapping+=f'29 {RUN}/{leaf}_day29_{kind}_v3/actions.txt\n'
    name=f'{leaf}_terminal_{kind}_audit';map_path=RUN/(name+'.txt');map_path.write_text(mapping)
    source=EXP/f'runs/late_animal_rotation_001/goose_c31_d13_{leaf}_002';output=RUN/name
    command=['conda','run','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),str(RUN/f'build/audit_{leaf}'),str(source),str(map_path),str(output)]
    with (RUN/(name+'.log')).open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    original=json.loads((output/'original.json').read_text())['games']
    assert original==json.loads((source/'exact/candidate.json').read_text())['games']
    candidate=json.loads((output/'candidate.json').read_text())['games']
    control=json.loads((EXP/f'runs/late_animal_rotation_001/control_c31_d13_{leaf}_002/exact/candidate.json').read_text())['games']
    activated=[i for i,g in enumerate(original) if any(l[0]==9 and l[1:3]==[1,3] and l[5]==13 for l in g['profile']['lives'])]
    result={'leaf':leaf,'kind':kind,'command':command,'games':len(candidate),'activated_games':len(activated),
            'produced_equal':sum(a['produced']==b['produced'] for a,b in zip(candidate,original)),
            'sold_equal':sum(a['sold']==b['sold'] for a,b in zip(candidate,original)),
            'rival_actions_equal':sum(a['opponent_action_hash']==b['opponent_action_hash'] for a,b in zip(candidate,original)),
            'activated_output_gain':[statistics.mean(candidate[i]['produced'][p]-original[i]['produced'][p] for i in activated) for p in range(12)],
            'activated_sold_gain':[statistics.mean(candidate[i]['sold'][p]-original[i]['sold'][p] for i in activated) for p in range(12)],
            'activated_cash_gain':statistics.mean(candidate[i]['cash']-original[i]['cash'] for i in activated),
            'activated_hire_cost_gain':statistics.mean(candidate[i]['profile']['hire_cost']-original[i]['profile']['hire_cost'] for i in activated),
            'activated_hires_gain':statistics.mean(candidate[i]['profile']['hires']-original[i]['profile']['hires'] for i in activated),
            'mean_margin_vs_crop_control':statistics.mean(a['cash']-a['opponent_cash']-b['cash']+b['opponent_cash'] for a,b in zip(candidate,control)),
            'min_cash_gain':min(a['cash']-b['cash'] for a,b in zip(candidate,original))}
    (RUN/(name+'.json')).write_text(json.dumps(result,indent=2)+'\n')
    print(result,flush=True)
    return result
with ThreadPoolExecutor(max_workers=2) as pool:results=list(pool.map(run,cases))
(RUN/'TERMINAL_AUDIT.json').write_text(json.dumps(results,indent=2)+'\n')
