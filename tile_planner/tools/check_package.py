"""Check the relocated C++ component using only its example and root dependencies."""
import argparse
import hashlib
import json
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

PACKAGE = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--build', type=Path, default=PACKAGE / 'build')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    example = PACKAGE / 'examples/rotation'
    expected = json.loads((example / 'EXPECTED.json').read_text())
    commands = []

    def run(name, arguments):
        command = [str(args.build / name), *map(str, arguments)]
        with (output / (name + '.log')).open('w') as stream:
            subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT, check=True)
        commands.append(command)

    course = output / 'rotation'
    run('compile_cold', [example / 'INPUT.plan', course, example / 'assignment.txt', 0, 0, 4001, 0.005,
        '--opponent', 'public_router', '--shop_seed', 5003, '--bank', example / 'BANK.bin', '--warm_seconds', 0])
    summary = json.loads((course / 'SUMMARY.json').read_text())
    for key in ['complete', 'days', 'hire_bill', 'cash', 'opponent_cash', 'queries']:
        if summary[key] != expected[key]:
            raise ValueError('example result changed: ' + key)
    actions = hashlib.sha256(b''.join((course / f'{day:02d}/executable.actions.txt').read_bytes() for day in range(30))).hexdigest()
    if actions != expected['actions_sha256']:
        raise ValueError('example executable actions changed')
    run('export_calendar', [course, 4001, 0.005, 0, course / 'calendar'])
    calendar = json.loads((course / 'calendar/CALENDAR.json').read_text())
    if not calendar['schedule_certified'] or calendar['verified_transitions'] != 719 or calendar['cash'] != expected['cash']:
        raise ValueError('example calendar failed')
    run('check_ordered_stock', [])
    run('check_repeated_service', [output / 'repeated_services'])
    run('check_preparation_repair', [output / 'preparation.json'])
    manifest = output / 'plans.txt'
    manifest.write_text(str(example / 'INPUT.plan') + '\n')
    run('check_life_compile_reuse', [manifest, output / 'oracle.json'])
    run('check_life_day_cache', [manifest, output / 'cache.json'])
    optimizer = {}
    for seconds in [0, 5]:
        directory = output / f'optimize_{seconds}'
        command = [sys.executable, str(PACKAGE / 'tools/optimize_placement.py'), str(course), str(directory),
            '--tools', str(args.build), '--seconds', str(seconds), '--mode', 'search', '--objective', 'cash']
        with (output / f'optimizer_{seconds}.log').open('w') as stream:
            subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT, check=True)
        commands.append(command)
        result = json.loads((directory / 'RESULT.json').read_text())
        if result['best'] is None or result['cash'] < expected['cash'] or (seconds and not result['input_reverified']):
            raise ValueError('bounded optimizer lost its supplied incumbent')
        optimizer[str(seconds)] = {key: result[key] for key in ['best', 'cash', 'hire_bill', 'input_reverified', 'elapsed_seconds', 'termination_reason']}
    report = {'passed': True, 'checked_at_utc': datetime.now(timezone.utc).isoformat(),
        'scope': 'relocation check: byte-identical example actions/outcomes, independent 719-transition financial replay, existing stock/service/preparation/cache checks and zero/five-second bounded calls; no experiment data used',
        'example': {key: summary[key] for key in ['complete', 'days', 'hire_bill', 'cash', 'opponent_cash', 'queries']},
        'actions_sha256': actions, 'calendar': {key: calendar[key] for key in ['schedule_certified', 'verified_transitions', 'cash', 'events']},
        'oracle_reuse': json.loads((output / 'oracle.json').read_text()), 'day_cache': json.loads((output / 'cache.json').read_text()),
        'preparation': json.loads((output / 'preparation.json').read_text()), 'optimizer': optimizer, 'commands': commands}
    (output / 'VALIDATION.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ['passed', 'scope', 'example', 'optimizer']}, indent=2))


if __name__ == '__main__':
    main()
