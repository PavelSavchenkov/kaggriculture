"""Find the earliest actual execution failures in the newly selected farms."""
from collections import Counter
from pathlib import Path
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
OPS = 'PASS NORTH SOUTH EAST WEST PICKUP DROP PLACE PLANT WATER HARVEST FERTILIZE DIG BUILD_COOP BUILD_PASTURE FEED COLLECT_FERTILIZER CARE'.split()


def main():
    actual = json.loads((RUN / 'native.json').read_text())['games']
    expected = json.loads((EXP / 'runs/rival_wool_validation_003/native/rival_wool_context_v3_vs_public_router_v52.json').read_text())['games']
    assert actual == expected
    rows = []
    failures, first_days, guard_days = Counter(), Counter(), Counter()
    for game in actual:
        path = RUN / f'native.json.{game["seed"]}_{game["seat"]}.trace.jsonl'
        trace = [json.loads(line) for line in path.read_text().splitlines()]
        assert sum(x['unit_faults'] for x in trace) == game['unit_faults']
        if not any(x['selected'] for x in trace):
            continue
        faults = [x for x in trace if x['selected'] and x['unit_faults']]
        guards = [x for x in trace if x['day_guard_differences'] and not x['day_guard_differences'][0]['matches']]
        for step in faults:
            for unit in step['failed_units']:
                failures[OPS[unit['action'][0]]] += 1
        first_days[faults[0]['step'] // 24 if faults else -1] += 1
        guard_days[guards[0]['step'] // 24 if guards else -1] += 1
        row = {'seed': game['seed'], 'seat': game['seat'], 'shops': game['shops'],
               'cash': game['cash'], 'opponent_cash': game['opponent_cash'], 'unit_faults': game['unit_faults'],
               'first_failed_unit_step': faults[0] if faults else None,
               'first_guard_rejection': guards[0] if guards else None,
               'day_status': [{'day': x['step'] // 24, 'cash': x['cash'], 'matched': x['matched'],
                              'guards': x['day_guard_differences']} for x in trace if x['selected'] and x['step'] % 24 == 0]}
        rows.append(row)
    result = {'games': len(actual), 'complete_records_equal': True, 'activated_games': len(rows),
              'failed_ops': dict(failures), 'first_unit_failure_days': dict(first_days),
              'first_guard_rejection_days': dict(guard_days), 'cases': rows}
    (RUN / 'EXECUTION_DIAGNOSIS.json').write_text(json.dumps(result, indent=2) + '\n')
    print({k: v for k, v in result.items() if k != 'cases'})
    for seed in [1954003, 1954045]:
        row = next(x for x in rows if x['seed'] == seed and x['seat'] == 0)
        fault = row['first_failed_unit_step']
        guard = row['first_guard_rejection']
        print('witness', seed, 'first fault', fault['step'], fault['failed_units'])
        print('first guard', guard['step'], guard['day_guard_differences'])


if __name__ == '__main__':
    main()
