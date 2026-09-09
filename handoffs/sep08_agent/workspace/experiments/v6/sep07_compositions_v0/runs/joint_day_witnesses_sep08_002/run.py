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
path = 'compiled/joint_routes_p362_m1'
source = json.loads((RUN / path / 'SOURCE_GAME.json').read_text())
expected = json.loads((EXP / f'runs/joint_day_witnesses_sep08_001/{path}/SOURCE_GAME.json').read_text())
assert source == expected
rows = [json.loads(line) for line in (RUN / path / 'results.jsonl').read_text().splitlines()]
assert len(rows) == 3
report = {'source_full_game_and_start_state_exact': True, 'cases': rows,
    'all_solved_full_engine_exact': all(r['full_endpoint_equal'] and r['cash_equal'] for r in rows if r['solved']),
    'scope': 'Only the three prior timeouts. UNKNOWN is not infeasible; initial45 exact results remain separate.'}
(RUN / 'RESULTS.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2), flush=True)
