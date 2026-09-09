"""Compare hiring changes, source controls and dated-composition fulfillment."""
from datetime import datetime, timezone
from pathlib import Path
import json
import statistics

RUN=Path(__file__).resolve().parent
expected={r['case']:r for r in json.loads((RUN/'controls_results/expected.json').read_text())}
rows=[]
parity=0


def summarize(path,case):
    d=json.loads(path.read_text())
    games=d['games']
    mean=lambda f:statistics.mean(f(g) for g in games)
    demanded=[[0]*12 for _ in range(30)]
    for item,start,end,x,y in expected[case]['lives']:
        for day in range(start//24,min(30,(end+23)//24)):
            demanded[day][item]+=1
    deficits=[]
    for g in games:
        realized=[[0]*12 for _ in range(30)]
        for life in g['profile']['lives']:
            for day in range(30):
                if life[6]&(1<<day):realized[day][life[0]]+=1
        deficits.append(sum(max(0,demanded[d][i]-realized[d][i]) for d in range(30) for i in range(12)))
    row={'case':case,'file':str(path.relative_to(RUN)),'agent':d['agent_a'],'opponent':d['agent_b'],
        'games':len(games),'utility':d['win_utility'],'cash':mean(lambda g:g['cash']),
        'margin':mean(lambda g:g['cash']-g['opponent_cash']),
        'hires':mean(lambda g:g['profile']['hires']),'hire_cost':mean(lambda g:g['profile']['hire_cost']),
        'unit_faults':mean(lambda g:g['unit_faults']),'weed_digs':mean(lambda g:g['profile']['weed_digs']),
        'moves':mean(lambda g:sum(g['profile']['successful'][1:5])),
        'produced':[mean(lambda g,i=i:g['produced'][i]) for i in range(9)],
        'missing_requested_item_days':statistics.mean(deficits),'animals':{}}
    for item,name in [(9,'goose'),(10,'cow'),(11,'sheep')]:
        def counts(g,field):return sum(l[field].bit_count() for l in g['profile']['lives'] if l[0]==item)
        days=sum(counts(g,6) for g in games)
        birthdays=[l[5] for g in games for l in g['profile']['lives'] if l[0]==item]
        row['animals'][name]={'days':days/len(games),'mean_born_day':statistics.mean(birthdays) if birthdays else None,
            'fed_rate':sum(counts(g,8) for g in games)/days if days else None,
            'cared_rate':sum(counts(g,9) for g in games)/days if days else None,
            'fertilizer_collection_rate':sum(counts(g,10) for g in games)/days if days else None}
    return row


for case in expected:
    original=json.loads((RUN/f'controls_results/original_{case}.json').read_text())
    copied=json.loads((RUN/f'discovery/compiler_labor_{case}_m0_vs_public_router.json').read_text())
    assert original['games']==copied['games'],case
    parity+=len(original['games'])
    for mode in range(4):
        for opp in ['public_router','observed_sale_lead_start_216']:
            rows.append(summarize(RUN/f'discovery/compiler_labor_{case}_m{mode}_vs_{opp}.json',case))
    if case.startswith('p'):
        for control in ['source_hiring','source_trace']:
            rows.append(summarize(RUN/f'controls_results/{control}_{case}.json',case))
report={'created_utc':datetime.now(timezone.utc).isoformat(),'copied_mode0_full_records_exact':parity,
        'discovery_games':512,'control_games':128,'rows':rows,'expected':expected,
        'conclusions':['Hiring-only changes improve some production and cash but none of the512 compiler discovery games is a win.',
            'The existing dated estimator can demand expensive daily workforce peaks. More output does not imply more profit.',
            'Source hiring calendars help but leave large gaps to source execution on identical composition families. Routing, input delivery and service fulfillment remain unresolved.',
            'Cold mixed requested sheep at day0 are generally placed much later. Higher worker spending can also delay a cow. Funding constraints must be diagnosed separately from service capacity.'],
        'limitations':['Missing requested item-days measures dated product counts, not exact tile identity or productive yield. Earlier intended harvesting can make it conservative or count useful early clearing as a deficit.',
            'All cases and seeds are exposed discovery scenarios. No strong-agent promotion, generalization or competitiveness claim.',
            'The requested cold starting farm can require more starting capital than available; missing births alone do not establish routing failure.']}
assert all(r['utility']==0 for r in rows if r['file'].startswith('discovery/'))
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
lines=['# Hiring-only compiler results','',f'Mode0 reproduces all {parity} complete parent records, including profiles. 512 discovery games and128 control games completed. No policy is promoted.','',
       '| Case | Policy | Opponent | Cash | Margin | Hires | Wages | Missing item-days |','| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |']
for r in rows:
    lines.append(f"| {r['case']} | {r['agent']} | {r['opponent']} | {r['cash']:.2f} | {r['margin']:.2f} | {r['hires']:.2f} | {r['hire_cost']:.2f} | {r['missing_requested_item_days']:.2f} |")
lines+=['']+report['conclusions']+['']+report['limitations']+['']
(RUN/'RESULTS.md').write_text('\n'.join(lines))
print('Complete',parity,'exact mode0 records;',len(rows),'comparison rows.')
for r in rows:
    if r['agent'].startswith('source_'):
        print(r['agent'],'cash',round(r['cash']),'missing itemdays',round(r['missing_requested_item_days']),r['animals'])
