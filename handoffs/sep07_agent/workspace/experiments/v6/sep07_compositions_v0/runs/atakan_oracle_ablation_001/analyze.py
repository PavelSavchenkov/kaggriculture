"""Measure oracle ranking changes against saved exact C++ outcomes."""
import csv
import hashlib
import itertools
import json
import math
import statistics
from collections import Counter
from pathlib import Path

RUN = Path(__file__).resolve().parent
LABELS = {0: 'baseline', 1: 'rival_flows', 2: 'own_flows', 3: 'both_flows',
          4: 'future_shops', 5: 'rival_flows_and_shops', 6: 'own_flows_and_shops', 7: 'all_three',
          15: 'all_three_and_rival_fixed_cost', 31: 'all_three_fixed_and_exact_consumption_calendar'}
PRODUCTS = ['WHEAT', 'CARROT', 'TOMATO', 'STRAWBERRY', 'MELON', 'EGG', 'MILK', 'WOOL', 'FERTILIZER']


def mean(values):
    return statistics.mean(values)


def summarize(records):
    return {'contexts': len(records), 'pairwise_correct': sum(r['pairwise_correct'] for r in records),
            'pairwise_total': 3*len(records), 'pairwise_accuracy': mean(r['pairwise_correct']/3 for r in records),
            'hindsight_best_choices': sum(r['regret'] == 0 for r in records),
            'mean_regret': mean(r['regret'] for r in records),
            'chosen_actual_wins': sum(r['selected_actual_margin'] > 0 for r in records),
            'chosen_actual_mean_margin': mean(r['selected_actual_margin'] for r in records),
            'pair_delta_mae': mean(v for r in records for v in r['pair_delta_absolute_errors']),
            'mean_margin_gain_vs_baseline': mean(r['gain_vs_baseline'] for r in records),
            'better_than_baseline': sum(r['gain_vs_baseline'] > 0 for r in records),
            'worse_than_baseline': sum(r['gain_vs_baseline'] < 0 for r in records),
            'branch_counts': dict(Counter(r['branch'] for r in records))}


def main():
    expected = json.loads((RUN / 'inputs/expected.json').read_text())
    forecasts = json.loads((RUN / 'forecasts.json').read_text())
    assert len(expected) == len(forecasts) == 320
    records, seed_case, residuals = [], {}, []
    for truth, forecast in zip(expected, forecasts):
        for key in ['rival', 'seed', 'seat', 'cash_before226', 'rival_cash_before226', 'physical_before226']:
            assert truth[key] == forecast[key], (key, truth['rival'], truth['seed'], truth['seat'])
        variants = {v['mask']: v['branches'] for v in forecast['variants']}
        for branch in range(3):
            for key in ['own', 'rival']:
                assert variants[0][branch][key] == truth['baseline_estimates'][branch][key]
        actual = truth['actual_margins']
        baseline_branch = max(range(3), key=lambda b: variants[0][b]['own']-variants[0][b]['rival'])
        for mask, branches in variants.items():
            predicted = [b['own']-b['rival'] for b in branches]
            branch = max(range(3), key=lambda b: predicted[b])
            pair_errors, correct = [], 0
            for a, b in itertools.combinations(range(3), 2):
                pred_delta, actual_delta = predicted[a]-predicted[b], actual[a]-actual[b]
                correct += pred_delta*actual_delta > 0 or pred_delta == actual_delta == 0
                pair_errors.append(abs(pred_delta-actual_delta))
                for p, product in enumerate(PRODUCTS):
                    pred_rev = branches[a]['rival_revenue'][p]-branches[b]['rival_revenue'][p]
                    exact_rev = truth['exact_cash'][a][1]['revenue'][p]-truth['exact_cash'][b][1]['revenue'][p]
                    residuals.append({'mask': mask, 'product': product, 'revenue_delta_error': pred_rev-exact_rev})
            records.append({'rival': truth['rival'], 'seed': truth['seed'], 'seat': truth['seat'], 'mask': mask,
                            'branch': branch, 'regret': max(actual)-actual[branch], 'pairwise_correct': correct,
                            'selected_actual_margin': actual[branch], 'gain_vs_baseline': actual[branch]-actual[baseline_branch],
                            'pair_delta_absolute_errors': pair_errors, 'predicted_margin': predicted,
                            'actual_margins': actual})
        if (truth['rival'], truth['seed'], truth['seat']) == ('public_router_v5', 1028, 0):
            seed_case = {'context': ['public_router_v5', 1028, 0], 'delta': 'goose minus cow',
                         'actual_own_delta': truth['actual_own_profit'][2]-truth['actual_own_profit'][0],
                         'actual_rival_delta': truth['actual_rival_profit'][2]-truth['actual_rival_profit'][0],
                         'actual_margin_delta': actual[2]-actual[0],
                         'actual_rival_milk_revenue_delta': truth['exact_cash'][2][1]['revenue'][6]-truth['exact_cash'][0][1]['revenue'][6],
                         'shops': truth['shops'], 'variants': []}
            for mask, branches in variants.items():
                cow, goose = branches[0], branches[2]
                seed_case['variants'].append({'mask': mask, 'name': LABELS[mask],
                    'predicted_own_delta': goose['own']-cow['own'], 'predicted_rival_delta': goose['rival']-cow['rival'],
                    'predicted_margin_delta': (goose['own']-cow['own'])-(goose['rival']-cow['rival']),
                    'predicted_rival_milk_revenue_delta': goose['rival_revenue'][6]-cow['rival_revenue'][6],
                    'chosen_branch': max(range(3), key=lambda b: branches[b]['own']-branches[b]['rival']),
                    'rival_revenue_delta_by_product': [goose['rival_revenue'][p]-cow['rival_revenue'][p] for p in range(9)],
                    'cow_daily_milk_quotes': [day[6] for day in cow['daily_quotes']],
                    'goose_daily_milk_quotes': [day[6] for day in goose['daily_quotes']]})
    metrics = {mask: {'name': label, **summarize([r for r in records if r['mask'] == mask])} for mask, label in LABELS.items()}
    by_rival = {rival: {mask: summarize([r for r in records if r['mask'] == mask and r['rival'] == rival])
                        for mask in LABELS} for rival in sorted({r['rival'] for r in records})}
    shapley = {}
    for bit, name in [(1, 'rival_flows'), (2, 'own_flows'), (4, 'future_shops')]:
        regret_reduction, accuracy_gain = 0., 0.
        for mask in range(8):
            if mask&bit:
                continue
            n = mask.bit_count()
            weight = math.factorial(n)*math.factorial(2-n)/math.factorial(3)
            regret_reduction += weight*(metrics[mask]['mean_regret']-metrics[mask|bit]['mean_regret'])
            accuracy_gain += weight*(metrics[mask|bit]['pairwise_accuracy']-metrics[mask]['pairwise_accuracy'])
        shapley[name] = {'mean_regret_reduction': regret_reduction, 'pairwise_accuracy_gain': accuracy_gain}
    product_errors = {mask: {product: mean(abs(r['revenue_delta_error']) for r in residuals if r['mask'] == mask and r['product'] == product)
                              for product in PRODUCTS} for mask in LABELS}
    report = {'scope': 'Offline oracle ablation, 320 existing discovery contexts × three fixed branches. These are diagnostics, not deployable agent scores.',
              'baseline_source_exact_comparisons': 960, 'prefix_state_parity': 320, 'metrics': metrics,
              'by_opponent': by_rival, 'shapley_all_three_oracle_improvements': shapley,
              'rival_revenue_pair_delta_mae_by_product': product_errors,
              'limits': ['Rival flows oracle includes future product purchase costs. Baseline predicts animal sales only and no purchases.',
                         'All masks retain original daily midpoint pricing, internal netting and price-floor approximation.',
                         'Rival fixed costs are omitted in masks0..7; masks15 and31 add their realized values to isolate this omitted term.',
                         'Mask31 additionally fixes the exact consumption calendar, including no town-center fertilizer demand and the partial first day.',
                         'Future shop identities are unavailable at the original decision. Their oracle gain is not directly achievable by an observation-only predictor.',
                         'Exact quantities are conditioned on each branch and rival continuation, so oracle tests cannot be deployed as policies.',
                         'The same discovery contexts support all numbers; no holdout or statistical generalization claim is made.']}
    for name, value in [('REPORT', report), ('cases', records), ('seed1028_v5', seed_case)]:
        (RUN / f'{name}.json').write_text(json.dumps(value, indent=2)+'\n')
    with (RUN / 'metrics.csv').open('w') as out:
        rows = [{k:v for k,v in row.items() if k != 'branch_counts'} for row in metrics.values()]
        writer = csv.DictWriter(out, fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    with (RUN / 'seed1028_v5.csv').open('w') as out:
        rows = [{k:v for k,v in row.items() if not isinstance(v, list)} for row in seed_case['variants']]
        writer = csv.DictWriter(out, fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    print(json.dumps({'metrics': metrics, 'shapley': shapley, 'seed1028': [{k:v for k,v in x.items() if not isinstance(v, list)} for x in seed_case['variants']]}, indent=2))


if __name__ == '__main__':
    main()
