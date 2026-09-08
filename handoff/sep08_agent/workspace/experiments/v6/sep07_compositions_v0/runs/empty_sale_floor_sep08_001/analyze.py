from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import statistics

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
spec = json.loads((RUN / 'LINEAGE.json').read_text())
old = EXP / 'runs/empty_sale_slots_sep08_001/discovery'
parent = 'observed_sale_lead_start_216'
rows = []
controls = games = 0


def metrics(records):
    margins = [g['cash']-g['opponent_cash'] for g in records]
    return {'games': len(records), 'wins': sum(m > 0 for m in margins),
            'ties': sum(m == 0 for m in margins), 'losses': sum(m < 0 for m in margins),
            'utility': statistics.mean((m > 0) + .5*(m == 0) for m in margins),
            'margin': statistics.mean(margins), 'cash': statistics.mean(g['cash'] for g in records)}


for opponent in spec['discovery']['opponents']:
    control = json.loads((old / f'empty_sale_slots_m2_vs_{opponent}.json').read_text())['games']
    baseline = json.loads((old / f'{parent}_vs_{opponent}.json').read_text())['games']
    row = {'opponent': opponent, 'accepted_parent': metrics(baseline), 'old_empty_removal': metrics(control), 'modes': {}}
    for mode in range(4):
        name = f'empty_sale_floor_m{mode}'
        records = json.loads((RUN / f'discovery/{name}_vs_{opponent}.json').read_text())['games']
        assert len(records) == 128
        games += len(records)
        if mode == 0:
            assert records == control
            controls += len(records)
        values = metrics(records)
        for label, reference in [('parent', baseline), ('control', control)]:
            gains = [a['cash']-a['opponent_cash']-b['cash']+b['opponent_cash'] for a, b in zip(records, reference)]
            values[f'{label}_margin_gain'] = statistics.mean(gains)
            values[f'{label}_minimum_paired_margin_gain'] = min(gains)
            values[f'{label}_utility_gain'] = values['utility'] - metrics(reference)['utility']
        values['changed_production_vs_parent'] = sum(a['produced'] != b['produced'] for a, b in zip(records, baseline))
        row['modes'][mode] = values
    rows.append(row)
assert games == 4096 and controls == 1024
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'games': games,
          'old_empty_removal_exact_full_records': controls, 'rows': rows,
          'scope': 'Previously exposed discovery only. Required operational checks, causal witness replay and unused frozen fresh/native audits remain. No promotion based on this report.'}
(RUN / 'ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')
text = '# Empty-sale price-floor guard discovery\n\n' + report['scope'] + '\n\n'
text += '4096 full games; 1024 exact old empty-removal controls.\n\n'
text += '| Opponent | Mode | W/T/L | Margin gain vs accepted parent | Margin gain vs old removal | Minimum paired gain vs parent |\n| --- | ---: | --- | ---: | ---: | ---: |\n'
for row in rows:
    for mode, values in row['modes'].items():
        text += f"| {row['opponent']} | {mode} | {values['wins']}/{values['ties']}/{values['losses']} | {values['parent_margin_gain']:+.2f} | {values['control_margin_gain']:+.2f} | {values['parent_minimum_paired_margin_gain']:+d} |\n"
(RUN / 'RESULTS.md').write_text(text)
paths = [RUN / 'prepare.py', *sorted((RUN / 'proposals').rglob('agent.hpp')), *sorted((RUN / 'proposals').rglob('agent.cpp'))]
(RUN / 'SOURCE_HASHES.json').write_text(json.dumps({str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}, indent=2) + '\n')
print(games, 'full discovery games;', controls, 'complete old controls equal.')
for row in rows:
    print(row['opponent'], [(mode, round(v['parent_margin_gain'], 3), v['parent_minimum_paired_margin_gain']) for mode, v in row['modes'].items()])
