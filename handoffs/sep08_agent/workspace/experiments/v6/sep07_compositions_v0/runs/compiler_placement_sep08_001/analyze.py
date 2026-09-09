from datetime import datetime, timezone
from pathlib import Path
import json
import statistics

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
rows=[];parity=0;unchanged=0
for case in ['mixed','goose','dairy','wool']:
    for opp in ['public_router','observed_sale_lead_start_216']:
        pair=[json.loads((RUN/f'discovery/compiler_placement_{case}_m{mode}_vs_{opp}.json').read_text()) for mode in [0,1]]
        if case in ['mixed','goose']:
            mode=1 if case=='mixed' else 3
            old=json.loads((EXP/f'runs/compiler_labor_sep08_001/discovery/compiler_labor_{case}_m{mode}_vs_{opp}.json').read_text())
            assert old['games']==pair[0]['games'];parity+=len(old['games'])
        else:
            assert pair[0]['games']==pair[1]['games'];unchanged+=len(pair[0]['games'])
        for mode,d in enumerate(pair):
            gs=d['games'];mean=lambda f:statistics.mean(f(g) for g in gs)
            animals={}
            for item in [9,10,11]:
                ls=[l for g in gs for l in g['profile']['lives'] if l[0]==item]
                animals[str(item)]={'count':len(ls),'born_days':sorted(set(l[5] for l in ls)),
                    'mean_born_day':statistics.mean(l[5] for l in ls) if ls else None,
                    'animal_days':sum(l[6].bit_count() for l in ls)/len(gs)}
            rows.append({'case':case,'mode':mode,'opponent':opp,'games':len(gs),'utility':d['win_utility'],
                'cash':mean(lambda g:g['cash']),'margin':mean(lambda g:g['cash']-g['opponent_cash']),
                'hires':mean(lambda g:g['profile']['hires']),'hire_cost':mean(lambda g:g['profile']['hire_cost']),
                'land_cost':mean(lambda g:g['profile']['land_cost']),
                'moves':mean(lambda g:sum(g['profile']['successful'][1:5])),
                'produced':[mean(lambda g,i=i:g['produced'][i]) for i in range(9)],'animals':animals})
traces=[[json.loads(x) for x in (RUN/f'traces/mode{mode}.jsonl').read_text().splitlines()] for mode in [0,1]]
land_changes=[[(a['step'],a['quadrants'],b['quadrants']) for a,b in zip(t,t[1:]) if a['quadrants']!=b['quadrants']] for t in traces]
assert land_changes==[[(0,1,2)],[(168,1,2)]]
report={'created_utc':datetime.now(timezone.utc).isoformat(),'games':256,'mode0_full_records_exact':parity,
    'unchanged_dairy_wool_records':unchanged,'trace_full_records_exact':2,'rows':rows,
    'mixed_funding_witness':{'seed':1000,'seat':0,'land_changes':land_changes,'initial_turns':[t[:3] for t in traces]},
    'findings':['Owned-land-first moves both mixed-farm sheep fromday8 today0 in all32 discovery games. Both cows remain day0.',
        'Mixed-farm total land cost remains$1000 but moves fromstep0 tostep168 in the exact witness. Initial land saving pays for two$500sheep.',
        'Across16 public-router games, wool rises50->65.75 and fertilizer100->116, while wheat falls140->128. Own cash+$1660.375; mean margin+$4682.625.',
        'Goose avoids$1000land cost but walks farther and produces fewer eggs/fertilizer; its positive cash gain is cost saving, not production growth.',
        'Dairy and wool placements are unchanged; all64 complete paired records equal.',
        'No compiler variant wins or approaches the strongest league. This is useful placement/finance evidence, not a strong-agent promotion.'],
    'next':'Make placement explicit in cheap estimates and generate complete day tasks for difficult birth/service days. Keep observed cash and placement errors separate from biological value.'}
assert json.loads((RUN/'TRACE_CHECKS.json').read_text())['complete_records_exact']==2
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text='# Placement and funding results\n\n'+'\n\n'.join(report['findings'])+'\n\n'
text+='| Case | Mode | Opponent | Cash | Margin | Land cost | Hires cost | Moves |\n| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |\n'
for r in rows:text+=f"| {r['case']} | {r['mode']} | {r['opponent']} | {r['cash']:.2f} | {r['margin']:.2f} | {r['land_cost']:.2f} | {r['hire_cost']:.2f} | {r['moves']:.2f} |\n"
text+='\n'+report['next']+'\n'
(RUN/'RESULTS.md').write_text(text)
print('Placement:256 games,',parity,'exact controls,',unchanged,'unchanged dairy/wool pairs, two exact trace witnesses.')
