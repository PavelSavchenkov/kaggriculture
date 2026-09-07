"""Extract offline oracle inputs from already validated C++ games; no gameplay."""
import hashlib
import json
from pathlib import Path

RUN = Path(__file__).resolve().parent
SOURCE = RUN.parent / 'atakan_portfolio_001'
RIVALS = ['animal_adaptive_r1_c0_b0', 'teammate_shoprouter', 'public_router_v5', 'king_rc4', 'public_router']
BRANCHES = ['atakan_cow', 'atakan_sheep', 'atakan_goose']


def read(path):
    return json.loads(path.read_text())


def flows(profile):
    result = [[[0]*9 for _ in range(30)] for _ in range(2)]
    for step, product, bought, sold in profile['flows']:
        if step >= 226:
            result[0][step//24][product] += sold
            result[1][step//24][product] += bought
    return result


def flatten(value):
    for element in value:
        if isinstance(element, list):
            yield from flatten(element)
        else:
            yield str(element)


def main():
    hashes = {}

    def load(name):
        path = SOURCE / name
        hashes[name] = hashlib.sha256(path.read_bytes()).hexdigest()
        return read(path)

    models = load('flow_models.json')
    inputs, expected = [], []
    for rival in RIVALS:
        panels = [load(f'results/{name}_vs_{rival}.json')['games'] for name in BRANCHES]
        audits = [load(f'results/cash_{name}_vs_{rival}.json') for name in BRANCHES]
        traces = load(f'results/trace_{rival}.json')
        for index, trace in enumerate(traces):
            games = [panel[index] for panel in panels]
            cash = [audit[index] for audit in audits]
            assert all((g['seed'], g['seat']) == (trace['seed'], trace['seat']) for g in games)
            shops = games[0]['shops']
            assert all(g['shops'] == shops for g in games)
            oracle = []
            for branch, game in enumerate(games):
                own, other = flows(game['profile']), flows(game['opponent_profile'])
                fixed = [[0]*30 for _ in range(2)]
                for player, key in enumerate(['profile', 'opponent_profile']):
                    for step, cost, orders in game[key]['fixed_costs']:
                        if step >= 226:
                            fixed[player][step//24] += cost
                assert fixed[0] == models[branch]['fixed_costs']
                oracle.append([*own, *other, *fixed])
            inputs.append([rival, trace['seed'], trace['seat'], shops, oracle])
            expected.append({'rival': rival, 'seed': trace['seed'], 'seat': trace['seat'], 'shops': shops,
                             'baseline_estimates': trace['estimates'], 'cash_before226': trace['cash_before226'],
                             'rival_cash_before226': trace['rival_cash_before226'],
                             'actual_own_profit': [g['cash']-trace['cash_before226'] for g in games],
                             'actual_rival_profit': [g['opponent_cash']-trace['rival_cash_before226'] for g in games],
                             'actual_margins': [g['cash']-g['opponent_cash'] for g in games],
                             'physical_before226': trace['physical_before226'],
                             'exact_cash': [a['players'] for a in cash]})
    (RUN / 'inputs').mkdir(parents=True, exist_ok=True)
    with (RUN / 'inputs/oracles.txt').open('w') as out:
        out.write(' '.join(flatten([[m['sales'], m['buys'], m['fixed_costs']] for m in models]))+'\n')
        out.write(str(len(inputs))+'\n')
        for record in inputs:
            out.write(' '.join(flatten(record))+'\n')
    (RUN / 'inputs/expected.json').write_text(json.dumps(expected, separators=(',', ':'))+'\n')
    (RUN / 'INPUT_LINEAGE.json').write_text(json.dumps({'source_run': str(SOURCE), 'input_sha256': hashes,
        'scope': 'Offline future-quantity and shop oracles only. Not a deployable estimator or policy.',
        'contexts': len(inputs), 'branches_per_context': 3,
        'flat_format': 'Three donor [sales30x9,buys30x9,fixed30], context count, then each rival seed seat shops8 and three [own sales30x9,own buys30x9,rival sales30x9,rival buys30x9,own fixed30,rival fixed30].'}, indent=2)+'\n')
    print(f'Prepared {len(inputs)} contexts and {3*len(inputs)} branch flow plans.')


if __name__ == '__main__':
    main()
