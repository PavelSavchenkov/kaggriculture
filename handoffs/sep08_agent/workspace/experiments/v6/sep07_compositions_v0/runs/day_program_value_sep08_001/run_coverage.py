from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
binary = Path(json.loads((RUN / 'BUILD.json').read_text())['binary'])
command = json.loads((binary.parent / 'build.json').read_text())['command']
command[command.index(str(EXP / 'src/arena.cpp'))] = str(RUN / 'coverage.cpp')
command[command.index('-o')+1] = str(RUN / 'coverage')
commands = [command, ['conda', 'run', '-n', 'kaggriculture', str(RUN / 'coverage'), str(RUN / 'coverage_results')]]
for i, command in enumerate(commands):
    with (RUN / f'coverage_command{i}.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
(RUN / 'COVERAGE_COMMANDS.json').write_text(json.dumps(commands, indent=2)+'\n')
rows = []
for path in sorted((RUN / 'coverage_results').glob('*.json')):
    data = json.loads(path.read_text())['games']
    assert data == json.loads((RUN / 'discovery' / path.name).read_text())['games']
    coverage = [json.loads(line) for line in path.with_suffix('.coverage.jsonl').read_text().splitlines()]
    assert [(g['seed'], g['seat']) for g in coverage] == [(g['seed'], g['seat']) for g in data]
    rows.append({'name': path.stem, 'games': len(data), 'activated_games': sum(bool(g['days']) for g in coverage),
        'daily_totals': [[sum(g['daily'][d][i] for g in coverage) for i in range(5)] for d in range(30)],
        'max_act_ms': max(g['max_act_ms'] for g in coverage),
        'mean_total_act_ms': sum(g['total_act_ms'] for g in coverage)/len(coverage),
        'mean_forecast_steps': sum(g.get('forecast_steps',0) for g in coverage)/len(coverage),
        'mean_predicted_gain': sum(g.get('predicted_gain',0) for g in coverage)/len(coverage),
        'value_rejected': sum(g.get('value_rejected',0) for g in coverage)})
report = {'full_game_records_equal': sum(r['games'] for r in rows), 'columns': ['active_hours', 'abandoned', 'screened', 'rejected', 'skipped'], 'rows': rows,
    'timing_scope': 'Wall time under concurrent audit load; not a controlled throughput comparison.'}
assert report['full_game_records_equal'] == 512
(RUN / 'COVERAGE.json').write_text(json.dumps(report, indent=2)+'\n')
print('512 coverage full records equal discovery.')
for row in rows:
    if '_m3_' in row['name']: print(row['name'], 'activated', row['activated_games'], 'days14..16', row['daily_totals'][14:17])
