from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
opponents = ['empty_sale_slots_m2','teammate_shoprouter','public_router',
    'public_router_v52','joint_routes_p362_m0','service_bank_p362_m2']
names = ['shop_branch_m0','shop_branch_m1','shop_branch_m2']
parents = ['service_bank_p362_m2','cold_renewal_p98']
command = ['conda','run','-n','kaggriculture','python',str(EXP/'scripts/build_arena.py'),
    '--agents',*dict.fromkeys(names+parents+opponents)]
result = subprocess.run(command, capture_output=True, text=True)
(RUN/'build.log').write_text(result.stdout+result.stderr)
result.check_returncode()
binary = result.stdout.strip().splitlines()[-1]
(RUN/'BUILD.json').write_text(json.dumps({'command':command,'binary':binary}, indent=2)+'\n')
out = RUN/'parity'
out.mkdir(exist_ok=False)


def run(job):
    name, opponent, native = job
    seeds = 128 if name == 'shop_branch_m2' and not native else 8
    path = out/f'{name}_vs_{opponent}_{native}.json'
    command = ['conda','run','-n','kaggriculture',binary,'--a',name,'--b',opponent,
        '--games',str(seeds),'--seed-start','2400000','--seat-mode','both',
        '--threads','4','--budget-expansions','100000','--validate','--profile',
        '--output',str(path)]
    if native: command.append('--native-shops')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    print(name,opponent,'native',native,'complete',flush=True)
    return command


jobs = [(n,o,0) for o in opponents for n in names]
jobs += [(n,o,1) for o in opponents for n in [names[2],*parents]]
with ThreadPoolExecutor(max_workers=2) as pool:
    commands = list(pool.map(run,jobs))
(RUN/'PARITY_COMMANDS.json').write_text(json.dumps(commands, indent=2)+'\n')
checked = 0
branches = [0,0]
for native in range(2):
    for opponent in opponents:
        def get(name):
            path = out/f'{name}_vs_{opponent}_{native}.json' if native else EXP/'runs/cold_renewal_sep08_001/fresh'/f'{name}_vs_{opponent}.json'
            return json.loads(path.read_text())['games']
        originals = [get(n) for n in parents]
        actual = json.loads((out/f'shop_branch_m2_vs_{opponent}_{native}.json').read_text())['games']
        for i, game in enumerate(actual):
            shops = originals[0][i]['shops'][:2]
            choice = int(not any(s in [0,1,7] for s in shops))
            assert game == originals[choice][i], (native,opponent,game['seed'],game['seat'],choice)
            branches[choice] += 1
            checked += 1
        if not native:
            for mode in range(2):
                records = json.loads((out/f'shop_branch_m{mode}_vs_{opponent}_0.json').read_text())['games']
                assert records == originals[mode][:len(records)]
                checked += len(records)
report = {'full_game_records_checked':checked, 'selector_branch_counts':branches,
    'all_complete_records_equal':True, 'custom_games':1728, 'native_games':288,
    'scope':'Every full-game field equals the selected unchanged constituent on the prior exposed panel. Native selection uses only the common first two shops; future shops may diverge after the branch.'}
(RUN/'PARITY.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report), flush=True)
