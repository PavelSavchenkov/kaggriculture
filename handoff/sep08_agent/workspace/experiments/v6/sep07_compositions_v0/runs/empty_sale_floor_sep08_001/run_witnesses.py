from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
parent = 'observed_sale_lead_start_216'
names = json.loads((RUN / 'LINEAGE.json').read_text())['variants'] + [parent]
binary = json.loads((RUN / 'BUILD.json').read_text())['binary']
out = RUN / 'witnesses'
out.mkdir(exist_ok=False)
plan = {'created_utc': datetime.now(timezone.utc).isoformat(),
        'scope': 'Previously exposed native discovery and failed fresh witness. No unused audit claim.',
        'names': names, 'native_seeds': [2004000, 128], 'custom_seeds': [2000038, 1], 'seat_mode': 'both'}
(RUN / 'WITNESS_PLAN.json').write_text(json.dumps(plan, indent=2) + '\n')


def run(job):
    name, native = job
    label = 'native' if native else 'custom'
    path = out / f'{label}_{name}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', binary, '--a', name, '--b', parent,
               '--games', '128' if native else '1', '--seed-start', '2004000' if native else '2000038',
               '--seat-mode', 'both', '--threads', '4', '--validate', '--profile', '--output', str(path)]
    if native:
        command.append('--native-shops')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    d = json.loads(path.read_text())
    print(label, name, d['win_utility'], d['mean_margin'], flush=True)
    return command


with ThreadPoolExecutor(max_workers=2) as pool:
    commands = list(pool.map(run, [(name, native) for native in [True, False] for name in names]))
(RUN / 'WITNESS_COMMANDS.json').write_text(json.dumps(commands, indent=2) + '\n')
old = EXP / 'runs/empty_sale_validation_sep08_001'
report = {'games': 1290, 'scope': plan['scope'], 'rows': {}, 'native_complete_old_controls': 0, 'custom_endpoint_controls': 0}
for native in [True, False]:
    label = 'native' if native else 'custom'
    baseline = json.loads((out / f'{label}_{parent}.json').read_text())['games']
    for name in names:
        games = json.loads((out / f'{label}_{name}.json').read_text())['games']
        gains = [a['cash']-a['opponent_cash']-b['cash']+b['opponent_cash'] for a, b in zip(games, baseline)]
        report['rows'][f'{label}_{name}'] = {'minimum_parent_paired_margin': min(gains),
            'mean_parent_paired_margin': sum(gains)/len(gains),
            'negative_cases': [{'seed': g['seed'], 'seat': g['seat'], 'gain': gain} for g, gain in zip(games, gains) if gain < 0]}
        if name not in ['empty_sale_floor_m0', parent]:
            continue
        old_name = 'empty_sale_slots_m2' if name == 'empty_sale_floor_m0' else parent
        expected = json.loads((old / f'{"native" if native else "fresh"}/{old_name}_vs_{parent}.json').read_text())['games']
        expected = [g for g in expected if native or g['seed'] == 2000038]
        if native:
            assert games == expected
            report['native_complete_old_controls'] += len(games)
        else:
            for a, b in zip(games, expected):
                for key in ['seed', 'seat', 'cash', 'opponent_cash', 'action_hash', 'opponent_action_hash']:
                    assert a[key] == b[key]
            report['custom_endpoint_controls'] += len(games)
assert report['native_complete_old_controls'] == 512 and report['custom_endpoint_controls'] == 4
(RUN / 'WITNESS_ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2), flush=True)
