from datetime import datetime, timezone
from pathlib import Path
import json
import statistics

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
rows = []
parity = 0
for case in ['mixed','goose','p4','p55','p4_productive','p55_productive']:
    for opponent in ['public_router','observed_sale_lead_start_216']:
        pair = [json.loads((RUN/f'discovery/compiler_care_{case}_m{m}_vs_{opponent}.json').read_text()) for m in [0,1]]
        old_path = None
        if case in ['mixed','goose']:
            old_path = EXP/f'runs/compiler_placement_sep08_001/discovery/compiler_placement_{case}_m1_vs_{opponent}.json'
        elif case in ['p4','p55'] and opponent == 'public_router':
            old_path = EXP/f'runs/compiler_labor_sep08_001/controls_results/source_hiring_{case}.json'
        if old_path:
            old = json.loads(old_path.read_text())
            assert old['games'] == pair[0]['games'], old_path
            parity += len(old['games'])
        values = []
        for mode, data in enumerate(pair):
            games = data['games']
            mean = lambda f: statistics.mean(f(g) for g in games)
            values.append({'mode':mode,'games':len(games),'utility':data['win_utility'],
                'cash':mean(lambda g:g['cash']),'margin':mean(lambda g:g['cash']-g['opponent_cash']),
                'hires':mean(lambda g:g['profile']['hires']),'hire_cost':mean(lambda g:g['profile']['hire_cost']),
                'moves':mean(lambda g:sum(g['profile']['successful'][1:5])),
                'faults':mean(lambda g:g['unit_faults']),
                'produced':[mean(lambda g,i=i:g['produced'][i]) for i in range(9)]})
        contrast = {key:values[1][key]-values[0][key] for key in ['cash','margin','hires','hire_cost','moves','faults']}
        contrast['produced'] = [b-a for a,b in zip(values[0]['produced'],values[1]['produced'])]
        rows.append({'case':case,'opponent':opponent,'modes':values,'contrast':contrast})
report = {'created_utc':datetime.now(timezone.utc).isoformat(),'games':384,'old_complete_records_exact':parity,
    'rows':rows,'decision':'Use corrected care semantics in subsequent cold composition experiments. It increases small-farm animal production but is not a broad dense-farm improvement; extra work can displace other useful tasks.',
    'scope':'A generic compiler research correction, not an accepted strong-agent descendant. Old modes and source snapshots remain preserved.'}
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text = '# Care-bank correction results\n\n'+report['decision']+'\n\n'
text += f"384 profiled games; {parity} complete old controls exact. No incumbent change.\n\n"
text += '| Case | Opponent | Cash change | Margin change | Output change (wheat..fertilizer) |\n| --- | --- | ---: | ---: | --- |\n'
for row in rows:
    c = row['contrast']
    text += f"| {row['case']} | {row['opponent']} | {c['cash']:+.2f} | {c['margin']:+.2f} | {c['produced']} |\n"
(RUN/'RESULTS.md').write_text(text)
print('384 games;',parity,'exact old records')
for row in rows:
    print(row['case'],row['opponent'],row['contrast'])
