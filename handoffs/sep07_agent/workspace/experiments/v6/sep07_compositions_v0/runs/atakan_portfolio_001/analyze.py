"""Analyze saved C++ outcomes and forecasts; no policy gameplay."""
import csv
import hashlib
import json
import statistics
from collections import Counter
from pathlib import Path

from screen import NAMES, RIVALS

RUN = Path(__file__).resolve().parent
FIXED = NAMES[:3]


def read(name):
    return json.loads((RUN / 'results' / f'{name}.json').read_text())


def mean(values):
    return statistics.mean(values) if values else None


def core(game):
    return {k: v for k, v in game.items() if k not in ('profile', 'opponent_profile')}


def normalize_inventory(vector):
    cursor = 3 + 2*vector[0] + 1300 + 17
    result = vector[:cursor]
    for _ in range(vector[0]):
        n = vector[cursor];cursor += 1
        pairs = sorted(vector[cursor+2*i:cursor+2*i+2] for i in range(n))
        result += [n, *(x for pair in pairs for x in pair)]
        cursor += 2*n
    assert cursor == len(vector)
    return result


def measure(data):
    games = data['games']
    return {'games': len(games), 'wins': sum(g['cash'] > g['opponent_cash'] for g in games),
            'ties': sum(g['cash'] == g['opponent_cash'] for g in games),
            'mean_margin': mean([g['cash'] - g['opponent_cash'] for g in games]),
            'mean_cash': mean([g['cash'] for g in games]), 'pass_J': data['pass_J'],
            'margin_cvar10': data['margin_cvar10'], 'mean_unit_faults': mean([g['unit_faults'] for g in games])}


def actual_flows(game):
    sales, buys = [[0]*9 for _ in range(30)], [[0]*9 for _ in range(30)]
    for step, item, bought, sold in game['profile']['flows']:
        if step >= 226:
            sales[step//24][item] += sold;buys[step//24][item] += bought
    fixed = sum(row[1] for row in game['profile']['fixed_costs'] if row[0] >= 226)
    return sales, buys, fixed


def main():
    source_states = [json.loads((RUN / f'tests/source_state_{i}.json').read_text()) for i in range(3)]
    expected = [normalize_inventory(x) for x in source_states]
    models = json.loads((RUN / 'flow_models.json').read_text())
    measured = {a: {} for a in NAMES}
    cases, quantity_diagnostics, compatibility = [], [], []
    exact_route_records = 0
    for rival in RIVALS:
        panels = {a: read(f'{a}_vs_{rival}') for a in NAMES}
        for a in NAMES:
            measured[a][rival] = measure(panels[a])
        traces = read(f'trace_{rival}')
        for a in FIXED[1:]:
            other = read(f'trace_{a}_vs_{rival}')
            assert len(other) == len(traces)
            for x, y in zip(traces, other):
                for key in ['seed', 'seat', 'cash_before226', 'rival_cash_before226', 'physical_before226', 'estimates']:
                    assert x[key] == y[key], (rival, a, key)
        for i, trace in enumerate(traces):
            fixed = [panels[a]['games'][i] for a in FIXED]
            estimates = trace['estimates']
            choices = {}
            for name in NAMES[3:]:
                game = panels[name]['games'][i]
                matches = [b for b in range(3) if core(game) == core(fixed[b])]
                assert len(matches) == 1, (name, rival, game['seed'], matches)
                choices[name] = matches[0];exact_route_records += 1
            actual_own = [g['cash'] - trace['cash_before226'] for g in fixed]
            actual_rival = [g['opponent_cash'] - trace['rival_cash_before226'] for g in fixed]
            actual_margin = [a-b for a, b in zip(actual_own, actual_rival)]
            predicted = [x['own']-x['rival'] for x in estimates]
            assert choices['atakan_value_own'] == max(range(3), key=lambda b: estimates[b]['own'])
            assert choices['atakan_value_margin'] == max(range(3), key=lambda b: predicted[b])
            record = {'rival': rival, 'seed': trace['seed'], 'seat': trace['seat'], 'choices': choices,
                      'predicted_own': [x['own'] for x in estimates], 'predicted_rival': [x['rival'] for x in estimates],
                      'predicted_margin': predicted, 'predicted_min_cash': [x['min_cash'] for x in estimates],
                      'actual_own_profit_after226': actual_own, 'actual_rival_profit_after226': actual_rival,
                      'actual_margin_change_after226': actual_margin,
                      'oracle_branch_for_margin': max(range(3), key=lambda b: actual_margin[b]),
                      'fixed_future_costs': [actual_flows(g)[2] for g in fixed]}
            cases.append(record)
            actual_state = trace['physical_before226'];normalized = normalize_inventory(actual_state)
            wheat_offset = 3 + 2*actual_state[0] + 1300
            ignore_wheat = lambda v: v[:wheat_offset] + v[wheat_offset+1:]
            compatibility.append({'rival': rival, 'seed': trace['seed'], 'seat': trace['seat'],
                                  'wheat': trace['shed_wheat_before226'],
                                  'ordered_equal': [actual_state == x for x in source_states],
                                  'equal_ignoring_inventory_key_order': [normalized == x for x in expected],
                                  'equal_except_wheat_and_inventory_key_order': [ignore_wheat(normalized) == ignore_wheat(x) for x in expected]})
            for branch, game in enumerate(fixed):
                sales, buys, cost = actual_flows(game);model = models[branch]
                detail = {'rival': rival, 'seed': trace['seed'], 'seat': trace['seat'], 'branch': branch,
                          'fixed_cost_error': cost - sum(model['fixed_costs']),
                          'sales_quantity_error': [sum(sales[d][p] - model['sales'][d][p] for d in range(30)) for p in range(9)],
                          'buys_quantity_error': [sum(buys[d][p] - model['buys'][d][p] for d in range(30)) for p in range(9)],
                          'dated_sales_l1_error': sum(abs(sales[d][p] - model['sales'][d][p]) for d in range(30) for p in range(9)),
                          'dated_buys_l1_error': sum(abs(buys[d][p] - model['buys'][d][p]) for d in range(30) for p in range(9)),
                          'actual_minus_estimated_own_profit': actual_own[branch] - estimates[branch]['own'],
                          'actual_minus_estimated_rival_profit': actual_rival[branch] - estimates[branch]['rival'],
                          'target_lives': [life for life in game['profile']['lives'] if life[0] >= 9 and (life[1], life[2]) in [(1,4),(3,2),(4,1)] and life[3] >= 226],
                          'full_game_produced': game['produced']}
                quantity_diagnostics.append(detail)
    aggregate = {}
    for name, values in measured.items():
        aggregate[name] = {'games': sum(x['games'] for x in values.values()), 'wins': sum(x['wins'] for x in values.values()),
                           'mean_margin': mean([x['mean_margin'] for x in values.values()]), 'opponents': values}
    quality = {}
    for name in NAMES[3:]:
        regrets = [max(r['actual_margin_change_after226']) - r['actual_margin_change_after226'][r['choices'][name]] for r in cases]
        quality[name] = {'selected_branch_counts': dict(Counter(r['choices'][name] for r in cases)),
                         'oracle_branch_matches': sum(r['choices'][name] == r['oracle_branch_for_margin'] for r in cases),
                         'mean_margin_regret_within_three_branches': mean(regrets)}
    predictions_correct = []
    for r in cases:
        for a in range(3):
            for b in range(a+1, 3):
                predicted = r['predicted_margin'][a] - r['predicted_margin'][b]
                actual = r['actual_margin_change_after226'][a] - r['actual_margin_change_after226'][b]
                predictions_correct.append(predicted*actual > 0 or predicted == actual == 0)
    disagreements = []
    lookup = {(r['rival'], r['seed'], r['seat'], r['branch']): r for r in quantity_diagnostics}
    for r in cases:
        d, m = r['choices']['atakan_demand'], r['choices']['atakan_value_margin']
        if d == m:
            continue
        own = r['actual_own_profit_after226'];rival = r['actual_rival_profit_after226'];pred = r['predicted_margin']
        a, b = lookup[r['rival'], r['seed'], r['seat'], m], lookup[r['rival'], r['seed'], r['seat'], d]
        disagreements.append({**r, 'model_branch': m, 'demand_branch': d,
                              'predicted_margin_advantage_of_model': pred[m] - pred[d],
                              'actual_own_advantage_of_model': own[m] - own[d],
                              'actual_rival_advantage_of_model': rival[m] - rival[d],
                              'actual_margin_advantage_of_model': (own[m]-rival[m]) - (own[d]-rival[d]),
                              'model_branch_diagnostics': a, 'demand_branch_diagnostics': b})
    disagreements.sort(key=lambda r: r['actual_margin_advantage_of_model'])
    checks = {}
    for name in NAMES:
        a, b = read(f'{name}_generic16'), read(f'{name}_masked16')
        assert a['games'] == b['games']
        checks[name] = {'generic_masked_thread_records_equal': 16, 'pass': measure(read(f'{name}_pass256')), 'self': measure(read(f'{name}_self8'))}
    assert read('typed_debug16')['games'] == read('typed_generic16')['games']
    ref = 'investment_context_guarded_001_best'
    reference = {name: measure(read(f'{name}_vs_{ref}')) for name in NAMES}
    compatibility_summary = {'games': len(compatibility), 'local_controls_before226_identical': len(compatibility),
                             'ordered_donor_matches': [sum(r['ordered_equal'][b] for r in compatibility) for b in range(3)],
                             'donor_matches_ignoring_key_order': [sum(r['equal_ignoring_inventory_key_order'][b] for r in compatibility) for b in range(3)],
                             'donor_matches_except_wheat_and_key_order': [sum(r['equal_except_wheat_and_inventory_key_order'][b] for r in compatibility) for b in range(3)],
                             'wheat_distribution': dict(Counter(r['wheat'] for r in compatibility)),
                             'caveat': 'Donor JSON inventory key order is not proof of original runtime insertion order; all exact ordered states remain retained'}
    quantity_summary = {}
    for branch in range(3):
        subset = [r for r in quantity_diagnostics if r['branch'] == branch]
        quantity_summary[branch] = {key: mean([r[key] for r in subset]) for key in ['fixed_cost_error', 'dated_sales_l1_error', 'dated_buys_l1_error', 'actual_minus_estimated_own_profit', 'actual_minus_estimated_rival_profit']}
        quantity_summary[branch]['all_three_target_lives_placed'] = sum(len(r['target_lives']) == 3 for r in subset)
        quantity_summary[branch]['sales_quantity_error'] = [mean([r['sales_quantity_error'][p] for r in subset]) for p in range(9)]
        quantity_summary[branch]['buys_quantity_error'] = [mean([r['buys_quantity_error'][p] for r in subset]) for p in range(9)]
    report = {'discovery_panel': 'Seeds1000..1031,both seats, five opponents, official rules with independent shop stream',
              'aggregate': aggregate, 'selector_quality': quality,
              'model_pairwise_margin_ranking_correct': sum(predictions_correct), 'model_pairwise_comparisons': len(predictions_correct),
              'routing_matches_complete_fixed_branch_records': exact_route_records,
              'compatibility': compatibility_summary, 'quantity_realization': quantity_summary,
              'model_vs_demand': {'different_decisions': len(disagreements), 'model_better': sum(x['actual_margin_advantage_of_model'] > 0 for x in disagreements),
                                  'model_worse': sum(x['actual_margin_advantage_of_model'] < 0 for x in disagreements),
                                  'mean_model_advantage_on_disagreements': mean([x['actual_margin_advantage_of_model'] for x in disagreements])},
              'operational': checks, 'typed_generic_debug_records_equal': 16, 'latest_reference': {ref: reference},
              'decision': 'Retain demand and value-margin as distinct league specialists; neither beats the current incumbent. No promotion, submission or catalog mutation.'}
    for name, value in [('REPORT', report), ('forecast_cases', cases), ('quantity_diagnostics', quantity_diagnostics),
                        ('state_compatibility', compatibility), ('model_vs_demand_disagreements', disagreements)]:
        (RUN / f'{name}.json').write_text(json.dumps(value, indent=2) + '\n')
    csv_rows = [{'name': name, 'opponent': opponent, **values} for name, opponents in measured.items() for opponent, values in opponents.items()]
    with (RUN / 'metrics.csv').open('w') as out:
        writer = csv.DictWriter(out, fieldnames=list(csv_rows[0]));writer.writeheader();writer.writerows(csv_rows)
    for name in NAMES:
        target = RUN / 'proposals' / name
        meta = json.loads((target / 'IMPORT.json').read_text())
        meta['status'] = 'Retain as league specialist' if name in ('atakan_demand', 'atakan_value_margin') else 'Archived experimental control; not selected for league registration'
        meta['validation'] = str((RUN / 'REPORT.json').relative_to(RUN.parents[1]))
        meta['policy_source_sha256'] = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((RUN / 'source').glob('*'))}
        (target / 'IMPORT.json').write_text(json.dumps(meta, indent=2) + '\n')
    print(json.dumps({'aggregate': {k: {x:v[x] for x in ['wins', 'games', 'mean_margin']} for k,v in aggregate.items()}, 'quality': quality, 'compatibility': compatibility_summary, 'quantities': quantity_summary, 'model_vs_demand': report['model_vs_demand'], 'reference': reference}, indent=2))


if __name__ == '__main__':
    main()
