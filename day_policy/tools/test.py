"""Rebuild replay cases and benchmark the policy; write generated files outside the package."""
import argparse
from collections import Counter
import csv
import gzip
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess
import zipfile

PACKAGE = Path(__file__).resolve().parents[1]


def read(path):
    with path.open() as stream:
        return list(csv.DictReader(stream))


def save(path, value):
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + '\n')


def same(first, second):
    return len(first) == len(second) and all(
        {k: v for k, v in a.items() if k != 'microseconds'} ==
        {k: v for k, v in b.items() if k != 'microseconds'}
        for a, b in zip(first, second))


def key(row):
    return tuple(row[k] for k in ['game', 'seat', 'day'])


def summary(rows, baseline):
    unique = {key(r): r for r in rows}
    expected = {key(r) for r in baseline}
    if not set(unique) <= expected:
        raise ValueError('Results contain days outside the original eligible cohort')
    good = [r for r in unique.values() if r['status'] == '0']
    times = [float(r['microseconds']) / 1000 for r in rows]
    return dict(days=len(expected), mapped=len(unique), solved=len(good),
                late_days=sum(20 <= int(r['day']) <= 28 for r in baseline),
                late_solved=sum(20 <= int(r['day']) <= 28 for r in good),
                median_ms=statistics.median(times) if times else 0,
                mean_ms=statistics.mean(times) if times else 0,
                mean_hires=statistics.mean(int(r['hires']) for r in good) if good else 0)


def invoke(command, log):
    print(log.stem, flush=True)
    with log.open('w') as stream:
        subprocess.run([str(s) for s in command], stdout=stream, stderr=subprocess.STDOUT, check=True)


def unpack_reference(work):
    output = work / 'reference'
    output.mkdir(exist_ok=True)
    with zipfile.ZipFile(PACKAGE / 'measurements/rows.zip') as archive:
        for name in archive.namelist():
            if Path(name).name != name or not name.endswith('.csv'):
                raise ValueError('Unexpected measurement archive entry')
            (output / name).write_bytes(archive.read(name))
    return output


def prepare(args, cohorts):
    traces = args.work / 'traces'
    traces.mkdir(exist_ok=True)
    trace_meta = json.loads((PACKAGE / 'data/TRACES.json').read_text())
    games = {str(r['game']) for label in args.labels for r in cohorts[label]['games']}
    for game in sorted(games):
        content = gzip.decompress((PACKAGE / f'data/replays/{game}.txt.gz').read_bytes())
        if len(content) != trace_meta[game]['bytes'] or hashlib.sha256(content).hexdigest() != trace_meta[game]['sha256']:
            raise ValueError(f'Trace checksum mismatch: {game}')
        (traces / f'{game}.txt').write_bytes(content)
    reference = unpack_reference(args.work)
    report = {}
    for label in args.labels:
        output = args.work / label
        output.mkdir(exist_ok=True)
        cohort = output / 'cohort.txt'
        cohort.write_text(''.join(f"{r['game']} {r['seat']} {r['rank']} {traces}/{r['game']}.txt\n"
                                  for r in cohorts[label]['games']))
        original = output / 'original.bin'
        invoke([args.build / 'contract_extract', cohort, original, output / 'exclusions.csv'], output / 'extract.log')
        rows = read(output / 'exclusions.csv')
        eligible = [r for r in rows if r['reason'] == 'eligible']
        if len(rows) != cohorts[label]['player_days'] or len(eligible) != cohorts[label]['eligible_days']:
            raise ValueError(f'Changed cohort/exclusions: {label}')
        expected = read(reference / f'placement_{label}_s1_release_original_r0_h13_balanced4_a.csv')
        if {key(r) for r in eligible} != {key(r) for r in expected}:
            raise ValueError(f'Eligible day identities changed: {label}')
        # Both pre-night and dawn tile attributes must match the original tape.
        control = output / 'control.bin'
        invoke([args.build / 'contract_progression', cohort, original, control,
                output / 'control.csv', 1, 0, 1], output / 'control.log')
        control.unlink()
        report[label] = {'eligible': len(eligible), 'exclusions': dict(Counter(r['reason'] for r in rows)),
                         'original_grid_control': 'passed', 'styles': {}}
        for style in [0, 1]:
            mapped = output / f'own_s{style}.bin'
            audit = output / f'audit_s{style}.csv'
            bounds = output / f'bounds_s{style}.csv'
            invoke([args.build / 'contract_progression', cohort, original, mapped, audit, 0, 0, style],
                   output / f'progression_s{style}.log')
            invoke([args.build / 'contract_return_bounds', mapped, bounds], output / f'bounds_s{style}.log')
            for path, suffix in [(audit, 'audit'), (bounds, 'bounds')]:
                if read(path) != read(reference / f'placement_{label}_s{style}_release_{suffix}.csv'):
                    raise ValueError(f'Changed {suffix}: {label}, style {style}')
            report[label]['styles'][str(style)] = dict(Counter(r['progression_reason'] for r in read(audit)
                                                              if r['original_reason'] == 'eligible'))
    save(args.work / 'PREPARE.json', report)


def configurations(suite, labels):
    if suite == 'caps-missing':
        for label in labels:
            if label == 'dev':
                for profile in ['balanced4', 'full8']:
                    yield label, 1, 'original', 0, 10, profile
            for mode in [0, 1]:
                yield label, 1, 'own', mode, 10, 'balanced4'
        return
    if suite == 'quick':
        for label in labels:
            for layout, profile, mode, cap in [('original', 'balanced4', 0, 11), ('original', 'full8', 0, 13),
                                             ('own', 'balanced4', 0, 11), ('own', 'balanced4', 0, 13),
                                             ('own', 'balanced4', 1, 13)]:
                yield label, 1, layout, mode, cap, profile
        return
    for label in labels:
        for layout, profiles, modes in [('original', ['balanced4', 'full8'], [0]), ('own', ['balanced4'], [0, 1])]:
            for profile in profiles:
                for mode in modes:
                    caps = [10, 11, 13] if suite == 'caps' or (label != 'dev' and layout == 'original') else [11, 13]
                    for cap in caps:
                        yield label, 1, layout, mode, cap, profile
        if suite == 'all':
            for mode in [0, 1]:
                for cap in [11, 13]:
                    yield label, 0, 'own', mode, cap, 'balanced4'


def benchmark(args, cohorts):
    output = args.work / args.run
    output.mkdir()  # Preserve earlier measurements.
    reference = unpack_reference(args.work)
    results = {}
    for label, style, layout, mode, cap, profile in configurations(args.suite, args.labels):
        name = f'placement_{label}_s{style}_release_{layout}_r{mode}_h{cap}_{profile}'
        case = args.work / label / ('original.bin' if layout == 'original' else f'own_s{style}.bin')
        if not case.is_file():
            raise ValueError(f'Run prepare first: {case}')
        runs = []
        for repeat in range(args.repeats):
            path = output / f'{name}_{chr(97 + repeat)}.csv'
            effort, variants = (3, 4) if profile == 'balanced4' else (0, 8)
            command = [args.build / 'contract_evaluate', case, path, 1000000, cap, effort, mode, 0, variants, 1, 0, style]
            if args.cpu is not None:
                command = ['taskset', '-c', args.cpu, *command]
            invoke(command, path.with_suffix('.log'))
            rows = read(path)
            if runs and not same(rows, runs[0]):
                raise ValueError(f'Non-deterministic result: {name}')
            if args.check_reference and not same(rows, read(reference / f'{name}_a.csv')):
                raise ValueError(f'Reference mismatch: {name}')
            runs.append(rows)
        baseline = [r for r in read(args.work / label / 'exclusions.csv') if r['reason'] == 'eligible']
        rows = sum(runs, [])
        value = summary(rows, baseline)
        value['players'] = {str(rank): {'team': next(r['team'] for r in cohorts[label]['games'] if r['rank'] == rank),
                                        **summary([r for r in rows if int(r['rank']) == rank],
                                                  [r for r in baseline if int(r['rank']) == rank])}
                            for rank in sorted({r['rank'] for r in cohorts[label]['games']})}
        if layout in ['original', 'own']:
            value['original_hires_at_most_cap'] = summary([r for r in rows if int(r['original_hires']) <= cap],
                                                        [r for r in baseline if int(r['original_hires']) <= cap])
        results[name] = value
        save(output / 'SUMMARY.json', results)
        print(f"  {value['solved']}/{value['days']}; {value['median_ms']:.2f}/{value['mean_ms']:.2f} ms", flush=True)
    save(output / 'RUN.json', {'suite': args.suite, 'repeats': args.repeats, 'cpu': args.cpu,
                              'reference_match': args.check_reference,
                              'executable_sha256': hashlib.sha256((args.build / 'contract_evaluate').read_bytes()).hexdigest(),
                              'configurations': len(results)})


def smoke(args):
    output = args.work / args.run
    output.mkdir()
    invoke(['ctest', '--test-dir', args.build, '--output-on-failure'], output / 'regressions.log')
    for opponent in ['pass', 'day_policy_contract']:
        expected = None
        for runner in ['contract_arena', 'contract_pair']:
            for repeat in range(2):
                path = output / f'{runner}_{opponent}_{repeat}.json'
                invoke([args.build / runner, '--b', opponent, '--games', 4, '--seed-start', 2400,
                        '--seat-mode', 'both', '--threads', 2, '--budget-expansions', 256,
                        '--validate', '--output', path], path.with_suffix('.log'))
                rows = json.loads(path.read_text())
                if len(rows) != 8 or any(r['turns'] != 719 or r['faults'] != 0 for r in rows):
                    raise ValueError(f'Arena faults: {path}')
                if expected is not None and rows != expected:
                    raise ValueError(f'Arena parity mismatch: {path}')
                expected = rows
    save(output / 'VALIDATION.json', {'regressions': 'passed', 'arena_runs': 8,
                                    'games_per_run': 8, 'turns_per_game': 719, 'faults': 0,
                                    'generic_pair_repeat_parity': 'passed'})


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase', choices=['prepare', 'benchmark', 'smoke', 'unpack-reference'])
    parser.add_argument('--work', type=Path, required=True, help='Generated files directory outside day_policy')
    parser.add_argument('--build', type=Path, help='CMake build directory')
    parser.add_argument('--labels', nargs='+', choices=['dev', '639', '645'], default=['dev', '639', '645'])
    parser.add_argument('--suite', choices=['quick', 'final', 'all', 'caps', 'caps-missing'], default='quick')
    parser.add_argument('--run', default='run')
    parser.add_argument('--repeats', type=int, choices=range(1, 27), default=2)
    parser.add_argument('--cpu', type=int)
    parser.add_argument('--check-reference', action='store_true')
    args = parser.parse_args()
    args.work = args.work.resolve()
    if args.work == PACKAGE or PACKAGE in args.work.parents or any(c.isspace() for c in str(args.work)):
        parser.error('--work must be outside day_policy and contain no whitespace (native cohort format)')
    if Path(args.run).name != args.run or args.run in ['.', '..']:
        parser.error('--run must be a single directory name')
    if args.phase != 'unpack-reference':
        if args.build is None or not (args.build / 'contract_evaluate').is_file():
            parser.error('--build must contain compiled policy tools')
        args.build = args.build.resolve()
    if args.cpu is not None and args.cpu not in os.sched_getaffinity(0):
        parser.error('--cpu is not available to this process')
    args.work.mkdir(parents=True, exist_ok=True)
    cohorts = json.loads((PACKAGE / 'data/COHORTS.json').read_text())
    if args.phase == 'prepare':
        prepare(args, cohorts)
    elif args.phase == 'benchmark':
        benchmark(args, cohorts)
    elif args.phase == 'smoke':
        smoke(args)
    else:
        unpack_reference(args.work)
