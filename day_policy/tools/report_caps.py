"""Report only days whose original hire count is at most the tested cap."""
import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import statistics
import zipfile

from test import PACKAGE, key, same

PIPELINES = [('original', 'balanced4', 0), ('original', 'full8', 0),
             ('own', 'balanced4', 0), ('own', 'balanced4', 1)]


def stats(runs, baseline):
    if not same(*runs):
        raise ValueError('Repeated runs changed non-time fields')
    expected = {key(r) for r in baseline}
    if len(expected) != len(baseline) or not {key(r) for r in runs[0]} <= expected:
        raise ValueError('Invalid day identities or denominator')
    good = [r for r in runs[0] if r['status'] == '0']
    late = lambda rows: [r for r in rows if 20 <= int(r['day']) <= 28]
    times = sorted(float(r['microseconds']) / 1000 for run in runs for r in run)
    late_times = [float(r['microseconds']) / 1000 for run in runs for r in late(run)]
    mean = lambda rows: statistics.mean(rows) if rows else None
    median = lambda rows: statistics.median(rows) if rows else None
    return dict(days=len(baseline), mapped=len(runs[0]), solved=len(good),
                late_days=len(late(baseline)), late_solved=len(late(good)),
                median_ms=median(times), mean_ms=mean(times),
                p95_ms=times[int(.95 * (len(times) - 1))] if times else None,
                max_ms=max(times) if times else None,
                late_median_ms=median(late_times), late_mean_ms=mean(late_times),
                mean_hires=mean([int(r['hires']) for r in good]),
                original_mean_hires_on_success=mean([int(r['original_hires']) for r in good]),
                hires_saved_on_success=sum(int(r['original_hires']) - int(r['hires']) for r in good))


def tag(layout, profile, mode):
    return f'{layout}_{profile}_r{mode}'


def numbers(row):
    if row['median_ms'] is None:
        return '—'
    return f"{row['median_ms']:.2f} / {row['mean_ms']:.2f}"


def coverage(row):
    return f"{row['solved']}/{row['days']}; {row['late_solved']}/{row['late_days']}"


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--extra-results', type=Path, help='New measurements not yet in the evidence archive')
    args = parser.parse_args()
    output = args.output.resolve()
    if output == PACKAGE or PACKAGE in output.parents:
        parser.error('Write generated reports outside day_policy')
    output.mkdir(parents=True, exist_ok=True)
    cohorts = json.loads((PACKAGE / 'data/COHORTS.json').read_text())
    names = {r['rank']: r['team'] for r in cohorts['645']['games']}
    with zipfile.ZipFile(PACKAGE / 'measurements/rows.zip') as archive:
        evidence = {name: archive.read(name) for name in archive.namelist()}
    if args.extra_results:
        for path in args.extra_results.glob('*.csv'):
            if path.name in evidence:
                raise ValueError(f'Duplicate evidence: {path.name}')
            evidence[path.name] = path.read_bytes()
    sources = {}

    def load(label, layout, profile, mode, cap, repeat):
        name = f'placement_{label}_s1_release_{layout}_r{mode}_h{cap}_{profile}_{repeat}.csv'
        data = evidence[name]
        sources[name] = hashlib.sha256(data).hexdigest()
        return list(csv.DictReader(io.StringIO(data.decode())))

    baseline = {label: load(label, 'original', 'balanced4', 0, 13, 'a') for label in cohorts}
    totals, players, rows = {}, {}, []
    groups = {'dev': ['dev'], '639': ['639'], '645': ['645'], 'comparison': ['639', '645']}
    for group, labels in groups.items():
        totals[group], players[group] = {}, {}
        for cap in [10, 11, 13]:
            base = [r for label in labels for r in baseline[label] if int(r['original_hires']) <= cap]
            totals[group][cap], players[group][cap] = {}, {}
            for layout, profile, mode in PIPELINES:
                runs = [[r for label in labels for r in load(label, layout, profile, mode, cap, repeat)
                         if int(r['original_hires']) <= cap] for repeat in ['a', 'b']]
                name = tag(layout, profile, mode)
                total = stats(runs, base)
                totals[group][cap][name] = total
                rows.append(dict(cohort=group, pipeline=name, cap=cap, rank=0, team='ALL', **total))
                players[group][cap][name] = {}
                for rank, team in sorted(names.items()):
                    part = stats([[r for r in run if int(r['rank']) == rank] for run in runs],
                                 [r for r in base if int(r['rank']) == rank])
                    players[group][cap][name][rank] = part
                    rows.append(dict(cohort=group, pipeline=name, cap=cap, rank=rank, team=team, **part))

    with (output / 'CAPS.csv').open('w') as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]), lineterminator='\n')
        writer.writeheader()
        writer.writerows(rows)
    report = {'filter': 'original_hires <= tested cap, before coverage and timing aggregation',
              'farmer': 'excluded from original and solver hire counts', 'late': 'days 20–28',
              'timings': 'milliseconds; both repeats; all calls including failures; missing mappings have no timing',
              'totals': totals, 'source_csv_sha256': sources}
    (output / 'CAPS.json').write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n')
    lines = ['# Coverage and timings by original hire cap', '',
             'For cap N, retain only compatible days with successful original hires <= N,',
             'then test our solver at cap N. Hires exclude the farmer. The filter applies',
             'before calculating coverage, hires and timings. Caps therefore have different',
             'denominators. Late means days 20–28.', '',
             'Times combine two sequential CPU14-pinned native Release runs per configuration,',
             'including failed calls and hire minimization. Mapping failures remain in the',
             'coverage denominator but have no solve-time sample. These are uninstrumented',
             'solver timings, including its internal verifier and excluding the evaluator’s',
             'second verification. GCC 13.3, O3/native/LTO, Intel Core i7-14700K; no PGO.',
             'Additional cap-10 runs use the same executable as the saved cap-11/13 runs.',
             'Other machine activity was not controlled. Per-player means use the same filter.', '',
             'Original = exact replay dawn, our placement for new products. Own strict =',
             'our placements carried from game start, with original return deadlines.',
             'Own h23 = the separate diagnostic moving all returns to hour 23; it does not',
             'test early-return coverage. These geometry tests preserve recorded prefix',
             'service outcomes; see `../docs/TESTING.md` for limitations.', '',
             '`CAPS.csv` contains totals and all 30 original agents separately for each',
             'cohort and the combined comparison set, including late-day timings, p95/max',
             'times and hire savings on successful days. `CAPS.json` contains totals and',
             'input CSV hashes. The two comparison cohorts contain four games per agent.', '',
             '## Totals', '',
             '| Cohort | Cap | Pipeline | Solved | Late | Median / mean ms | Mean hires on success |',
             '|---|---:|---|---:|---:|---:|---:|']
    titles = {'original_balanced4_r0': 'Original B4', 'original_full8_r0': 'Original Full8',
              'own_balanced4_r0': 'Own strict B4', 'own_balanced4_r1': 'Own h23 B4'}
    for group in groups:
        for cap in [10, 11, 13]:
            for name, row in totals[group][cap].items():
                lines.append(f"| {group} | {cap} | {titles[name]} | {row['solved']}/{row['days']} | "
                             f"{row['late_solved']}/{row['late_days']} | {numbers(row)} | {row['mean_hires']:.2f} |")
    for cap in [10, 11, 13]:
        for heading, first, second in [('Original dawn', 'original_balanced4_r0', 'original_full8_r0'),
                                       ('Our placement', 'own_balanced4_r0', 'own_balanced4_r1')]:
            lines += ['', f'## {heading}, comparison games, original hires <= {cap}, solver cap {cap}', '',
                      f'| Original agent | {titles[first]} solved; late | Median / mean ms | {titles[second]} solved; late | Median / mean ms |',
                      '|---|---:|---:|---:|---:|']
            for rank, team in sorted(names.items()):
                a, b = [players['comparison'][cap][name][rank] for name in [first, second]]
                lines.append(f"| {team.replace('|', '/')} | {coverage(a)} | {numbers(a)} | {coverage(b)} | {numbers(b)} |")
    (output / 'CAPS.md').write_text('\n'.join(lines) + '\n')
    print('Wrote filtered cap tables, totals and per-original-agent CSV')
