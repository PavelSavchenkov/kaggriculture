"""Unused-seed C++ controls for discovery-selected public crop forecasts."""
import hashlib
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from branch_inputs import FAMILIES, flatten

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
OUT = RUN / 'fresh_1660000'


def execute(job):
    family, policy, rival, trace = job
    source = EXP / 'runs' / FAMILIES[family]['source']
    output = OUT / f'{family}_{policy}_vs_{rival}{"_trace" if trace else ""}.json'
    binary = source / 'build/generic' / ('diagnostics' if trace else 'arena')
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', policy, '--b', rival,
               '--games', '64', '--seed-start', '1660000', '--seat-mode', 'both',
               '--threads', '2', '--validate', '--output', str(output)]
    with output.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    return {'command': command, 'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
            'output': output.name, 'output_sha256': hashlib.sha256(output.read_bytes()).hexdigest()}


def main():
    OUT.mkdir(exist_ok=True)
    frozen = {'seed_start': 1660000, 'seeds': 64, 'both_seats': True,
              'selected_before_fresh': [{'integration': 64, 'crop_mode': m, 'feed_net': 0,
                                         'objective': 'margin'} for m in [0, 2, 4]],
              'scope': 'Offline rankings of exact fixed-course outcomes; no deployed forecast policy.',
              'discovery_sha256': hashlib.sha256((RUN / 'BRANCH_REPORT.json').read_bytes()).hexdigest(),
              'source_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                for p in (RUN / 'source').glob('*')}}
    checkpoint = OUT / 'PREREGISTRATION.json'
    assert not checkpoint.exists(), 'Fresh audit already started; do not overwrite.'
    checkpoint.write_text(json.dumps(frozen, indent=2) + '\n')
    jobs = [(family, policy, rival, False) for family, cfg in FAMILIES.items()
            for policy in cfg['branches'] for rival in cfg['rivals']]
    jobs += [(family, cfg['branches'][0], rival, True) for family, cfg in FAMILIES.items()
             for rival in cfg['rivals']]
    with ThreadPoolExecutor(max_workers=4) as pool:
        commands = list(pool.map(execute, jobs))
    for family, cfg in FAMILIES.items():
        source = EXP / 'runs' / cfg['source']
        models = json.loads((source / 'flow_models.json').read_text())
        inputs, expected = [], []
        for rival in cfg['rivals']:
            panels = [json.loads((OUT / f'{family}_{p}_vs_{rival}.json').read_text())['games']
                      for p in cfg['branches']]
            traces = json.loads((OUT / f'{family}_{cfg["branches"][0]}_vs_{rival}_trace.json').read_text())
            for index, t in enumerate(traces):
                games = [panel[index] for panel in panels]
                assert all((g['seed'], g['seat']) == (t['seed'], t['seat']) for g in games)
                assert all(g['shops'] == games[0]['shops'] for g in games)
                inputs.append([rival, t['seed'], t['seat'], games[0]['shops']])
                prefix = cfg['prefix']
                expected.append({'rival': rival, 'seed': t['seed'], 'seat': t['seat'],
                                 'own_cash': t[f'cash_before{prefix}'],
                                 'rival_cash': t[f'rival_cash_before{prefix}'],
                                 'physical': t[f'physical_before{prefix}'],
                                 'own_final': [g['cash'] for g in games],
                                 'rival_final': [g['opponent_cash'] for g in games],
                                 'margins': [g['cash'] - g['opponent_cash'] for g in games],
                                 'shops': games[0]['shops']})
        input_path = OUT / f'{family}_inputs.txt'
        with input_path.open('w') as f:
            f.write(str(len(models)) + '\n')
            f.write(' '.join(flatten([[m['sales'], m['buys'], m['fixed_costs']] for m in models])) + '\n')
            f.write(str(len(inputs)) + '\n')
            for row in inputs:
                f.write(' '.join(flatten(row)) + '\n')
        (OUT / f'{family}_expected.json').write_text(json.dumps(expected, separators=(',', ':')) + '\n')
        binary = RUN / 'build' / f'{family}_forecast'
        command = ['conda', 'run', '-n', 'kaggriculture', str(binary), str(input_path),
                   str(OUT / f'{family}_predictions.json')]
        subprocess.run(command, check=True)
        commands.append({'command': command, 'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest()})
    (OUT / 'COMMANDS.json').write_text(json.dumps(commands, indent=2) + '\n')


if __name__ == '__main__':
    main()
