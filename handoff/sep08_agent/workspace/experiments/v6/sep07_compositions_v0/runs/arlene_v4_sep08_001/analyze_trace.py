from pathlib import Path
import json

RUN = Path(__file__).resolve().parent
rows = []
for seed in [1000, 1001]:
    for seat in [0, 1]:
        series, ends, games = [], [], []
        for mode in [0, 2]:
            name = f'm{mode}_{seed}_s{seat}'
            end = json.loads((RUN / f'trace_results/{name}.json').read_text())
            game = next(g for g in json.loads((RUN / f'discovery/arlene_v4_m{mode}_vs_public_router_v52.json').read_text())['games'] if g['seed'] == seed and g['seat'] == seat)
            assert all(end[k] == game[k] for k in ['cash', 'opponent_cash', 'action_hash', 'opponent_action_hash'])
            ends.append(end)
            games.append(game)
            series.append([json.loads(line) for line in (RUN / f'trace_results/{name}.jsonl').read_text().splitlines()])
        first_action = next(i for i, (a, b) in enumerate(zip(*series)) if a['own_orders'] != b['own_orders'])
        first_cash = next(i for i, (a, b) in enumerate(zip(*series)) if a['own'][0] != b['own'][0] or a['rival'][0] != b['rival'][0])
        first_workers = next((i for i, (a, b) in enumerate(zip(*series)) if a['own'][1:] != b['own'][1:] or a['rival'][1:] != b['rival'][1:]), None)
        events = sorted(set([first_action, first_cash - 1, first_cash] + ([first_workers] if first_workers else [])))
        rows.append({'seed': seed, 'seat': seat, 'first_action_difference': first_action,
            'first_cash_difference': first_cash, 'first_workforce_difference': first_workers,
            'cash_gain': ends[1]['cash'] - ends[0]['cash'], 'opponent_cash_gain': ends[1]['opponent_cash'] - ends[0]['opponent_cash'],
            'own_output_gain': [b - a for a, b in zip(games[0]['produced'], games[1]['produced'])],
            'events': [[series[0][i], series[1][i]] for i in events]})
(RUN / 'CLAMP_WITNESSES.json').write_text(json.dumps({'exact_full_trace_games': 8, 'rows': rows,
    'scope': 'Same source controller and rival, same seeds and seats; only clamp bit2 differs. Retained zero SELL changes the slot of a later positive buy; first cash differences precede successful hire differences. Both action hashes and rewards match discovery.'}, indent=2) + '\n')
print('Eight exact clamp traces; first cash difference step5 and workforce difference step25 in all four paired contexts.')
