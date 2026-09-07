"""Summarize completed C++ screens and validate operational equivalence."""
import json
import statistics
from pathlib import Path

import numpy as np

from diagnose import REFERENCE, RIVALS
from screen import RETAIN

RUN = Path(__file__).resolve().parent


def read(name):
    return json.loads((RUN / 'results' / f'{name}.json').read_text())


def measure(panel):
    games = panel['games']
    return {'games': len(games), 'wins': sum(g['cash'] > g['opponent_cash'] for g in games),
            'ties': sum(g['cash'] == g['opponent_cash'] for g in games),
            'mean_margin': statistics.mean(g['cash']-g['opponent_cash'] for g in games),
            'mean_cash': statistics.mean(g['cash'] for g in games), 'pass_J': panel['pass_J'],
            'mean_unit_faults': statistics.mean(g['unit_faults'] for g in games)}


def main():
    checks = {}
    for name in RETAIN:
        assert read(f'{name}_16')['games'] == read(f'{name}_masked16')['games']
        assert all(g['cash'] == g['opponent_cash'] for g in read(f'{name}_self8')['games'])
        checks[name] = {'generic_masked_thread_parity': 16, 'self_ties': 8, 'pass256': measure(read(f'{name}_pass256'))}
    assert read('typed_16')['games'] == read('typed_debug16')['games']
    rivals = RIVALS+[REFERENCE]
    fresh = {name: {rival: measure(read(f'fresh_{name}_vs_{rival}')) for rival in rivals}
             for name in RETAIN+['atakan_value_margin', 'atakan_demand']}
    aggregate = {name: {'games': sum(m['games'] for m in panels.values()), 'wins': sum(m['wins'] for m in panels.values()),
                        'mean_margin': statistics.mean(m['mean_margin'] for m in panels.values())} for name, panels in fresh.items()}
    gains = {}
    for name in RETAIN:
        gains[name] = {}
        for control in ['atakan_value_margin', 'atakan_demand']:
            paired, clusters = [], {}
            for rival in rivals:
                a, b = read(f'fresh_{name}_vs_{rival}')['games'], read(f'fresh_{control}_vs_{rival}')['games']
                for x, y in zip(a,b):
                    assert (x['seed'],x['seat']) == (y['seed'],y['seat'])
                    delta = (x['cash']-x['opponent_cash'])-(y['cash']-y['opponent_cash'])
                    paired.append(delta)
                    cluster = clusters.setdefault(x['seed'], [[], []])
                    cluster[0].append(delta)
                    cluster[1].append(int(x['cash']>x['opponent_cash'])-int(y['cash']>y['opponent_cash']))
            values = np.array([[statistics.mean(v[0]), statistics.mean(v[1])] for _, v in sorted(clusters.items())])
            rng = np.random.default_rng(20260907)
            bootstrap = values[rng.integers(len(values), size=(20000, len(values)))].mean(axis=1)
            gains[name][control] = {'mean_margin_gain': statistics.mean(paired), 'better_games': sum(x>0 for x in paired),
                                    'worse_games': sum(x<0 for x in paired), 'equal_games': sum(x==0 for x in paired),
                                    'wins_gain': aggregate[name]['wins']-aggregate[control]['wins'],
                                    'seed_cluster_bootstrap95_margin_gain': np.quantile(bootstrap[:,0], [.025,.975]).tolist(),
                                    'seed_cluster_bootstrap95_winrate_gain': np.quantile(bootstrap[:,1], [.025,.975]).tolist(),
                                    'bootstrap_scope': '20000 paired seed-cluster resamples; both seats and six opponents stay together. Descriptive percentile interval.'}
    direct = {name: {control: measure(read(f'fresh_{name}_vs_{control}')) for control in ['atakan_value_margin','atakan_demand']} for name in RETAIN}
    direct['mean_vs_sample64'] = measure(read(f'fresh_{RETAIN[0]}_vs_{RETAIN[1]}'))
    native = {}
    if (RUN / 'NATIVE_COMMANDS.json').exists():
        native = {name: {rival: measure(read(f'native_{name}_vs_{rival}')) for rival in [REFERENCE,'public_router_v5','teammate_shoprouter']}
                  for name in RETAIN+['atakan_value_margin','atakan_demand']}
    result = {'fresh_seeds': '1450000..1450255, both seats, six opponents, independent shop stream',
              'fresh': fresh, 'fresh_aggregate': aggregate, 'paired_gains': gains, 'direct_matchups': direct,
              'operational': checks, 'typed_generic_debug_parity': 16, 'native': native,
              'discovery_report': 'DIAGNOSTIC_REPORT.json', 'source_realization': 'SOURCE_REALIZATION_PARITY.json',
              'scope': 'Atakan replay portfolio specialists. Sampling changes unknown-shop integration only; donor flow and rival realization errors remain.'}
    (RUN / 'REPORT.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({'aggregate': aggregate, 'gains': gains, 'direct': direct, 'fresh':fresh, 'native':native}, indent=2))


if __name__ == '__main__':
    main()
