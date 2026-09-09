"""Measure realized composition, labor and score effects of the purchase retry."""
from pathlib import Path
import json
import math
import statistics

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
CANDIDATE, PARENT = 'rival_wool_purchase_repair_v1', 'rival_wool_context_v3'


def metrics(games):
    margins = [g['cash']-g['opponent_cash'] for g in games]
    return {'games': len(games), 'wins': sum(v > 0 for v in margins), 'ties': sum(v == 0 for v in margins),
            'utility': statistics.mean(1 if v > 0 else .5 if v == 0 else 0 for v in margins),
            'cash': statistics.mean(g['cash'] for g in games),
            'rival_cash': statistics.mean(g['opponent_cash'] for g in games),
            'margin': statistics.mean(margins), 'cvar10_margin': statistics.mean(sorted(margins)[:math.ceil(len(games)/10)]),
            'faults': statistics.mean(g['unit_faults'] for g in games),
            'hires': statistics.mean(g['profile']['hires'] for g in games),
            'hire_cost': statistics.mean(g['profile']['hire_cost'] for g in games)}


def main():
    report = {'scope': 'Exposed discovery pools, not promotion evidence.', 'comparisons': {}}
    for native in [0, 1]:
        data = {a: json.loads((RUN / f'discovery/{a}_native{native}.json').read_text())['games'] for a in [CANDIDATE, PARENT]}
        a, b = data[CANDIDATE], data[PARENT]
        assert [(g['seed'], g['seat']) for g in a] == [(g['seed'], g['seat']) for g in b]
        if native:
            assert b == json.loads((EXP / 'runs/rival_wool_validation_003/native/rival_wool_context_v3_vs_public_router_v52.json').read_text())['games']
        values = {key: metrics(games) for key, games in data.items()}
        delta = {key: values[CANDIDATE][key]-values[PARENT][key] for key in values[CANDIDATE] if key != 'games'}
        for key in ['produced', 'sold', 'discarded']:
            delta[key] = [statistics.mean(x[key][i]-y[key][i] for x, y in zip(a, b)) for i in range(12)]
        changed = [(x, y) for x, y in zip(a, b) if x['action_hash'] != y['action_hash']]
        row = {'metrics': values, 'delta': delta, 'changed': len(changed),
               'mean_delta_when_changed': {k: statistics.mean(x[k]-y[k] for x, y in changed) for k in ['cash', 'opponent_cash', 'unit_faults']} if changed else {},
               'shop_sequences_changed': sum(x['shops'] != y['shops'] for x, y in zip(a, b)),
               'full_records_equal_when_actions_equal': all(x == y for x, y in zip(a, b) if x['action_hash'] == y['action_hash'])}
        report['comparisons'][str(native)] = row
        print('native', native, 'changed', len(changed), 'delta', delta)
        print('changed-case means', row['mean_delta_when_changed'])
    (RUN / 'DISCOVERY_ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
