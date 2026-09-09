"""Report paired opening discovery and retain all opponent regressions."""
from pathlib import Path
from statistics import mean
import json

RUN=Path(__file__).resolve().parent
out=RUN/'discovery'
assert json.loads((out/'EXECUTION.json').read_text())['sources_unchanged']
protocol=json.loads((out/'PROTOCOL.json').read_text())
opponents=list(dict.fromkeys(b for a,b in protocol['jobs']))
def load(a,b):return json.loads((out/f'{a}_vs_{b}.json').read_text())['games']
def metrics(games):
    assert len(games)==128 and all(g['turns']==719 for g in games)
    w=sum(g['cash']>g['opponent_cash'] for g in games);t=sum(g['cash']==g['opponent_cash'] for g in games)
    return {'wtl':[w,t,128-w-t],'utility':(w+.5*t)/128,
        'cash':mean(g['cash'] for g in games),'margin':mean(g['cash']-g['opponent_cash'] for g in games)}
rows=[];groups={}
for q in (13,20,24):
    name=f'opening_funding_q{q}';deltas=[]
    for opponent in opponents:
        a,b=load(name,opponent),load('empty_sale_slots_m2',opponent)
        assert [(g['seed'],g['seat']) for g in a]==[(g['seed'],g['seat']) for g in b]
        am,bm=metrics(a),metrics(b)
        row={'candidate':name,'opponent':opponent,'candidate_metrics':am,'parent_metrics':bm,
            'utility_gain_pp':100*(am['utility']-bm['utility']),'margin_gain':am['margin']-bm['margin'],
            'own_cash_gain':am['cash']-bm['cash'],'full_records_equal':sum(x==y for x,y in zip(a,b))}
        rows.append(row);deltas.append(row)
    groups[name]={'all11_utility_gain_pp':mean(d['utility_gain_pp'] for d in deltas),
        'all11_margin_gain':mean(d['margin_gain'] for d in deltas),
        'utility_regressions':[d['opponent'] for d in deltas if d['utility_gain_pp']<0],
        'margin_regressions':[d['opponent'] for d in deltas if d['margin_gain']<0]}
report={'games':5632,'exact_control_games':16,'operational_games':48,'frozen_dependencies':476,
    'rows':rows,'groups':groups,'verdict':'No promotion. All variants improve the new counter but regress King; q13/q20 also regress Bohann. Refine funding robustness or reject after broader evidence.'}
(RUN/'RESULTS.json').write_text(json.dumps(report,indent=2)+'\n')
body='''# Opening funding discovery

Four complete C++packages pass48operational games. The unchangedq32control
matches16saved current-reference games in every field.5632paired discovery
games cover64exposed seeds/both seats/11opponents with476unchanged frozen
dependencies. Only step0wheat round-trip quantities change in each candidate.

| Opponent | Current W/T/L | q13 W/T/L | q20 W/T/L | q24 W/T/L |
| --- | ---: | ---: | ---: | ---: |
'''
for opponent in opponents:
    selected=[r for r in rows if r['opponent']==opponent]
    values=[selected[0]['parent_metrics']['wtl'],*[r['candidate_metrics']['wtl'] for r in selected]]
    body+=f"| {opponent} | "+' | '.join('/'.join(map(str,v)) for v in values)+' |\n'
body+='''
All three changes turn the new Yusuke matchup from0/128wins to113/128. q13/q20
also beat the current parent128/128; q24wins118/128. Several old opponent
records remain exactly equal, while King regresses. q24 preserves all128wins
againstBohann and increases mean margin sharply; q13/q20lose six wins there.
The first-turn market interaction creates large nonlinear funding effects.
Do not interpret every direct win as extra production or general strength.

No candidate is promoted. Next trace the exact first missed hire/purchase and
schedule guard, and search nearby quantities or a later observation-driven
financing repair. Recheck King,Bohann,Yusuke and broad strong opponents before
fresh promotion work. Initial choices cannot identify an unobserved opponent.
Full owncash/margin contrasts and regressions are in RESULTS.json.

Animal-group policies remain a separate active priority. Their cheap evaluator
correctly reverses cow selection in a newly tested market context, and both
sheepberryleaves plus cowweedcases have complete action/endpoint audits.
'''
(RUN/'RESULTS.md').write_text(body)
print(json.dumps(groups,indent=2))
