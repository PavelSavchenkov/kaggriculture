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
    return {'cash': mean(lambda g: g['cash']), 'margin': mean(lambda g: g['cash'] - g['opponent_cash']),
            'utility': mean(lambda g: (g['cash'] > g['opponent_cash']) + .5 * (g['cash'] == g['opponent_cash'])),
            'hires': mean(lambda g: g['profile']['hires']), 'hire_cost': mean(lambda g: g['profile']['hire_cost']),
            'moves': mean(lambda g: sum(g['profile']['successful'][1:5])),
            'faults': mean(lambda g: g['unit_faults']),
            'produced': [mean(lambda g, i=i: g['produced'][i]) for i in range(9)]}


for case in ['mixed', 'goose', 'p355', 'p362', 'p4', 'p55']:
    for opponent in ['public_router', 'observed_sale_lead_start_216']:
        data = {mode: json.loads((RUN / f'discovery/joint_resources_{case}_m{mode}_vs_{opponent}.json').read_text())['games']
                for mode in [0, 1, 2, 3, 7]}
        old = json.loads((EXP / f'runs/joint_day_routes_sep08_001/discovery/joint_routes_{case}_m1_vs_{opponent}.json').read_text())['games']
        split = json.loads((EXP / f'runs/compiler_split_sep08_001/discovery/compiler_split_{case}_vs_{opponent}.json').read_text())['games']
        assert data[0] == old
        controls += len(old)
        games += sum(map(len, data.values()))
        modes = {mode: metrics(records) for mode, records in data.items()}
        split_metrics = metrics(split)
        for mode, values in modes.items():
            for label, reference in [('joint', modes[0]), ('split', split_metrics)]:
                values[f'{label}_cash_gain'] = values['cash'] - reference['cash']
                values[f'{label}_margin_gain'] = values['margin'] - reference['margin']
            values['produced_gain'] = [a-b for a, b in zip(values['produced'], modes[0]['produced'])]
        rows.append({'case': case, 'opponent': opponent, 'modes': modes, 'original_split': split_metrics})
assert games == 960 and controls == 192
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'games': games,
          'exact_old_joint_full_records': controls, 'rows': rows,
          'decision': 'Do not replace the compiler with these routes. Cumulative cargo helps some dense cases but does not recover the losses from replacing the original split compiler. The scarcity penalty is not generally useful.',
          'next': 'Use exact day-solver schedules on changed midseason states to measure missing task dependencies and input transfers. Do not keep tuning route costs while leaving those constraints absent.'}
(RUN / 'ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')
text = '# Cumulative-input route results\n\n' + report['decision'] + '\n\n'
text += '960 full games; 192 complete old route controls equal. No strongest-agent promotion.\n\n'
text += '| Farm | Opponent | Mode | Cash gain vs joint | Cash gain vs original split | Margin gain vs joint |\n| --- | --- | ---: | ---: | ---: | ---: |\n'
for row in rows:
    for mode, values in row['modes'].items():
        if mode:
            text += f"| {row['case']} | {row['opponent']} | {mode} | {values['joint_cash_gain']:+.2f} | {values['split_cash_gain']:+.2f} | {values['joint_margin_gain']:+.2f} |\n"
text += '\nMode bits: 1 costs initial inputs after crediting earlier route harvest/collection; 2 uses that calculation for withdrawals; 4 penalizes currently missing stock. Mode 7 combines all three.\n\n' + report['next'] + '\n'
(RUN / 'RESULTS.md').write_text(text)
(RUN / 'README.md').write_text('# Cumulative-input routes\n\nRead RESULTS.md, ANALYSIS.json and LINEAGE.json. prepare.py creates 30 C++ policies from the earlier joint routes; run_discovery.py runs the 960-game comparison; analyze.py verifies 192 complete controls and compares both the route parent and original compiler. check_operations.py covers generic/pair/debug, instances, self-play and PASS. These routes were not accepted as a compiler or strongest-agent replacement.\n')
paths = [RUN / 'routes.hpp', RUN / 'prepare.py', EXP / 'runs/joint_day_routes_sep08_001/compiler/source/agent.hpp', EXP / 'runs/joint_day_routes_sep08_001/compiler/source/agent.cpp']
(RUN / 'SOURCE_HASHES.json').write_text(json.dumps({str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}, indent=2) + '\n')
print(report['decision'], games, 'games;', controls, 'exact controls.')
