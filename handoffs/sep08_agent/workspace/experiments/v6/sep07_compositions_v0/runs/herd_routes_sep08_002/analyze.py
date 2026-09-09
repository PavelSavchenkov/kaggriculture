from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import statistics

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
rows = []
controls = 0
for case in ['mixed', 'goose', 'p355', 'p362']:
    for opponent in ['public_router', 'observed_sale_lead_start_216']:
        data = [json.loads((RUN / f'discovery/herd_partial_{case}_m{mode}_vs_{opponent}.json').read_text()) for mode in [1, 2]]
        old = json.loads((EXP / f'runs/herd_routes_sep08_001/discovery/herd_routes_{case}_m1_vs_{opponent}.json').read_text())
        assert data[0]['games'] == old['games']
        controls += len(old['games'])
        modes = []
        for mode, d in zip([1, 2], data):
            games = d['games']
            mean = lambda f: statistics.mean(f(g) for g in games)
            animals = [life for g in games for life in g['profile']['lives'] if life[0] >= 9]
            active = sum(life[6].bit_count() for life in animals)
            modes.append({'mode': mode, 'games': len(games), 'cash': mean(lambda g: g['cash']),
                'margin': mean(lambda g: g['cash'] - g['opponent_cash']),
                'hires': mean(lambda g: g['profile']['hires']),
                'moves': mean(lambda g: sum(g['profile']['successful'][1:5])),
                'fed_fraction': sum(life[8].bit_count() for life in animals) / active,
                'care_fraction': sum(life[9].bit_count() for life in animals) / active,
                'produced': [mean(lambda g, i=i: g['produced'][i]) for i in range(9)]})
        for row in modes:
            row['cash_gain'] = row['cash'] - modes[0]['cash']
            row['margin_gain'] = row['margin'] - modes[0]['margin']
        rows.append({'case': case, 'opponent': opponent, 'modes': modes,
                     'unchanged_records': sum(a == b for a, b in zip(data[0]['games'], data[1]['games']))})
report = {'created_utc': datetime.now(timezone.utc).isoformat(), 'games': 256,
    'old_control_full_records_equal': controls, 'rows': rows,
    'decision': 'Partial loads change cash by less than two dollars per opponent mean and do not improve broad strength. Verify the diagnosed reversal is removed, then move to mixed crop/animal task routing.'}
(RUN / 'ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')
print(controls, 'exact previous-route controls; 256 games analyzed.')
for row in rows:
 print(row['case'],row['opponent'],row['modes'][1]['cash_gain'],row['unchanged_records'])
