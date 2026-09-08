from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
source = EXP/'runs/day_service_bank_sep08_001'
binary = json.loads((source/'BUILD.json').read_text())['binary']
out = RUN/'reverse'
out.mkdir(exist_ok=False)


def run(opponent):
    path = out/f'{opponent}_vs_service_bank_p362_m2.json'
    command = ['conda','run','-n','kaggriculture',binary,'--a',opponent,'--b','service_bank_p362_m2',
        '--games','128','--seed-start','2300000','--seat-mode','both','--threads','4',
        '--budget-expansions','100000','--validate','--profile','--output',str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    reverse = json.loads(path.read_text())['games']
    old = json.loads((source/'fresh'/f'service_bank_p362_m2_vs_{opponent}.json').read_text())['games']
    lookup = {(g['seed'],g['seat']):g for g in old}
    for g in reverse:
        match = lookup[g['seed'],g['seat']^1]
        for a,b in [('cash','opponent_cash'),('opponent_cash','cash'),('action_hash','opponent_action_hash'),
                    ('opponent_action_hash','action_hash'),('profile','opponent_profile'),('opponent_profile','profile')]:
            assert g[a] == match[b], (opponent,g['seed'],g['seat'],a)
    print(opponent,'256 exact reversed records; opponent production now exported.',flush=True)
    return command


with ThreadPoolExecutor(max_workers=2) as pool:
    commands = list(pool.map(run,['empty_sale_slots_m2','public_router']))
(RUN/'REVERSE_EXPORT.json').write_text(json.dumps({'games':512,'all_cash_action_hashes_profiles_exact':True,'commands':commands},indent=2)+'\n')
