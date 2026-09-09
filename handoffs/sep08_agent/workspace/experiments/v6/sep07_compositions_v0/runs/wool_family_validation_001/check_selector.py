from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import csv
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
spec = json.loads((RUN / 'FRESH_PREREGISTERED.json').read_text())
binary = ROOT / json.loads((RUN / 'FRESH_BUILD.json').read_text())['binary']
diagnosis = EXP / 'runs/v52_family_001/counterfactuals'
output = RUN / 'selector_checks'
output.mkdir(exist_ok=False)
opponents = ['late_value_s32_t0_r05', 'public_router_v52', 'public_router']


def run(opponent):
    path = output / f'{opponent}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', spec['candidate'], '--b', opponent,
               '--games', '1024', '--seed-start', '1000', '--seat-mode', 'both', '--threads', '6', '--validate', '--output', str(path)]
    path.with_suffix('.command.json').write_text(json.dumps(command, indent=2) + '\n')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    actual = json.loads(path.read_text())['games']
    alternatives = {a: json.loads((diagnosis / f'{a}_vs_{opponent}.json').read_text())['games'] for a in ['baseline', 'force']}
    features = list(csv.DictReader((diagnosis / f'force_vs_{opponent}.json.features.csv').open()))
    changed = 0
    for i, (game, feature) in enumerate(zip(actual, features)):
        first, second = int(float(feature['f0'])), int(float(feature['f1']))
        take = int(feature['selected']) == 1 and ((first == 7 and second != 4) or (second == 7 and first in [5, 6]))
        changed += take
        expected = alternatives['force' if take else 'baseline'][i]
        expected = {k: v for k, v in expected.items() if k not in ['profile', 'opponent_profile']}
        assert game == expected, (opponent, i, game['seed'], take)
    return {'opponent': opponent, 'games': len(actual), 'changed': changed, 'all_game_fields_and_action_hashes_equal': True}


with ThreadPoolExecutor(max_workers=2) as pool:
    reports = list(pool.map(run, opponents))
report = {'games': sum(r['games'] for r in reports), 'opponents': reports,
          'scope': 'Every emitted game field and both action hashes exactly match the independently executed chosen counterfactual. Detailed profile objects were omitted from this second execution.',
          'all_game_fields_equal': True}
(RUN / 'SELECTOR_PARITY.json').write_text(json.dumps(report, indent=2) + '\n')
print(report)
