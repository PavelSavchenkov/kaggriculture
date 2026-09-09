"""Compare public cash residuals with actual orders on already exposed games."""
import csv
import json
from collections import Counter
from pathlib import Path

RUN = Path(__file__).resolve().parent


def previous_context(first, second):
    return first == 7 and second in [3, 5, 6, 7] or second == 7 and first in [5, 6]


def main():
    reports = {}
    for opponent, seed in [('john_131', 1000), ('public_router_v52', 1000), ('john_131', 1930150)]:
        path = RUN / f'{opponent}_{seed}.json'
        games = json.loads(path.read_text())['games']
        with Path(str(path) + '.features.csv').open() as stream:
            features = list(csv.DictReader(stream))
        rows = []
        for game, raw in zip(games, features, strict=True):
            assert (game['seed'], game['seat']) == (int(raw['seed']), int(raw['seat']))
            f = [float(raw[f'f{i}']) for i in range(63)]
            first, second = game['shops'][:2]
            consumed = 1 + sum(shop in [0, 1, 2, 3, 5] for shop in [first, second])
            wheat_flow = f[13] - f[4] + consumed
            fertilizer_flow = f[14] - f[5]
            if 7 not in [first, second] or previous_context(first, second) or f[9] != 7 or wheat_flow != -1:
                continue
            profile = game['opponent_profile']
            fixed = sum(x[1] for x in profile['fixed_costs'] if x[0] == 144)
            extra_spend = f[1] - f[10] - 20 - f[8]
            row = {'seed': game['seed'], 'seat': game['seat'], 'shops': [first, second],
                   'rival_cash_before': f[1], 'rival_cash_after': f[10], 'wheat_buy_quote': f[8],
                   'wheat_flow': wheat_flow, 'fertilizer_flow': fertilizer_flow,
                   'public_extra_spend_after_hires_and_wheat': extra_spend,
                   'actual_fixed_spend_after_hires': fixed - 20,
                   'actual_product_flows': [x for x in profile['flows'] if x[0] == 144]}
            if fertilizer_flow == 0:
                assert extra_spend == fixed - 20
            rows.append(row)
        reports[f'{opponent}_{seed}'] = rows
        print(opponent, seed, 'contexts', len(rows), 'fert/extra spend',
              dict(Counter((x['fertilizer_flow'], x['public_extra_spend_after_hires_and_wheat']) for x in rows)))
    witness = next(x for x in reports['john_131_1930150'] if x['seat'] == 1)
    assert witness['fertilizer_flow'] == 0 and witness['public_extra_spend_after_hires_and_wheat'] == 0
    result = {'scope': '514 diagnostic games from exposed pools; observation-only residual checked against full profiles.',
              'witness': witness, 'rows': reports,
              'finding': 'Failed John fertilizer sale leaves zero spending beyond hires and wheat. V5/2 pays for seeds. Public residual matches actual non-hire fixed spending in all zero-fertilizer-flow contexts.',
              'limitation': 'Net market flows do not generally identify gross purchases and sales. This verifies these source prefixes, not arbitrary opponents.'}
    (RUN / 'PUBLIC_SPENDING_DIAGNOSIS.json').write_text(json.dumps(result, indent=2) + '\n')
    print('witness', witness)


if __name__ == '__main__':
    main()
