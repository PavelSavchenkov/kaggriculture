from datetime import datetime, timezone
from pathlib import Path
import json
import statistics

RUN=Path(__file__).resolve().parent
PARENT=RUN.parent/'compiler_labor_sep08_001'
parity=0
rows=[]
for case in ['mixed','goose','p4','p55']:
    old=(PARENT/f'controls_results/source_hiring_{case}.json') if case.startswith('p') else (
        PARENT/f'discovery/compiler_labor_{case}_m{1 if case=="mixed" else 3}_vs_public_router.json')
    control=json.loads(old.read_text())
    new=json.loads((RUN/f'discovery/compiler_route_{case}_m0_vs_public_router.json').read_text())
    assert control['games']==new['games'],case
    parity+=len(new['games'])
    for opp in ['public_router','observed_sale_lead_start_216']:
        baseline=json.loads((RUN/f'discovery/compiler_route_{case}_m0_vs_{opp}.json').read_text())
        for mode in range(4):
            path=RUN/f'discovery/compiler_route_{case}_m{mode}_vs_{opp}.json'
            d=json.loads(path.read_text());games=d['games'];mean=lambda f:statistics.mean(f(g) for g in games)
            row={'case':case,'mode':mode,'opponent':opp,'games':len(games),'cash':mean(lambda g:g['cash']),
                'cash_gain':mean(lambda g:g['cash'])-statistics.mean(g['cash'] for g in baseline['games']),
                'margin':mean(lambda g:g['cash']-g['opponent_cash']),
                'margin_gain':mean(lambda g:g['cash']-g['opponent_cash'])-statistics.mean(g['cash']-g['opponent_cash'] for g in baseline['games']),
                'utility':d['win_utility'],'hires':mean(lambda g:g['profile']['hires']),
                'hire_cost':mean(lambda g:g['profile']['hire_cost']),
                'moves':mean(lambda g:sum(g['profile']['successful'][1:5])),
                'produced':[mean(lambda g,i=i:g['produced'][i]) for i in range(9)],
                'unit_faults':mean(lambda g:g['unit_faults'])}
            rows.append(row)
report={'created_utc':datetime.now(timezone.utc).isoformat(),'mode0_full_records_exact':parity,'discovery_games':512,'rows':rows,
    'conclusion':'No general routing improvement. Current-tile bonus reduces moves but trades animal output for crops/fertilizer. Source55 gains cash, source4 and cold goose regress. No candidate is promoted.',
    'next':'Stop tuning greedy priority constants. Separate placement-induced early land spending and missing investment funding, then use complete daily task/routing constraints for remaining service losses.'}
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text='# Routing-score results\n\n'+report['conclusion']+f' All {parity} copied controls reproduce complete prior game records.\n\n'
text+='| Case | Mode | Opponent | Cash gain | Margin gain | Moves |\n| --- | ---: | --- | ---: | ---: | ---: |\n'
for r in rows:text+=f"| {r['case']} | {r['mode']} | {r['opponent']} | {r['cash_gain']:.2f} | {r['margin_gain']:.2f} | {r['moves']:.2f} |\n"
text+='\n'+report['next']+'\n'
(RUN/'RESULTS.md').write_text(text)
print(parity,'mode0 complete records exact; no generally improved mode.')
