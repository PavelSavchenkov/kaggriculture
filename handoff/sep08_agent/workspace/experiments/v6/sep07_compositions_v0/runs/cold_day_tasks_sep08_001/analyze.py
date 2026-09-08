from datetime import datetime, timezone
from pathlib import Path
import json
import statistics

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
spec=json.loads((RUN/'LINEAGE.json').read_text())
rows=[];parity=0
for opp in spec['discovery']['opponents']:
    control=json.loads((RUN/f'discovery/cold_day_tasks_control_vs_{opp}.json').read_text())
    if opp!='pass':
        old=json.loads((EXP/f'runs/compiler_placement_sep08_001/discovery/compiler_placement_mixed_m1_vs_{opp}.json').read_text())
        actual={(g['seed'],g['seat']):g for g in control['games']}
        for g in old['games']:assert g==actual[(g['seed'],g['seat'])]
        parity+=len(old['games'])
    previous={(g['seed'],g['seat']):g for g in control['games']}
    for a in spec['variants']:
        d=json.loads((RUN/f'discovery/{a}_vs_{opp}.json').read_text());gs=d['games']
        mean=lambda f:statistics.mean(f(g) for g in gs)
        rows.append({'agent':a,'opponent':opp,'games':len(gs),'utility':d['win_utility'],
            'cash':mean(lambda g:g['cash']),'cash_gain':mean(lambda g:g['cash']-previous[(g['seed'],g['seat'])]['cash']),
            'margin_gain':mean(lambda g:(g['cash']-g['opponent_cash'])-(previous[(g['seed'],g['seat'])]['cash']-previous[(g['seed'],g['seat'])]['opponent_cash'])),
            'hire_cost':mean(lambda g:g['profile']['hire_cost']),'buy_wheat':mean(lambda g:g['profile']['buys'][0]),
            'unit_faults':mean(lambda g:g['unit_faults']),
            'equal_production_games':sum(g['produced']==previous[(g['seed'],g['seat'])]['produced'] for g in gs),
            'produced':[mean(lambda g,i=i:g['produced'][i]) for i in range(9)]})
coverage=json.loads((RUN/'COVERAGE.json').read_text())
assert all(x['complete_day_schedule']==x['games'] for x in coverage['summary'].values())
report={'created_utc':datetime.now(timezone.utc).isoformat(),'discovery_games':768,'control_full_records_exact':parity,
    'solver':spec['solver_results'],'coverage':coverage,'rows':rows,
    'findings':['All three task-generated first days solve in about51–67milliseconds and match exact root-engine endpoints againstPASS.',
        'The instrumented96 full games reproduce their discovery records; all96 execute all24 planned first-day hours without the unit-count fallback.',
        'The first-day replacement adds only about$1–$100mean own cash in these samples. Most of the full-season loss remains later.',
        'Skipping first-day service saves bought wheat without reducing animal output in this particular reactive continuation. This is not proof that the service is biologically redundant under improved later schedules.',
        'The full-service version also changes realized future wages against active rivals despite the same hiring rule. Execution and financing effects remain coupled.',
        'No new agent beats the active opponents in this cold-farm panel, and no strong reference is promoted.'],
    'next':'Generate and solve later daily tasks, with exact capital/supply/withdrawal constraints and cheaper workforce choices. Diagnose which days first lose the requested output instead of tuning first-day details.'}
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text='# Task-generated first-day results\n\n'+'\n\n'.join(report['findings'])+'\n\n'
text+='| Agent | Opponent | Cash | Cash gain | Margin gain | Wages | Wheat bought | Equal-production games |\n| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |\n'
for r in rows:text+=f"| {r['agent']} | {r['opponent']} | {r['cash']:.2f} | {r['cash_gain']:.2f} | {r['margin_gain']:.2f} | {r['hire_cost']:.2f} | {r['buy_wheat']:.2f} | {r['equal_production_games']}/{r['games']} |\n"
text+='\n'+report['next']+'\n'
(RUN/'RESULTS.md').write_text(text)
print(parity,'control records exact; all96 instrumented games use the entire compiled first day.')
for r in rows:print(r['agent'],r['opponent'],'cashgain',round(r['cash_gain'],3),'equalproduction',r['equal_production_games'])
