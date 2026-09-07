"""Exact product attribution from C++ output; private truth is diagnostic only."""
import csv
import json
import statistics
from collections import Counter
from pathlib import Path

from cash_audit import RIVALS

RUN = Path(__file__).resolve().parent
NAMES = ['atakan_cow', 'atakan_sheep', 'atakan_goose']
PRODUCTS = ['WHEAT', 'CARROT', 'TOMATO', 'STRAWBERRY', 'MELON', 'EGG', 'MILK', 'WOOL', 'FERTILIZER']


def read(name):
    return json.loads((RUN / name).read_text())


def mean(values):
    return statistics.mean(values)


def main():
    exact, original = {}, {}
    parity = 0
    for rival in RIVALS:
        for branch, name in enumerate(NAMES):
            audited = read(f'results/cash_{name}_vs_{rival}.json')
            games = read(f'results/{name}_vs_{rival}.json')['games']
            for audit, game in zip(audited, games):
                assert (audit['seed'], audit['seat']) == (game['seed'], game['seat'])
                key = rival, game['seed'], game['seat'], branch
                exact[key], original[key] = audit, game
                for player in range(2):
                    a = audit['players'][player]
                    assert a['cash'] == game['opponent_cash' if player else 'cash']
                    assert str(a['hash']) == str(game['opponent_action_hash' if player else 'action_hash'])
                    profile = game['opponent_profile' if player else 'profile']
                    sold, bought = [0]*9, [0]*9
                    for step, item, buys, sales in profile['flows']:
                        if step >= 226:
                            sold[item] += sales
                            bought[item] += buys
                    assert a['sold'] == sold and a['bought'] == bought
                    assert a['fixed_cost'] == sum(x[1] for x in profile['fixed_costs'] if x[0] >= 226)
                parity += 1
    disagreements = read('model_vs_demand_disagreements.json')
    attributed = []
    for case in disagreements:
        key = case['rival'], case['seed'], case['seat']
        m, d = case['model_branch'], case['demand_branch']
        am, ad = exact[*key, m], exact[*key, d]
        rm, rd = am['players'][1], ad['players'][1]
        fm, fd = am['forecasts'][m], ad['forecasts'][d]
        gm, gd = original[*key, m]['opponent_profile'], original[*key, d]['opponent_profile']
        product_rows = []
        for p, product in enumerate(PRODUCTS):
            predicted = fm['rival_revenue'][p] - fd['rival_revenue'][p]
            actual = rm['revenue'][p] - rd['revenue'][p]
            pm = rm['revenue'][p]/rm['sold'][p] if rm['sold'][p] else 0
            pd = rd['revenue'][p]/rd['sold'][p] if rd['sold'][p] else 0
            q_effect = (rm['sold'][p]-rd['sold'][p])*(pm+pd)/2
            price_effect = (pm-pd)*(rm['sold'][p]+rd['sold'][p])/2
            assert abs(actual-q_effect-price_effect) < 1e-7
            product_rows.append({'product': product, 'predicted_rival_revenue_delta': predicted,
                                 'exact_rival_revenue_delta': actual, 'missed_revenue_delta': actual-predicted,
                                 'exact_rival_spend_delta': rm['spend'][p]-rd['spend'][p],
                                 'quantity_term_symmetric': q_effect, 'average_price_term_symmetric': price_effect})
        own_error = case['predicted_own'][m]-case['predicted_own'][d]-case['actual_own_advantage_of_model']
        missed_rival = case['actual_rival_advantage_of_model']-(case['predicted_rival'][m]-case['predicted_rival'][d])
        ranking_error = case['predicted_margin_advantage_of_model']-case['actual_margin_advantage_of_model']
        assert own_error+missed_rival == ranking_error
        assert abs(sum(x['missed_revenue_delta']-x['exact_rival_spend_delta'] for x in product_rows)-(rm['fixed_cost']-rd['fixed_cost'])-missed_rival) < 1e-7
        attributed.append({'rival': case['rival'], 'seed': case['seed'], 'seat': case['seat'],
                           'model_branch': m, 'demand_branch': d, 'same_rival_action_hash': rm['hash']==rd['hash'],
                           'same_rival_dated_trade_flows': gm['flows']==gd['flows'],
                           'same_rival_lives': gm['lives']==gd['lives'], 'same_rival_fixed_cost': rm['fixed_cost']==rd['fixed_cost'],
                           'own_delta_overestimate': own_error, 'missed_rival_profit_delta': missed_rival,
                           'margin_delta_overestimate': ranking_error, 'products': product_rows})
    seed_key = 'public_router_v5', 1028, 0
    cow, goose = exact[*seed_key, 0], exact[*seed_key, 2]
    case = next(x for x in attributed if (x['rival'], x['seed'], x['seat']) == seed_key)
    current = cow['players'][1]
    rows = []
    for p, product in enumerate(PRODUCTS):
        row = dict(case['products'][p])
        row.update({'predicted_sale_quantity': cow['forecasts'][0]['rival_quantity'][p],
                    'realized_sale_quantity': current['sold'][p], 'realized_future_harvest_quantity': current['produced_after226'][p],
                    'initial_public_held': current['public_held_at226'][p],
                    'initial_private_shed_OFFLINE_ONLY': current['private_shed_at226_OFFLINE_ONLY'][p],
                    'initial_private_carried_OFFLINE_ONLY': current['private_carried_at226_OFFLINE_ONLY'][p],
                    'predicted_revenue_cow': cow['forecasts'][0]['rival_revenue'][p],
                    'predicted_revenue_goose': cow['forecasts'][2]['rival_revenue'][p],
                    'realized_revenue_cow': current['revenue'][p], 'realized_revenue_goose': goose['players'][1]['revenue'][p]})
        rows.append(row)
    life_rows = original[*seed_key, 0]['opponent_profile']['lives']
    additions = [x for x in life_rows if x[0] >= 9 and x[3] >= 226]
    # Symmetric price/quantity decomposition is an exact accounting identity, not a causal experiment.
    summary = {'exact_game_parity': parity, 'products': PRODUCTS, 'disagreements': len(attributed),
               'same_rival_actions': sum(x['same_rival_action_hash'] for x in attributed),
               'same_rival_dated_trade_flows': sum(x['same_rival_dated_trade_flows'] for x in attributed),
               'same_rival_lives': sum(x['same_rival_lives'] for x in attributed),
               'same_rival_fixed_cost': sum(x['same_rival_fixed_cost'] for x in attributed),
               'mean_own_delta_overestimate': mean(x['own_delta_overestimate'] for x in attributed),
               'mean_missed_rival_profit_delta': mean(x['missed_rival_profit_delta'] for x in attributed),
               'mean_margin_delta_overestimate': mean(x['margin_delta_overestimate'] for x in attributed),
               'mean_absolute_own_delta_error': mean(abs(x['own_delta_overestimate']) for x in attributed),
               'mean_absolute_rival_profit_delta_error': mean(abs(x['missed_rival_profit_delta']) for x in attributed),
               'per_product_mean': [{ 'product': name, **{metric: mean(x['products'][p][metric] for x in attributed)
                                          for metric in ['predicted_rival_revenue_delta', 'exact_rival_revenue_delta', 'missed_revenue_delta',
                                                         'exact_rival_spend_delta', 'quantity_term_symmetric', 'average_price_term_symmetric']}}
                                    for p, name in enumerate(PRODUCTS)],
               'limits': ['All quantities, cash and private inventories after the decision are offline diagnostic truth only.',
                          'Average price differences combine market prediction error, timing, stock feedback and future shops; this is not yet a causal ablation.',
                          'The estimator predicts rival gross animal revenue; exact profit comparisons additionally include product spend and fixed costs.',
                          'All results are the common discovery panel, not a new holdout.']}
    case_report = {'context': seed_key, 'branch_delta': 'goose minus cow', 'accounting': case,
                   'rows': rows, 'public_animals_at226': cow['rival_visible_animals_at226'],
                   'public_crops_at226': cow['rival_visible_crops_at226'],
                   'exact_future_animal_placements': additions,
                   'future_placement_counts': dict(Counter(x[0] for x in additions)),
                   'finding': 'Milk accounts for 12534 of the 13710 missed rival profit gain. Predicted milk quantity 258 versus actual249, with zero current held/private milk: the dominant gap is milk price/timing/market projection, not missing milk expansion or inventory. Exact future quantities/shops ablations are needed to separate those price mechanisms.'}
    for name, value in [('CASH_ATTRIBUTION', summary), ('cash_attribution_cases', attributed), ('cash_case_1028_v5', case_report)]:
        (RUN / f'{name}.json').write_text(json.dumps(value, indent=2) + '\n')
    with (RUN / 'cash_case_1028_v5.csv').open('w') as out:
        writer = csv.DictWriter(out, fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    print(json.dumps(summary, indent=2))
    print('Future animal placements', additions)


if __name__ == '__main__':
    main()
