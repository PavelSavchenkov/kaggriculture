from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
commands = [
    ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O2', '-g', '-DKAG_VERIFY_MASKS',
     '-fno-exceptions', '-fno-rtti', '-pthread', '-I', str(ROOT), str(RUN / 'verify_model.cpp'),
     str(EXP / 'runs/joint_day_routes_sep08_001/compiler/source/agent.cpp'),
     str(EXP / 'league/top_replay_library/source/agent.cpp'), str(EXP / 'league/public_router/source/agent.cpp'),
     '-o', str(RUN / 'verify_model')],
    ['conda', 'run', '-n', 'kaggriculture', str(RUN / 'verify_model'), str(RUN / 'model_checks')],
]
(RUN / 'MODEL_COMMANDS.json').write_text(json.dumps(commands, indent=2) + '\n')
for i, command in enumerate(commands):
    with (RUN / f'model_command{i}.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
expected = json.loads((EXP / 'runs/joint_day_routes_sep08_001/discovery/joint_routes_p355_m0_vs_public_router.json').read_text())['games']
for seed in [1000, 1001]:
    for seat in [0, 1]:
        actual = json.loads((RUN / f'model_checks/{seed}_s{seat}.json').read_text())
        source = next(g for g in expected if g['seed'] == seed and g['seat'] == seat)
        for key in ['cash', 'opponent_cash', 'action_hash', 'opponent_action_hash']:
            assert actual[key] == source[key], (seed, seat, key)
report = json.loads((RUN / 'model_checks/CHECKS.json').read_text())
assert report['current_own_observations_exact'] == 2876
assert report['daytime_solo_steps_exact'] == 2760
assert report['nonempty_rival_stock_excluded_cases'] > 0
report.update({'source_full_games_exact': 4,
    'scope': 'Observation-built own state and deterministic market timers exactly match actual-engine own actions against PASS on non-night turns. This does not predict real rival actions, private inventory, future shops or random night weeds.',
    'source_hashes': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [RUN / 'model.hpp', RUN / 'verify_model.cpp', RUN / 'verify_model.py']}})
(RUN / 'MODEL_CHECKS.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
