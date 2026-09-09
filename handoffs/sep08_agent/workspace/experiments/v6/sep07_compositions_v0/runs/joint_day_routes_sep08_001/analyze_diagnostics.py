from collections import Counter
from pathlib import Path
import json

RUN = Path(__file__).resolve().parent
rows = []
records = 0
for case in ['p355', 'p362']:
    for mode in [0, 1]:
        name = f'joint_routes_{case}_m{mode}'
        games = json.loads((RUN / f'diagnostics/{name}_vs_public_router.json').read_text())['games']
        old = json.loads((RUN / f'discovery/{name}_vs_public_router.json').read_text())['games']
        assert games == old[:4]
        assert all(g['unit_faults'] == 0 for g in games)
        records += len(games)
        for seed in [1000, 1001]:
            for seat in [0, 1]:
                trace = [json.loads(line) for line in (RUN / f'diagnostics/{name}_{seed}_s{seat}.jsonl').read_text().splitlines()]
                for day in [14, 15, 16]:
                    states = [row for row in trace if row['step'] // 24 == day]
                    end = states[-1]
                    assert end['step'] % 24 == 23
                    pending = Counter()
                    for task in end['tasks']:
                        completed = any(w[1] * 10 + w[0] == task[0] and w[4] == task[1]
                            and (task[1] != 7 or w[5] == task[2]) for w in end['workers'])
                        if not completed:
                            pending[task[1]] += 1
                    operations = Counter(w[4] for row in states for w in row['workers'])
                    rows.append({'case': case, 'mode': mode, 'seed': seed, 'seat': seat, 'day': day,
                        'uncompleted_after_last_actions': dict(pending),
                        'wheat_in_shed_before_last': end['shed'][0], 'fertilizer_in_shed_before_last': end['shed'][8],
                        'carried_wheat_before_last': sum(w[2] for w in end['workers']),
                        'carried_fertilizer_before_last': sum(w[3] for w in end['workers']),
                        'pass_actions': operations[0], 'moves': sum(operations[i] for i in range(1, 5))})
report = {'full_record_exact_games': records, 'rows': rows,
    'method': 'All sixteen complete games and profiles equal discovery, with zero unit faults. Subtract distinct successful last-hour cell/operation pairs from pre-last-hour pending tile tasks; not a claim those tasks all have equal economic value.',
    'finding': 'New routes leave simultaneous feeding/watering/fertilizing tasks unfinished despite wheat/fertilizer still held in the shed or carried. Routing and input distribution remain coupled; mere total stock is not an execution guarantee.',
    'next': 'Credit actual earlier route output against later input demand, as in the persistent day solver; separately evaluate route cost, runtime pickups and shared-stock scarcity. Complete dependent task sequences and cross-route exchanges remain unresolved.'}
(RUN / 'TASK_DIAGNOSTICS.json').write_text(json.dumps(report, indent=2) + '\n')
print(records, 'exact diagnostic games;', len(rows), 'daily comparisons.')
for row in rows:
    if row['seed'] == 1000 and row['seat'] == 0 and row['day'] == 14:
        print(row)
