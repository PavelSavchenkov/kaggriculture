"""Paired economic attribution for the completed placement discovery panel."""
import json
from pathlib import Path

RUN = Path(__file__).resolve().parent


def main():
    base = json.loads((RUN / 'profiles/wheat_one_fert.json').read_text())['games']
    rows = []
    for path in sorted((RUN / 'profiles').glob('wp_*.json')):
        games = json.loads(path.read_text())['games']
        assert len(games) == len(base) == 64
        assert [(g['seed'], g['seat']) for g in games] == [(g['seed'], g['seat']) for g in base]
        row = {'agent': path.stem, 'games': len(games)}
        for key in ('cash', 'opponent_cash', 'unit_faults', 'worker_days', 'revenue', 'spend'):
            row[key] = sum(x[key] - y[key] for x, y in zip(games, base)) / len(base)
        for key in ('hires', 'hire_cost', 'weed_digs'):
            row[key] = sum(x['profile'][key] - y['profile'][key] for x, y in zip(games, base)) / len(base)
        for key in ('produced', 'sold', 'discarded'):
            row[key] = [sum(x[key][i] - y[key][i] for x, y in zip(games, base)) / len(base) for i in range(9)]
        for key, count in (('buys', 12), ('seed_buys', 5)):
            row[key] = [sum(x['profile'][key][i] - y['profile'][key][i] for x, y in zip(games, base)) / len(base) for i in range(count)]
        row['different_action_games'] = sum(x['action_hash'] != y['action_hash'] for x, y in zip(games, base))
        row['different_cash_games'] = sum(x['cash'] != y['cash'] for x, y in zip(games, base))
        assert abs(row['cash'] - row['revenue'] + row['spend']) < 1e-9
        rows.append(row)
    report = {'scope': '64 paired public_router games, seeds 1000..1031, both seats; mean candidate minus wheat_one_fert.',
              'product_order': ['WHEAT', 'CARROT', 'TOMATO', 'STRAWBERRY', 'MELON', 'EGG', 'MILK', 'WOOL', 'FERTILIZER'],
              'rows': rows}
    (RUN / 'profiles/CAUSAL.json').write_text(json.dumps(report, indent=2) + '\n')
    for row in rows:
        print(row['agent'], 'cash', row['cash'], 'labor', row['hire_cost'], 'changed actions', row['different_action_games'])


if __name__ == '__main__':
    main()
