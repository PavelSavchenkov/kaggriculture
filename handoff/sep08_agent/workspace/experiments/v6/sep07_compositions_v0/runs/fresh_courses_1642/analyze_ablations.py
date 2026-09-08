"""Attribute complete component-control outcomes on identical games."""
import json
from pathlib import Path

RUN = Path(__file__).resolve().parent
AGENTS = ['fresh_bohann25_opening', 'fresh_bohann25_suffix1', 'fresh_bohann25_suffix6', 'fresh_bohann25_opening_suffix6']
OPPONENTS = ['crop_mix_t2_wheat', 'wheat_one_fert', 'teammate_shoprouter', 'public_router', 'king_rc4', 'public_router_v5']


def main():
    rows = []
    for agent in AGENTS:
        row = {'agent': agent, 'per_opponent': {}}
        for opponent in OPPONENTS:
            data = json.loads((RUN / f'ablation_discovery/{agent}_vs_{opponent}.json').read_text())
            base = json.loads((RUN / f'ablation_discovery/crop_mix_t2_wheat_vs_{opponent}.json').read_text())
            a, b = data['games'], base['games']
            assert len(a) == len(b) == 128
            assert [(g['seed'], g['seat']) for g in a] == [(g['seed'], g['seat']) for g in b]
            result = {key: data[key] for key in ('win_utility', 'mean_margin', 'margin_cvar10')}
            result['utility_gain'] = data['win_utility'] - base['win_utility']
            result['changed_actions'] = sum(x['action_hash'] != y['action_hash'] for x, y in zip(a, b))
            result['changed_rival_actions'] = sum(x['opponent_action_hash'] != y['opponent_action_hash'] for x, y in zip(a, b))
            for key in ('cash', 'opponent_cash', 'revenue', 'spend', 'unit_faults', 'worker_days'):
                result[key + '_delta'] = sum(x[key] - y[key] for x, y in zip(a, b)) / len(a)
            result['margin_delta'] = result['cash_delta'] - result['opponent_cash_delta']
            for key in ('produced', 'sold', 'discarded'):
                result[key + '_delta'] = [sum(x[key][i] - y[key][i] for x, y in zip(a, b)) / len(a) for i in range(9)]
            result['wins'] = sum(g['cash'] > g['opponent_cash'] for g in a)
            result['ties'] = sum(g['cash'] == g['opponent_cash'] for g in a)
            row['per_opponent'][opponent] = result
        row['mean_margin_delta'] = sum(v['margin_delta'] for v in row['per_opponent'].values()) / len(OPPONENTS)
        row['mean_utility_delta'] = sum(v['utility_gain'] for v in row['per_opponent'].values()) / len(OPPONENTS)
        rows.append(row)
    report = {'scope': 'Discovery: four component controls, 64 seeds both seats, six opponents; no promotion.', 'results': rows}
    (RUN / 'ABLATION_SCREEN.json').write_text(json.dumps(report, indent=2) + '\n')
    for row in rows:
        print(row['agent'], 'margin delta', round(row['mean_margin_delta'], 2), 'utility delta', round(row['mean_utility_delta'], 4))
        for opponent, result in row['per_opponent'].items():
            print(' ', opponent, 'win', result['wins'], 'ties', result['ties'], 'margin delta', round(result['margin_delta'], 2),
                  'own', round(result['cash_delta'], 2), 'rival', round(result['opponent_cash_delta'], 2), 'changed', result['changed_actions'])


if __name__ == '__main__':
    main()
