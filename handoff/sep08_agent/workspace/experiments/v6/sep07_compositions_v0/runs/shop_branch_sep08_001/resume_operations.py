"""Complete checks using equal budgets; preserve the initial invalid comparison."""
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
out = RUN/'operations'
binaries = json.loads((RUN/'OPERATIONAL_BINARIES.json').read_text())
variants = json.loads((RUN/'LINEAGE.json').read_text())['variants']
get = lambda name:json.loads((out/f'{name}.json').read_text())['games']
reference = get('pair')
assert all(get(name) == reference for name in ['generic','debug','one_thread'])
zero = get('zero_budget')
assert zero != reference
commands = []


def run(label,build,a,b,threads=2,budget=100000):
    path = out/f'{label}.json'
    assert not path.exists()
    command = ['conda','run','-n','kaggriculture',binaries[build]['binary'],'--a',a,'--b',b,
        '--games','8','--seed-start','2400000','--seat-mode','both','--threads',str(threads),
        '--budget-expansions',str(budget),'--validate','--profile','--output',str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    commands.append(command)
    return get(label)


for build,threads in [('pair',2),('debug',2),('generic',1)]:
    assert run(f'zero_{build}_{threads}',build,'shop_branch_m2','service_bank_p362_m2',threads,0) == zero
for name in variants:
    for opponent in [name,'pass']:
        run(f'{name}_vs_{opponent}','debug',name,opponent)
report = {'games':80+len(commands)*16,
    'generic_pair_debug_thread_full_records_equal':True,
    'zero_budget_generic_pair_debug_thread_full_records_equal':True,
    'all_variants_full_debug_self_and_pass':True,
    'initial_comparison_failed':{'script':'check_operations.py',
        'reason':'It incorrectly required zero-budget and 100000-budget games to be equal. The inherited day-program screen intentionally declines search when max_expansions is zero.',
        'evidence':'runs/day_programs_sep08_001/program.hpp:58,104',
        'different_games':sum(a!=b for a,b in zip(zero,reference)),
        'policy_source_changed':False},
    'state':'Both child policies and the latch are per-instance and reset; shared tables are immutable.',
    'observation':'Only legal own/public observations and public configuration.',
    'budget':'Same-budget records are exact across modes and threads. Zero budget uses the inherited fallback and returns legal actions.',
    'initial_commands_source':'check_operations.py','completion_commands':commands}
(RUN/'OPERATIONAL_CHECKS.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2),flush=True)
