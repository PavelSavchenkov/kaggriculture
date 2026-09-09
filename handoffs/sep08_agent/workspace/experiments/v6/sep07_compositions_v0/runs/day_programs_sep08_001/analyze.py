from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import statistics

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
rows = []
controls = games = 0


def metrics(records):
    mean = lambda f: statistics.mean(f(g) for g in records)
    values = {k: mean(lambda g, k=k: g[k]) for k in ['cash', 'opponent_cash', 'unit_faults', 'worker_days']}
    values['margin'] = values['cash'] - values['opponent_cash']
    values['utility'] = mean(lambda g: (g['cash'] > g['opponent_cash']) + .5 * (g['cash'] == g['opponent_cash']))
    for k in ['hires', 'hire_cost', 'land_cost']:
        values[k] = mean(lambda g, k=k: g['profile'][k])
    values['moves'] = mean(lambda g: sum(g['profile']['successful'][1:5]))
    for k in ['produced', 'sold', 'discarded']:
        values[k] = [mean(lambda g, i=i, k=k: g[k][i]) for i in range(12)]
    values['successful'] = [mean(lambda g, i=i: g['profile']['successful'][i]) for i in range(18)]
    return values


for case in ['p355', 'p362']:
    for opponent in ['public_router', 'observed_sale_lead_start_216', f'joint_routes_{case}_m0', 'pass']:
        data = {mode: json.loads((RUN / f'discovery/day_program_{case}_m{mode}_vs_{opponent}.json').read_text())['games'] for mode in range(4)}
        expected = [(seed, seat) for seed in range(1000, 1008) for seat in range(2)]
        assert all([(g['seed'], g['seat']) for g in records] == expected and all(g['turns'] == 719 for g in records) for records in data.values())
        if opponent in ['public_router', 'observed_sale_lead_start_216']:
            old = json.loads((EXP / f'runs/joint_day_routes_sep08_001/discovery/joint_routes_{case}_m0_vs_{opponent}.json').read_text())['games']
            assert data[0] == old
            controls += len(old)
        games += sum(map(len, data.values()))
        values = {mode: metrics(records) for mode, records in data.items()}
        for mode in range(4):
            v = values[mode]
            v['gain'] = {k: v[k]-values[0][k] for k in ['cash', 'margin', 'utility', 'unit_faults', 'hires', 'hire_cost', 'moves']}
            for k in ['produced', 'sold', 'discarded', 'successful']:
                v['gain'][k] = [a-b for a, b in zip(v[k], values[0][k])]
            v['changed_games'] = sum(a != b for a, b in zip(data[mode], data[0]))
            v['paired_margin_losses'] = [{'seed': a['seed'], 'seat': a['seat'], 'gain': a['cash']-a['opponent_cash']-b['cash']+b['opponent_cash']} for a, b in zip(data[mode], data[0]) if a['cash']-a['opponent_cash'] < b['cash']-b['opponent_cash']]
        rows.append({'case': case, 'opponent': opponent, 'modes': values})
assert controls == 64 and games == 512
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'games': games, 'exact_old_controls': controls, 'rows': rows,
    'decision': 'Keep relaxed day-program reuse as an experimental compiler component. It improves both cold farms against the public router and accepted reference, but p362 loses to its original compiler. Neither farm challenges the accepted reference. Operational and activation checks remain required.',
    'next': 'Measure activation, interrupted plans and economic consequences before selecting or repairing programs. A legal day program need not be a profitable season change.'}
(RUN / 'ANALYSIS.json').write_text(json.dumps(report, indent=2)+'\n')
text = '# Day-program discovery\n\n'+report['decision']+'\n\n512 full games; 64 complete original controls equal.\n\n| Farm | Opponent | Mode | Changed / 16 | Cash gain | Margin gain | Hire-cost gain |\n|---|---|---:|---:|---:|---:|---:|\n'
for row in rows:
    for mode, v in row['modes'].items():
        if mode:
            text += f"| {row['case']} | {row['opponent']} | {mode} | {v['changed_games']} | {v['gain']['cash']:+.2f} | {v['gain']['margin']:+.2f} | {v['gain']['hire_cost']:+.2f} |\n"
(RUN / 'RESULTS.md').write_text(text+'\n'+report['next']+'\n')
print('Day programs:', games, 'games;', controls, 'exact controls.')
for row in rows:
    v = row['modes'][3]
    print(row['case'], row['opponent'], 'mode3', 'changed', v['changed_games'], 'gain', v['gain'])
