from pathlib import Path
import json
import statistics

RUN=Path(__file__).resolve().parent
names=json.loads((RUN/'LINEAGE.json').read_text())['variants']
opponents=['empty_sale_slots_m2','public_router','joint_routes_p362_m0','service_bank_p362_m2']
estimates={f"cold_renewal_p{p['id']}":p for p in json.loads((RUN/'ESTIMATES.json').read_text())['proposals']}
rows=[]
for opponent in opponents:
    get=lambda n:json.loads((RUN/'discovery'/f'{n}_vs_{opponent}.json').read_text())['games']
    baseline=get('cold_renewal_p0')
    assert baseline==get('joint_routes_p362_m0')
    for name in names:
        games=get(name);assert len(games)==16
        assert [(g['seed'],g['seat']) for g in games]==[(g['seed'],g['seat']) for g in baseline]
        delta=lambda fn:statistics.mean(fn(a)-fn(b) for a,b in zip(games,baseline))
        row={'agent':name,'opponent':opponent,'games':16,
            'wtl':[sum((g['cash']>g['opponent_cash'],g['cash']==g['opponent_cash'],g['cash']<g['opponent_cash'])[i] for g in games) for i in range(3)],
            'cash':statistics.mean(g['cash'] for g in games),'margin':statistics.mean(g['cash']-g['opponent_cash'] for g in games),
            'cash_gain':delta(lambda g:g['cash']),'margin_gain':delta(lambda g:g['cash']-g['opponent_cash']),
            'hire_cost_gain':delta(lambda g:g['profile']['hire_cost']),'fault_gain':delta(lambda g:g['unit_faults']),
            'produced':[statistics.mean(g['produced'][i] for g in games) for i in range(9)],
            'production_gain':[delta(lambda g:g['produced'][i]) for i in range(9)]}
        rows.append(row)
summary=[]
for name in names:
    strong=[r for r in rows if r['agent']==name and r['opponent'] in opponents[:2]]
    p=estimates[name]
    summary.append({'agent':name,'estimated_cash':p['cash'],'estimated_margin':p['margin'],
        'estimated_produced':p['produced'],'cash_gain':statistics.mean(r['cash_gain'] for r in strong),
        'margin_gain':statistics.mean(r['margin_gain'] for r in strong),
        'mean_cash':statistics.mean(r['cash'] for r in strong),'mean_margin':statistics.mean(r['margin'] for r in strong),
        'actual_produced':[statistics.mean(r['produced'][i] for r in strong) for i in range(9)]})
factorial=[]
for opponent in opponents:
    lookup={r['agent']:r for r in rows if r['opponent']==opponent}
    for a,b,label in [('cold_renewal_p50','cold_renewal_p0','renewal on mixed herd'),
        ('cold_renewal_p98','cold_renewal_p1','renewal on cow replacement herd'),
        ('cold_renewal_p98','cold_renewal_p50','cow replacement with the same renewal'),
        ('cold_renewal_p1','cold_renewal_p0','cow replacement without renewal')]:
        factorial.append({'opponent':opponent,'candidate':a,'control':b,'effect':label,
            'cash_gain':lookup[a]['cash']-lookup[b]['cash'],'margin_gain':lookup[a]['margin']-lookup[b]['margin'],
            'production_gain':[x-y for x,y in zip(lookup[a]['produced'],lookup[b]['produced'])]})
report={'discovery_games':768,'exact_original_control_games':64,'rows':rows,'summary':summary,'factorial':factorial,
    'scope':'Exposed eight-seed/both-seat discovery. Cheap forecasts use16seeds/both-seats fixed rival flows against the old service program, so aggregate estimate/cash differences are not pure paired estimator errors.',
    'next':'Fresh comparison of p98 (cow replacement + tomato/carrot renewal), p157 (omit geese + wheat renewal +10hands), and retained service_bank_p362_m2 across a broader field. No global promotion.'}
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text='# Retired-tile renewal and herd search\n\n196 farms x64 exposed scenarios evaluated in0.456 seconds;11 finalists executed in768 games. All64 original-control games reproduce exactly. No global promotion.\n\n| Agent | Estimated cash | Exact cash vs two strong opponents | Cash gain over original compiler | Margin gain |\n| --- | ---: | ---: | ---: | ---: |\n'
for r in summary:
    text+=f"| {r['agent']} | {r['estimated_cash']:.2f} | {r['mean_cash']:.2f} | {r['cash_gain']:+.2f} | {r['margin_gain']:+.2f} |\n"
text+='\nEach exact gain compares the same seeds/opponents. The estimate column uses the earlier fixed-flow panel and is not a paired error measure. See ANALYSIS.json for production, labor and four controlled renewal/herd contrasts.\n'
(RUN/'RESULTS.md').write_text(text)
print(text)
