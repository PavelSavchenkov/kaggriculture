"""Verify stratification and complete-game identity with selected frozen courses."""
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

from prepare import NAMES
from diagnose import REFERENCE, RIVALS

RUN = Path(__file__).resolve().parent
SOURCE = RUN.parent / 'atakan_portfolio_001'
EXP = RUN.parents[1]
ROOT = EXP.parents[2]


def core(game):
    return {k:v for k,v in game.items() if k not in ('profile', 'opponent_profile')}


def main():
    tests = RUN / 'build/tests';tests.mkdir(parents=True, exist_ok=True)
    for name in ['sampling_checks.cpp', 'agent.cpp', 'agent.hpp', 'data.inc']:
        shutil.copyfile(RUN / 'source' / name, tests / name)
    snapshot = RUN / 'build/generic/source_snapshot'
    command = ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O2', '-I', str(snapshot),
               str(tests / 'sampling_checks.cpp'), '-o', str(tests / 'sampling_checks')]
    subprocess.run(command, check=True)
    run = ['conda', 'run', '-n', 'kaggriculture', str(tests / 'sampling_checks')]
    result = json.loads(subprocess.check_output(run, text=True))
    decisions = json.loads((RUN / 'decision_cases.json').read_text())
    choices = {(r['name'],r['rival'],r['seed'],r['seat']):r['branch'] for r in decisions}
    exact, counts = 0, {}
    for rival in RIVALS+[REFERENCE]:
        controls = []
        for animal in ['cow','sheep','goose']:
            panel = json.loads((SOURCE / f'results/atakan_{animal}_vs_{rival}.json').read_text())
            controls.append({(g['seed'],g['seat']):g for g in panel['games']})
        for name in NAMES:
            panel = json.loads((RUN / f'results/{name}_vs_{rival}.json').read_text())
            for game in panel['games']:
                branch = choices[name,rival,game['seed'],game['seat']]
                assert core(game) == core(controls[branch][game['seed'],game['seat']]), (name,rival,game['seed'],game['seat'])
                exact += 1
                counts[name] = counts.get(name,0)+1
    assert exact == 3840
    out = {'sampling': result, 'full_games_matching_forecast_selected_fixed_course': exact,
           'per_policy_games': counts, 'commands': [command, run],
           'sampling_binary_sha256': hashlib.sha256((tests / 'sampling_checks').read_bytes()).hexdigest()}
    (RUN / 'SOURCE_REALIZATION_PARITY.json').write_text(json.dumps(out, indent=2)+'\n')
    print(json.dumps({k:v for k,v in out.items() if k != 'commands'}, indent=2))


if __name__ == '__main__':
    main()
