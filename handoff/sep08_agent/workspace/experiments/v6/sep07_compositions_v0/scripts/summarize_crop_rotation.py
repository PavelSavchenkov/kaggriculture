"""Compare complete crop-rotation profiles with their paired source games."""
import argparse
import json
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('run')
    args = parser.parse_args()
    folder = EXP / 'runs' / args.run
    parent = json.loads((folder / 'exact/parent.json').read_text())['games']
    candidate = json.loads((folder / 'exact/candidate.json').read_text())['games']
    assert len(parent) == len(candidate) > 0
    pairs = list(zip(parent, candidate))
    assert all((a['seed'], a['seat']) == (b['seed'], b['seat']) for a, b in pairs)
    count = len(pairs)

    def mean_delta(key, profile=False):
        return sum((b['profile'] if profile else b)[key] -
                   (a['profile'] if profile else a)[key] for a, b in pairs) / count

    def vector_delta(key, profile=False):
        size = len((parent[0]['profile'] if profile else parent[0])[key])
        return [sum((b['profile'] if profile else b)[key][i] -
                    (a['profile'] if profile else a)[key][i] for a, b in pairs) / count
                for i in range(size)]

    report = {
        'games': count,
        'changed': sum(a['action_hash'] != b['action_hash'] for a, b in pairs),
        'cash_delta': mean_delta('cash'),
        'rival_delta': mean_delta('opponent_cash'),
        'mean_margin_delta': mean_delta('cash') - mean_delta('opponent_cash'),
        'wins': sum(b['cash'] > b['opponent_cash'] for a, b in pairs),
        'parent_wins': sum(a['cash'] > a['opponent_cash'] for a, b in pairs),
        'hires_delta': mean_delta('hires', True),
        'hire_cost_delta': mean_delta('hire_cost', True),
        'own_cash_before_labor_delta': mean_delta('cash') + mean_delta('hire_cost', True),
        'faults_delta': mean_delta('unit_faults'),
        'output_delta_cases': sorted({tuple(y - x for x, y in zip(a['produced'], b['produced'])) for a, b in pairs}),
        'scope': 'Discovery seeds1000..1031, both seats, public_router. Diagnostic only; no promotion.',
    }
    for key in ['produced', 'sold', 'discarded']:
        report[key + '_delta'] = vector_delta(key)
    for key in ['buys', 'seed_buys']:
        report[key + '_delta'] = vector_delta(key, True)
    (folder / 'SCREEN.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
