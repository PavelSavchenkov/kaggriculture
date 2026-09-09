from pathlib import Path
from datetime import datetime, timezone
import json
import statistics

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
rows=[];controls=0
for case in ['mixed','p355','p362','p4','p55']:
    for opponent in ['public_router','observed_sale_lead_start_216']:
        data={b:json.loads((RUN/f'discovery/compiler_feed_{case}_b{b}_vs_{opponent}.json').read_text()) for b in [1,2,4]}
        if case in ['p355','p362']:
            path=EXP/f'runs/dated_expansion_sep08_001/discovery/dated_expansion_{case}_vs_{opponent}.json'
        else:
            path=EXP/f'runs/compiler_care_sep08_001/discovery/compiler_care_{case}_m1_vs_{opponent}.json'
        old=json.loads(path.read_text())
        keys={(g['seed'],g['seat']) for g in old['games']}
        current=[g for g in data[4]['games'] if (g['seed'],g['seat']) in keys]
        assert current==old['games'];controls+=len(current)
        modes=[]
        for bundle,d in data.items():
            games=d['games'];mean=lambda f:statistics.mean(f(g) for g in games)
            animals=[l for g in games for l in g['profile']['lives'] if l[0]>=9]
            active=sum(l[6].bit_count() for l in animals)
            modes.append({'bundle':bundle,'games':len(games),'cash':mean(lambda g:g['cash']),
                'margin':mean(lambda g:g['cash']-g['opponent_cash']),'utility':d['win_utility'],
                'hires':mean(lambda g:g['profile']['hires']),'hire_cost':mean(lambda g:g['profile']['hire_cost']),
                'moves':mean(lambda g:sum(g['profile']['successful'][1:5])),
                'faults':mean(lambda g:g['unit_faults']),
                'fed_fraction':sum(l[8].bit_count() for l in animals)/active,
                'care_fraction':sum(l[9].bit_count() for l in animals)/active,
                'produced':[mean(lambda g,i=i:g['produced'][i]) for i in range(9)]})
        base=next(m for m in modes if m['bundle']==4)
        for m in modes:
            m['cash_gain']=m['cash']-base['cash'];m['margin_gain']=m['margin']-base['margin']
            m['produced_gain']=[a-b for a,b in zip(m['produced'],base['produced'])]
        rows.append({'case':case,'opponent':opponent,'modes':modes})
report={'created_utc':datetime.now(timezone.utc).isoformat(),'games':480,'old_control_full_records_equal':controls,'rows':rows,
    'decision':'Reject a universal smaller wheat bundle. It helps the small mixed farm but hurts the larger cold and dense source farms on broad cash/margin measures. Delivery needs a complete route and resource assignment, not only smaller loads.',
    'next':'Use the exact day-solver delivery witness to design a reusable day-task/unit-route planner with current stocks and optional service tasks. Separate physical action planning from market projection so new schedules retain valid sales/purchases. Require old behavior parity after refactoring.',
    'scope':'Compiler research only; accepted strongest agent remains unchanged.'}
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text='# Wheat bundle comparison\n\n'+report['decision']+'\n\n'
text+=f'480 profiled games; {controls} old complete controls exact.\n\n'
text+='| Case | Opponent | Bundle | Cash gain | Margin gain | Fed fraction | Moves |\n| --- | --- | ---: | ---: | ---: | ---: | ---: |\n'
for row in rows:
    for m in row['modes']:
        text+=f"| {row['case']} | {row['opponent']} | {m['bundle']} | {m['cash_gain']:+.2f} | {m['margin_gain']:+.2f} | {m['fed_fraction']:.4f} | {m['moves']:.2f} |\n"
text+='\n'+report['next']+'\n';(RUN/'RESULTS.md').write_text(text)
print('480 games,',controls,'exact old controls.')
for row in rows:
    print(row['case'],row['opponent'],[(m['bundle'],m['cash_gain'],m['margin_gain'],round(m['fed_fraction'],3),round(m['moves'])) for m in row['modes']])
