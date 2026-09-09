from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
commands = [
    ['conda', 'run', '-n', 'kaggriculture', 'cmake', '-S', str(RUN), '-B', str(RUN / 'build'), '-DCMAKE_BUILD_TYPE=Release'],
    ['conda', 'run', '-n', 'kaggriculture', 'cmake', '--build', str(RUN / 'build'), '--target', 'compile_days', '-j2'],
    ['conda', 'run', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'), str(RUN / 'build/compile_days'), str(RUN / 'compiled'), '8'],
]
(RUN / 'COMMANDS.json').write_text(json.dumps(commands, indent=2) + '\n')
for i, command in enumerate(commands):
    with (RUN / f'command{i}.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    print('Stage', i, 'complete', flush=True)
path='compiled/day_program_p362_m3'
source=json.loads((RUN/path/'SOURCE_GAME.json').read_text())
expected=next(g for g in json.loads((EXP/'runs/day_programs_sep08_001/discovery/day_program_p362_m3_vs_joint_routes_p362_m0.json').read_text())['games'] if g['seed']==1006 and g['seat']==0)
for key in ['seed','seat','cash','opponent_cash','action_hash','opponent_action_hash']:assert source[key]==expected[key],(key,source[key],expected[key])
rows=[json.loads(line) for line in (RUN/path/'results.jsonl').read_text().splitlines()]
assert len(rows)==4
assert all(r['full_endpoint_equal'] and r['cash_equal'] for r in rows if r['solved'])
report={'source_full_game_equal':True,'cases':rows,'scope':'Actual relaxed-day-program p362 day16 entry after prior day14/15 changes. Pending current-state service, original versus two fewer workers; full-engine endpoints for all solved cases. UNKNOWN is not infeasibility.'}
(RUN/'RESULTS.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2),flush=True)
