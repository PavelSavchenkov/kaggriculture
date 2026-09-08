"""Compare V24 with V23 and the same transform on our strongest parent."""
from pathlib import Path
from statistics import mean
import json

RUN=Path(__file__).resolve().parent
out=RUN/'discovery'
execution=json.loads((out/'EXECUTION.json').read_text());assert execution['sources_unchanged']
protocol=json.loads((out/'PROTOCOL.json').read_text())
opponents=list(dict.fromkeys(b for a,b in protocol['jobs']))
agents=list(dict.fromkeys(a for a,b in protocol['jobs']))
games={(a,b):json.loads((out/f'{a}_vs_{b}.json').read_text())['games'] for a,b in protocol['jobs']}
def metrics(games):
    assert len(games)==128 and all(g['turns']==719 for g in games)
    w=sum(g['cash']>g['opponent_cash'] for g in games);t=sum(g['cash']==g['opponent_cash'] for g in games)
    return {'wtl':[w,t,128-w-t],'utility':(w+.5*t)/128,
        'cash':mean(g['cash'] for g in games),'margin':mean(g['cash']-g['opponent_cash'] for g in games)}
rows=[];groups={}
for candidate,parent in [('premium_sales_s144','empty_sale_slots_m2'),('premium_sales_s216','empty_sale_slots_m2'),('ahmed_v24','ahmed_v23')]:
    for b in opponents:
        a,c=games[candidate,b],games[parent,b]
        assert [(g['seed'],g['seat']) for g in a]==[(g['seed'],g['seat']) for g in c]
        am,bm=metrics(a),metrics(c)
        rows.append({'candidate':candidate,'parent':parent,'opponent':b,'candidate_metrics':am,'parent_metrics':bm,
            'utility_gain_pp':100*(am['utility']-bm['utility']),'margin_gain':am['margin']-bm['margin'],
            'own_cash_gain':am['cash']-bm['cash'],'full_records_equal':sum(x==y for x,y in zip(a,c)),
            'production_equal':sum(x['produced']==y['produced'] for x,y in zip(a,c))})
    neutral=[b for b in opponents if b not in (candidate,parent,'pass','investment_context_guarded_001_best','empty_sale_slots_m2')]
    rr=[r for r in rows if r['candidate']==candidate and r['opponent'] in neutral]
    groups[candidate]={'neutral_opponents':neutral,'neutral_gain_pp':mean(r['utility_gain_pp'] for r in rr),
        'neutral_margin_gain':mean(r['margin_gain'] for r in rr),
        'utility_regressions':[r['opponent'] for r in rows if r['candidate']==candidate and r['utility_gain_pp']<0],
        'margin_regressions':[r['opponent'] for r in rows if r['candidate']==candidate and r['margin_gain']<0]}
report={'games':execution['games'],'source_parity_cases':4096,'operational_games':48,'rows':rows,'groups':groups,
    'scope':'Paired exposed discovery, no promotion. Neutral group excludes candidate/parent self-play, our current reference, submitted ancestor and PASS; group membership differs for public versus mainline comparisons.'}
(RUN/'RESULTS.json').write_text(json.dumps(report,indent=2)+'\n')
body='''# Premium sale order discovery

7040complete games,64exposed seeds/both seats/11opponents, five compared agents.
4096original-source transform cases and48operational games pass. Sixteen current
control full records equal prior fresh results.386frozen dependencies unchanged.
V24 differs from the verifiedV23 in only its final market-order wrapper; all
other36top-levelASTnodes are identical. No promotion from this discovery alone.

| Opponent | Current | Current+144 | Current+216 | AhmedV23 | AhmedV24 |
| --- | ---: | ---: | ---: | ---: | ---: |
'''
for b in opponents:
    values=[metrics(games[a,b])['wtl'] for a in ['empty_sale_slots_m2','premium_sales_s144','premium_sales_s216','ahmed_v23','ahmed_v24']]
    body+=f'| {b} | '+' | '.join('/'.join(map(str,v)) for v in values)+' |\n'
body+='\nW/T/L includes ties. Paired neutral groups exclude direct parent/candidate self-play and PASS.\n\n'
for a,g in groups.items():body+=f"- {a}: paired neutral utility gain{g['neutral_gain_pp']:+.3f}pp; mean margin gain{g['neutral_margin_gain']:+.2f}. Win regressions: {', '.join(g['utility_regressions']) or 'none'}.\n"
body+='\nFull source lineage, quantities/production comparisons, regressions and exact commands are retained in IMPORT.json, RESULTS.json and discovery/.\n'
(RUN/'RESULTS.md').write_text(body)
print(json.dumps(groups,indent=2))
