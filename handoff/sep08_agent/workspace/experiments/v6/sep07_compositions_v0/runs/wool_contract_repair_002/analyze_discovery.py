"""Compare repair versions on identical full-game discovery observations."""
from pathlib import Path
import json
import math
import statistics

RUN = Path(__file__).resolve().parent
AGENTS = ['wool_contract_repair_v2', 'rival_wool_purchase_repair_v1', 'rival_wool_context_v3']


def metrics(games):
    margins = [g['cash'] - g['opponent_cash'] for g in games]
    return {'games': len(games), 'wins': sum(v > 0 for v in margins),
            'ties': sum(v == 0 for v in margins),
            'utility': statistics.mean((v > 0) + .5 * (v == 0) for v in margins),
            'margin': statistics.mean(margins),
            'cvar10_margin': statistics.mean(sorted(margins)[:math.ceil(len(games) / 10)]),
            'cash': statistics.mean(g['cash'] for g in games),
            'rival_cash': statistics.mean(g['opponent_cash'] for g in games),
            'faults': statistics.mean(g['unit_faults'] for g in games),
            'hires': statistics.mean(g['profile']['hires'] for g in games),
            'hire_cost': statistics.mean(g['profile']['hire_cost'] for g in games)}


def main():
    report = {'scope': 'Exposed discovery pools only; no promotion claim.', 'comparisons': {}}
    for path in sorted((RUN / 'discovery').glob(f'{AGENTS[0]}_vs_*.json')):
        suffix = path.name.split('_vs_', 1)[1]
        data = {a: json.loads((path.parent / f'{a}_vs_{suffix}').read_text())['games'] for a in AGENTS}
        values = {a: metrics(games) for a, games in data.items()}
        comparisons = {}
        for candidate, parent in [(AGENTS[0], AGENTS[2]), (AGENTS[1], AGENTS[2]), (AGENTS[0], AGENTS[1])]:
            a, b = data[candidate], data[parent]
            assert [(g['seed'], g['seat']) for g in a] == [(g['seed'], g['seat']) for g in b]
            delta = {k: values[candidate][k] - values[parent][k] for k in values[candidate] if k != 'games'}
            paired = [x['cash'] - x['opponent_cash'] - y['cash'] + y['opponent_cash'] for x, y in zip(a, b)]
            changes = [{'seed': x['seed'], 'seat': x['seat'], 'margin_delta': margin,
                        'cash_delta': x['cash'] - y['cash'], 'fault_delta': x['unit_faults'] - y['unit_faults']}
                       for x, y, margin in zip(a, b, paired) if x != y]
            entry = {'delta': delta, 'changed': len(changes), 'minimum_paired_margin': min(paired),
                     'improved': sum(v > 0 for v in paired), 'worsened': sum(v < 0 for v in paired),
                     'production_delta': [statistics.mean(x['produced'][i] - y['produced'][i] for x, y in zip(a, b)) for i in range(12)],
                     'changes': changes}
            comparisons[f'{candidate}_over_{parent}'] = entry
            print(suffix, candidate, 'over', parent, 'changed', len(changes),
                  'wins', delta['wins'], 'margin', round(delta['margin'], 3),
                  'own', round(delta['cash'], 3), 'faults', round(delta['faults'], 3),
                  'worst', min(paired), flush=True)
        report['comparisons'][suffix] = {'metrics': values, 'contrasts': comparisons}
    assert len(report['comparisons']) == 8
    (RUN / 'DISCOVERY_ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
