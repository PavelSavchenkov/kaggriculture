from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import csv
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
binary = EXP / 'build/faace656c5bb3083d227/arena'
output = RUN / 'selector_parity'
output.mkdir(exist_ok=False)


def run(opponent):
    path = output / f'late_value_s32_t0_r05_vs_{opponent}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', 'late_value_s32_t0_r05', '--b', opponent,
               '--games', '512', '--seed-start', '1790000', '--seat-mode', 'both', '--threads', '6', '--validate', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    actual = json.loads(path.read_text())['games']
    options = [json.loads((RUN / f'counterfactuals/force{c}_vs_{opponent}.json').read_text())['games'] for c in range(4)]
    rows = list(csv.DictReader((RUN / f'counterfactuals/force0_vs_{opponent}.json.predictions.csv').open()))
    choices, expected = [], []
    for i, row in enumerate(rows):
        scores = [0.] + [float(row[f'margin_{c}']) - .5 * float(row[f'deviation_{c}']) if int(row[f'eligible_{c}']) else -1e20 for c in range(1, 4)]
        choice = max(range(4), key=lambda c: scores[c])
        choices.append(choice)
        expected.append(options[choice][i])
    result = {'opponent': opponent, 'games': len(actual), 'full_records_equal': actual == expected,
              'choices': [choices.count(c) for c in range(4)]}
    print(result, flush=True)
    return result


with ThreadPoolExecutor(max_workers=2) as pool:
    rows = list(pool.map(run, ['opening_q32_b13_v1', 'public_router']))
report = {'games': sum(r['games'] for r in rows), 'full_records_equal': all(r['full_records_equal'] for r in rows), 'opponents': rows}
(RUN / 'SELECTOR_PARITY.json').write_text(json.dumps(report, indent=2) + '\n')
assert report['full_records_equal'], report
