from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
commands = [
    ['conda', 'run', '-n', 'kaggriculture', 'cmake', '-S', str(RUN), '-B', str(RUN / 'build'), '-DCMAKE_BUILD_TYPE=Release'],
    ['conda', 'run', '-n', 'kaggriculture', 'cmake', '--build', str(RUN / 'build'), '--target', 'compile_days', '-j2'],
    ['conda', 'run', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'), str(RUN / 'build/compile_days'), str(RUN / 'compiled'), '2'],
]
(RUN / 'COMMANDS.json').write_text(json.dumps(commands, indent=2) + '\n')
for i, command in enumerate(commands):
    with (RUN / f'command{i}.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    print('Stage', i, 'complete', flush=True)
rows = []
starts = {}
for case in ['p355', 'p362']:
    for mode in [0, 1]:
        name = f'joint_routes_{case}_m{mode}'
        directory = RUN / 'compiled' / name
        source = json.loads((directory / 'SOURCE_GAME.json').read_text())
        expected = json.loads((EXP / f'runs/joint_day_routes_sep08_001/discovery/{name}_vs_public_router.json').read_text())['games']
        expected = next(g for g in expected if g['seed'] == 1000 and g['seat'] == 0)
        for key in ['cash', 'opponent_cash', 'action_hash', 'opponent_action_hash']:
            assert source[key] == expected[key], (name, key)
        starts[case, mode] = source['day14_start_hash']
        cases = [json.loads(line) for line in (directory / 'results.jsonl').read_text().splitlines()]
        assert len(cases) == 12
        rows.extend({'agent': name, **r} for r in cases)
    assert starts[case, 0] == starts[case, 1]
report = {'source_full_games_exact': 4, 'same_day14_start_state_for_each_pair': True, 'cases': rows,
          'solved': sum(r['solved'] for r in rows),
          'all_solved_cases_full_engine_exact': all(r['full_endpoint_equal'] and r['cash_equal'] for r in rows if r['solved']),
          'scope': 'Offline contracts for days14..16 on two cold farms under original and joint routes. Augmentation closes currently requested production/service work, including newly enabled harvest after water; construction tasks remain excluded. Market trades stay fixed. Solved days are not yet general deployable full-season policies.'}
(RUN / 'RESULTS.json').write_text(json.dumps(report, indent=2) + '\n')
paths = [RUN / 'compile.cpp', RUN / 'run.py', RUN / 'CMakeLists.txt', EXP / 'include/day_contract.hpp',
         EXP / 'runs/joint_day_routes_sep08_001/compiler/source/agent.cpp', EXP / 'runs/joint_day_routes_sep08_001/routes.hpp']
(RUN / 'SOURCE_HASHES.json').write_text(json.dumps({str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}, indent=2) + '\n')
print('Four source games exact; cases', len(rows), 'solved', report['solved'], 'full-engine checks', report['all_solved_cases_full_engine_exact'])
