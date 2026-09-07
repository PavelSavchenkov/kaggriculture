"""Run C++ observation-only forecasts, then compare saved fixed-course outcomes."""
import csv
import itertools
import json
import statistics
import subprocess
from collections import Counter
from pathlib import Path

from prepare import COUNTS

RUN = Path(__file__).resolve().parent
SOURCE = RUN.parent / 'atakan_portfolio_001'
RIVALS = ['animal_adaptive_r1_c0_b0', 'teammate_shoprouter', 'public_router_v5', 'king_rc4', 'public_router']
REFERENCE = 'investment_context_guarded_001_best'


def mean(values):
    return statistics.mean(values)


def main():
    results = RUN / 'results';results.mkdir(exist_ok=True)
    commands, records, timing = [], [], []
    for rival in RIVALS + [REFERENCE]:
        output = results / f'trace_{rival}.json'
        command = ['conda', 'run', '-n', 'kaggriculture', str(RUN / 'build/generic/diagnostics'), '--b', rival,
                   '--seed-start', '1000', '--games', '32', '--seat-mode', 'both', '--threads', '1', '--output', str(output)]
        subprocess.run(command, check=True);commands.append(command)
        forecasts = json.loads(output.read_text())
        panels = [json.loads((SOURCE / f'results/atakan_{branch}_vs_{rival}.json').read_text())['games'] for branch in ['cow', 'sheep', 'goose']]
        for index, forecast in enumerate(forecasts):
            games = [panel[index] for panel in panels]
            assert all((g['seed'], g['seat']) == (forecast['seed'], forecast['seat']) for g in games)
            margins = [g['cash']-g['opponent_cash'] for g in games]
            for variant in forecast['variants']:
                count, branches = variant['count'], variant['branches']
                timing.append({'rival': rival, 'seed': forecast['seed'], 'seat': forecast['seat'], 'count': count, 'us': variant['microseconds_per_decision']})
                for objective in ['own', 'margin']:
                    score = [v['own']-(v['rival'] if objective == 'margin' else 0) for v in branches]
                    choice = max(range(3), key=lambda branch: score[branch])
                    pair_correct = 0
                    for a, b in itertools.combinations(range(3), 2):
                        prediction, actual = score[a]-score[b], margins[a]-margins[b]
                        pair_correct += prediction*actual > 0 or prediction == actual == 0
                    records.append({'rival': rival, 'seed': forecast['seed'], 'seat': forecast['seat'], 'count': count, 'objective': objective,
                                    'name': f'atakan_integrated_s{count}_{objective}', 'branch': choice,
                                    'cash': games[choice]['cash'], 'opponent_cash': games[choice]['opponent_cash'],
                                    'margin': margins[choice], 'regret': max(margins)-margins[choice], 'correct_pairs': pair_correct,
                                    'scores': score})
    measures = []
    for scope, rivals in [('discovery', RIVALS), ('reference', [REFERENCE])]:
        for count in COUNTS:
            for objective in ['own', 'margin']:
                subset = [r for r in records if r['count'] == count and r['objective'] == objective and r['rival'] in rivals]
                measures.append({'scope': scope, 'name': f'atakan_integrated_s{count}_{objective}', 'games': len(subset),
                                 'wins': sum(r['margin'] > 0 for r in subset), 'mean_margin': mean(r['margin'] for r in subset),
                                 'mean_regret': mean(r['regret'] for r in subset), 'best_branch_choices': sum(r['regret'] == 0 for r in subset),
                                 'pairwise_correct': sum(r['correct_pairs'] for r in subset), 'pairwise_total': 3*len(subset),
                                 'branch_counts': dict(Counter(r['branch'] for r in subset))})
    timing_summary = {count: {'mean_us': mean(r['us'] for r in timing if r['count'] == count),
                              'median_us': statistics.median(r['us'] for r in timing if r['count'] == count),
                              'p95_us': sorted(r['us'] for r in timing if r['count'] == count)[int(.95*len(timing)/len(COUNTS))],
                              'max_us': max(r['us'] for r in timing if r['count'] == count)} for count in COUNTS}
    report = {'scope': 'Observation-only forecast choices evaluated against saved exact fixed-branch outcomes; full new-policy realization is separate.',
              'metrics': measures, 'timing': timing_summary, 'baseline_source_forecast_comparisons': 384*3,
              'timing_scope': 'Eight repeated complete three-branch evaluations per context, one C++ thread, all sample generation included. Concurrent system work may affect wall times.'}
    for name, data in [('DIAGNOSTIC_REPORT', report), ('decision_cases', records), ('timing_cases', timing), ('DIAGNOSTIC_COMMANDS', commands)]:
        (RUN / f'{name}.json').write_text(json.dumps(data, indent=2)+'\n')
    with (RUN / 'diagnostic_metrics.csv').open('w') as out:
        rows = [{k:v for k,v in row.items() if k != 'branch_counts'} for row in measures]
        writer = csv.DictWriter(out, fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
