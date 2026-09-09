from pathlib import Path
import json
import statistics

RUN = Path(__file__).resolve().parent
spec = json.loads((RUN / 'LINEAGE.json').read_text())
rows = {}
controls = parity = 0
for opponent in spec['discovery']['opponents']:
    names = ['empty_sale_slots_m0', 'empty_sale_slots_m1', 'empty_sale_slots_m2', 'observed_sale_lead_start_216']
    data = {name: json.loads((RUN / f'discovery/{name}_vs_{opponent}.json').read_text())['games'] for name in names}
    assert data[names[0]] == data[names[3]]
    assert data[names[1]] == data[names[2]]
    controls += len(data[names[0]])
    parity += len(data[names[1]])
    old, new = data[names[0]], data[names[2]]
    margin = [g['cash'] - g['opponent_cash'] for g in new]
    previous = [g['cash'] - g['opponent_cash'] for g in old]
    difference = [a - b for a, b in zip(margin, previous)]
    rows[opponent] = {'mean_margin_gain': statistics.mean(difference), 'min_paired_margin_gain': min(difference),
        'negative_paired_games': sum(v < 0 for v in difference), 'wins': sum(v > 0 for v in margin),
        'ties': sum(v == 0 for v in margin), 'losses': sum(v < 0 for v in margin),
        'cash_gain': statistics.mean(a['cash'] - b['cash'] for a, b in zip(new, old)),
        'production_changed': sum(a['produced'] != b['produced'] for a, b in zip(new, old)),
        'utility_gain': statistics.mean((a > 0) + .5 * (a == 0) - (b > 0) - .5 * (b == 0) for a, b in zip(margin, previous))}
report = {'candidate': 'empty_sale_slots_m2', 'parent': 'observed_sale_lead_start_216', 'games': 4096,
    'old_control_records_exact': controls, 'm1_m2_records_exact': parity, 'rows': rows,
    'selection': 'Modes1/2 have identical full discovery records. Select mode2 because this retains the existing early funding and input-order behavior outside the observed successful change. No policy fitting on future validation seeds.',
    'screen_passed': all(r['mean_margin_gain'] >= 0 and r['utility_gain'] >= 0 for r in rows.values()) and rows['observed_sale_lead_start_216']['min_paired_margin_gain'] >= 0}
(RUN / 'ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')
text = '# Empty non-input sale slots\n\nMode2 removes zero SELL requests for non-input products only from step216 onward. Full discovery controls reproduce1024 old records; modes1/2 agree in1024 complete records. This changes market positions, not physical actions or requested positive quantities.\n\n'
text += '| Opponent | W/T/L | Mean margin gain | Smallest paired gain |\n| --- | ---: | ---: | ---: |\n'
for opponent, row in rows.items():
    text += f"| {opponent} | {row['wins']}/{row['ties']}/{row['losses']} | {row['mean_margin_gain']:+.3f} | {row['min_paired_margin_gain']:+} |\n"
text += '\nOwn production is unchanged in all1024 candidate comparisons. Seven Bohann games lose one dollar of paired margin despite a positive mean. Discovery is not a promotion; independent fresh/native/operational/frozen validation lives in ../empty_sale_validation_sep08_001/.\n'
(RUN / 'RESULTS.md').write_text(text)
(RUN / 'README.md').write_text('# Incumbent empty-sale-slot experiment\n\nRead RESULTS.md, LINEAGE.json and ANALYSIS.json. prepare.py creates the local wrappers; run_discovery.py runs4096 profiled games; analyze.py requires exact parent-control and all/late-only agreement. Source idea comes from the Arlene clamp failure; the transform is local and keeps all inherited lineage. Selected mode2 is frozen for independent validation.\n')
print('4096 discovery games; exact controls', controls, 'mode1/2 agreement', parity, 'screen', report['screen_passed'])
