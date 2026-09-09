"""Reproduce construction fixtures, mixed endpoints and the forecast in fresh outputs."""
from pathlib import Path
import argparse
import csv
import hashlib
import json
import subprocess

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
ENV = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture']


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--session', type=Path, default=HERE / '_work/session')
    parser.add_argument('--output', type=Path, default=HERE / '_work/compiler_validation')
    args = parser.parse_args()
    session = args.session.resolve()
    exp = session / 'experiments/v6/sep07_compositions_v0'
    group = exp / 'runs/animal_group_policy_sep08_001'
    mixed = exp / 'runs/mixed_funding_sep08_001'
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    commands = []

    def run(command, label):
        commands.append(command)
        with (out / (label + '.log')).open('w') as log:
            subprocess.run(ENV + command, cwd=session, stdout=log, stderr=subprocess.STDOUT, check=True)

    for folder, targets in [(group, ['check_policy', 'audit_public', 'compile_flexible']), (mixed, ['forecast_v2'])]:
        run(['cmake', '-S', str(folder), '-B', str(folder / 'build')], folder.name + '_configure')
        run(['cmake', '--build', str(folder / 'build'), '--target', *targets, '-j', '4'], folder.name + '_build')
    run([str(group / 'build/check_policy'), str(group / 'LIBRARY_FIXTURES.txt')], 'fixtures')
    fixture_lines = list(csv.DictReader((out / 'fixtures.log').open()))
    assert len(fixture_lines) == 12
    prefix = mixed / 'season_30s'
    days = list(csv.DictReader((prefix / 'compile.csv').open()))
    extra = next(row['extra_hires'] for row in reversed(days) if row['day'] == '29' and row['endpoint'] == '1')
    last = prefix / 'days/29'
    audit = out / 'audit'
    run([str(session / 'day_solver/with_runtime.sh'), str(group / 'build/audit_public'), str(prefix),
         str(last / f'raw_schedule_h{extra}.txt'), str(last / f'problem_h{extra}.json'),
         str(last / f'orders_h{extra}.txt'), '1008', str(audit)], 'mixed_audit')
    result = json.loads((audit / 'MATCHED_RESULT.json').read_text())
    expected = json.loads((prefix / 'MATCHED_RESULT.json').read_text())
    for key in ['cash', 'rival_cash', 'produced0', 'produced1', 'parent_cash', 'parent_rival_cash']:
        assert result[key] == expected[key], key
    daily = list(csv.DictReader((audit / 'daily.csv').open()))
    assert len(daily) == 30 and all(row['exact'] == '1' for row in daily)
    assert json.loads((audit / 'STATUS.json').read_text())['turns'] == 719
    forecast = out / 'forecast.csv'
    run([str(mixed / 'build/forecast_v2'), str(forecast)], 'forecast')
    assert forecast.read_bytes() == (mixed / 'FORECAST_V2.csv').read_bytes()
    rows = list(csv.DictReader((mixed / 'mixed_d9_BIOLOGY_V2.csv').open()))
    assert len(rows) == 9 and all(r['estimated_output_gain'] == r['realized_output_gain'] for r in rows)
    report = {'construction_fixture_games': 12, 'mixed_endpoints_exact': 30, 'mixed_turns': 719,
              'mixed_own_gain': result['cash'] - result['parent_cash'],
              'mixed_rival_gain': result['rival_cash'] - result['parent_rival_cash'],
              'forecast_csv_sha256': hashlib.sha256(forecast.read_bytes()).hexdigest(),
              'forecast_byte_identical': True, 'production_deltas_exact': 9, 'commands': commands,
              'scope': 'Restored experiment inputs and committed engine/day solver; new outputs, no live experiment read.'}
    (out / 'RESULTS.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
