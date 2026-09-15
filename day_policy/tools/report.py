from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
import statistics

import argparse
from pathlib import Path
from test import PACKAGE, read, key, summary as summarize, same

parser = argparse.ArgumentParser(description='Rebuild the saved total/per-player measurement report from CSV evidence')
parser.add_argument('--results', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
RESULTS = args.results.resolve()
OUTPUT = args.output.resolve()
if OUTPUT == PACKAGE or PACKAGE in OUTPUT.parents:
    parser.error('Write generated reports outside day_policy')
OUTPUT.mkdir(parents=True, exist_ok=True)


def result(label, style, layout, mode, cap, profile, repeat='a'):
    return RESULTS / f'placement_{label}_s{style}_release_{layout}_r{mode}_h{cap}_{profile}_{repeat}.csv'


def measurements(first, second, baseline):
    assert same(first, second)
    value = summarize(first, baseline)
    times = sorted(float(r['microseconds']) / 1000 for r in first + second)
    value.update(median_ms=statistics.median(times), mean_ms=statistics.mean(times),
                 p95_ms=times[int(.95 * (len(times)-1))], max_ms=max(times))
    return value


if __name__ == '__main__':
    names = {str(r['rank']): {'team': r['team']} for r in json.loads((PACKAGE / 'data/COHORTS.json').read_text())['645']['games']}
    report = {'utc': datetime.now(timezone.utc).isoformat(), 'totals': {}, 'players': {},
              'original_hire_filtered': {}, 'bounds': {}, 'mapping': {}, 'placement_cost': {}, 'sha256': {}}
    datasets = {}
    baselines = {}
    for label in ['dev', '639', '645']:
        baseline = read(result(label, 1, 'original', 0, 13, 'balanced4'))
        baselines[label] = baseline
        report['totals'][label] = {}
        for layout, profiles, modes in [('original', ['balanced4', 'full8'], [0]), ('own', ['balanced4'], [0, 1])]:
            for profile in profiles:
                for mode in modes:
                    for cap in [11, 13]:
                        paths = [result(label, 1, layout, mode, cap, profile, r) for r in ['a', 'b']]
                        runs = [read(p) for p in paths]
                        for p in paths:
                            report['sha256'][p.name] = hashlib.sha256(p.read_bytes()).hexdigest()
                        tag = f'{layout}_{profile}_r{mode}_h{cap}'
                        datasets[label, tag] = runs
                        report['totals'][label][tag] = measurements(*runs, baseline)
        for cap in [11, 13]:
            for mode in [0, 1]:
                rows = read(result(label, 0, 'own', mode, cap, 'balanced4'))
                report['totals'][label][f'legacy_own_r{mode}_h{cap}'] = summarize(rows, baseline)
                current = datasets[label, f'own_balanced4_r{mode}_h{cap}'][0]
                old = {key(r): r for r in rows if r['status'] == '0'}
                pairs = [(old[key(r)], r) for r in current if r['status'] == '0' and key(r) in old]
                report['totals'][label][f'paired_hires_r{mode}_h{cap}'] = dict(
                    days=len(pairs), legacy_total=sum(int(a['hires']) for a, b in pairs),
                    staged_total=sum(int(b['hires']) for a, b in pairs))
        for style in [0, 1]:
            rows = [r for r in read(RESULTS / f'placement_{label}_s{style}_release_bounds.csv') if int(r['product']) > 0]
            report['bounds'][f'{label}_s{style}'] = dict(days=len(rows), products=dict(Counter(r['product'] for r in rows)))
            solved = {key(r) for r in read(result(label, style, 'own', 0, 13, 'balanced4')) if r['status'] == '0'}
            assert not solved.intersection(key(r) for r in rows), 'return bound contradicted a verified success'
        audit = read(RESULTS / f'placement_{label}_s1_release_audit.csv')
        report['mapping'][label] = dict(Counter(r['progression_reason'] for r in audit if r['original_reason'] == 'eligible'))
        rows = read(RESULTS / f'placement_cost_{label}.csv')
        for tag, part in [('all', rows), ('with_new_products', [r for r in rows if int(r['new_products'])])]:
            times = [float(r['microseconds']) for r in part]
            report['placement_cost'][f'{label}_{tag}'] = dict(cases=len(part), median_us=statistics.median(times), mean_us=statistics.mean(times))
    for tag in report['totals']['639']:
        if not tag.startswith(('original_', 'own_')):
            continue
        runs = [datasets['639', tag][i] + datasets['645', tag][i] for i in [0, 1]]
        for rank, name in names.items():
            report['players'].setdefault(rank, {'team': name['team'], 'profiles': {}})
            filtered = [[r for r in run if r['rank'] == rank] for run in runs]
            baseline = [r for r in baselines['639'] + baselines['645'] if r['rank'] == rank]
            report['players'][rank]['profiles'][tag] = measurements(*filtered, baseline)
    for cap in [10, 11]:
        for profile in ['balanced4', 'full8']:
            runs = [[r for label in ['639', '645'] for r in read(result(label, 1, 'original', 0, cap, profile, repeat))
                     if int(r['original_hires']) <= cap] for repeat in ['a', 'b']]
            tag = f'{profile}_h{cap}'
            value = measurements(*runs, runs[0])
            value['players'] = {}
            for rank, name in names.items():
                filtered = [[r for r in run if r['rank'] == rank] for run in runs]
                value['players'][rank] = measurements(*filtered, filtered[0])
            report['original_hire_filtered'][tag] = value
    (OUTPUT / 'SUMMARY.json').write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n')
    lines = ['# Final day policy measurements', '',
             'Default: Balanced-4 with Staged placement. Broader option: Full-8.',
             'Hires exclude the farmer; late means days 20–28. Native GCC 13.3, O3,',
             'native CPU tuning and LTO; this release uses no PGO. Each selected-profile',
             'time combines two sequential runs pinned to CPU14. Times include failed',
             'calls and hire minimization, and exclude the evaluator’s second verification.',
             'Other machine activity was not controlled. See docs/TESTING.md for',
             'cohort construction, exclusions and the geometry-test limitations.', '',
             '## Exact original dawn layouts; our placement for new products', '',
             '| Cohort | Pipeline | Cap | Solved | Late | Median / mean ms | Mean hires on success |',
             '|---|---|---:|---:|---:|---:|---:|']
    for label in ['dev', '639', '645']:
        for cap in [11, 13]:
            for profile in ['balanced4', 'full8']:
                s = report['totals'][label][f'original_{profile}_r0_h{cap}']
                lines.append(f"| {label} | {profile} | {cap} | {s['solved']}/{s['days']} | {s['late_solved']}/{s['late_days']} | {s['median_ms']:.2f} / {s['mean_ms']:.2f} | {s['mean_hires']:.2f} |")
    lines += ['', '## Our placement carried from game start: Balanced-4', '',
              'The full original eligible denominator is retained, including mapping failures.',
              'Timing covers calls actually made. Hour-23 results relax deadlines and do',
              'not establish early-return coverage. Legacy rows use one timing run.', '',
              '| Cohort | Returns | Cap | Legacy solved | Staged solved | Staged late | Median / mean ms |',
              '|---|---|---:|---:|---:|---:|---:|']
    for label in ['dev', '639', '645']:
        for mode in [0, 1]:
            for cap in [11, 13]:
                s = report['totals'][label][f'own_balanced4_r{mode}_h{cap}']
                old = report['totals'][label][f'legacy_own_r{mode}_h{cap}']
                lines.append(f"| {label} | {'Strict' if mode == 0 else 'Hour 23'} | {cap} | {old['solved']}/{old['days']} | {s['solved']}/{s['days']} | {s['late_solved']}/{s['late_days']} | {s['median_ms']:.2f} / {s['mean_ms']:.2f} |")
    lines += ['', '## Hires on days completed by both placement policies, cap 13', '',
              'Shared solved days only, with all returns due at hour 23. This avoids',
              'comparing hire averages on different sets of successful days.', '',
              '| Cohort | Shared days | Legacy mean hires | Staged mean hires | Total hires saved |',
              '|---|---:|---:|---:|---:|']
    for label in ['dev', '639', '645']:
        s = report['totals'][label]['paired_hires_r1_h13']
        lines.append(f"| {label} | {s['days']} | {s['legacy_total']/s['days']:.2f} | {s['staged_total']/s['days']:.2f} | {s['legacy_total']-s['staged_total']} |")
    for cap in [11, 13]:
        lines += ['', f'## Per player, cap {cap}: four comparison games per player', '',
                  '| Player | Days / late | Original B4 solved / late | Original Full solved / late | B4 median / mean ms | Full median / mean ms | Own strict B4 solved / late | Own hour-23 B4 solved / late |',
                  '|---|---:|---:|---:|---:|---:|---:|---:|']
        for rank, player in report['players'].items():
            b, f, own, loose = [player['profiles'][tag] for tag in [f'original_balanced4_r0_h{cap}', f'original_full8_r0_h{cap}', f'own_balanced4_r0_h{cap}', f'own_balanced4_r1_h{cap}']]
            lines.append(f"| {player['team'].replace('|', '/')} | {b['days']} / {b['late_days']} | {b['solved']} / {b['late_solved']} | {f['solved']} / {f['late_solved']} | {b['median_ms']:.2f} / {b['mean_ms']:.2f} | {f['median_ms']:.2f} / {f['mean_ms']:.2f} | {own['solved']} / {own['late_solved']} | {loose['solved']} / {loose['late_solved']} |")
    lines += ['', '## Original-hire-filtered comparisons', '',
              'Only days with successful original hires <= the tested cap. Cap 10 and',
              'cap 11 have different denominators. The JSON includes every player.', '',
              '| Cap | Pipeline | Solved | Late | Median / mean ms |',
              '|---:|---|---:|---:|---:|']
    for tag, s in report['original_hire_filtered'].items():
        lines.append(f"| {tag[-2:]} | {tag[:-4]} | {s['solved']}/{s['days']} | {s['late_solved']}/{s['late_days']} | {s['median_ms']:.2f} / {s['mean_ms']:.2f} |")
    lines += ['', '## Placement cost and remaining limits', '',
              'The isolated placement measurement includes copying the compiled plan and',
              'a result checksum, but excludes job compilation. It is an upper estimate',
              'of the placement function alone; whole-solver times above are primary.', '']
    for label in ['dev', '639', '645']:
        s = report['placement_cost'][f'{label}_with_new_products']
        lines.append(f"- {label}, days with new products: {s['median_us']:.2f} / {s['mean_us']:.2f} microseconds median / mean.")
        old, new = [report['bounds'][f'{label}_s{s}']['days'] for s in [0, 1]]
        lines.append(f'- {label}: impossible inherited non-wheat return bounds: {old} Legacy, {new} Staged.')
    lines += ['', 'The changed-grid test still preserves recorded prefix service outcomes;',
              'the 639 cohort has two missing clearing targets and two cases blocked by',
              'an excluded prefix with multiple land purchases. All stay in the denominator.',
              'This is not a fully executed economic rollout. No minimum-worker proof or',
              'full-agent win-rate claim follows from these results. Final integration',
              'must plan returns against its actual resulting placement and state.', '',
              'An additional diagnostic Full-8 call solved the sole remaining 645-cohort',
              'hour-23 case at 13 hires (Otter Vibe, episode 109059154, seat 1, day 15,',
              '144 requested field operations). This does not change the Balanced tables',
              'or establish the runtime of a chained pipeline. No policy was tuned on it.']
    (OUTPUT / 'REPORT.md').write_text('\n'.join(lines) + '\n')
    print('wrote final placement report and all-player JSON')
