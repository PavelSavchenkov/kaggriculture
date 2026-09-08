"""Separate direct counter strength from the neutral common-opponent panel."""
from pathlib import Path
import json
import numpy as np

RUN=Path(__file__).resolve().parent
out=RUN/'fresh_2330000'
assert json.loads((out/'EXECUTION.json').read_text())['sources_unchanged']
variants=['yusuke_sep08_m1','yusuke_sep08_m2','yusuke_sep08_m3','empty_sale_slots_m2']
opponents=['empty_sale_slots_m2','teammate_shoprouter','public_router','public_router_v52','ahmed_v23','junghoon_wool_sales','king_rc4']
cash=np.zeros((4,7,512,2,2));rows=[]
for i,a in enumerate(variants):
    for j,b in enumerate(opponents):
        data=json.loads((out/f'{a}_vs_{b}.json').read_text());games=data['games'];assert len(games)==1024
        assert {(g['seed'],g['seat']) for g in games}=={(s,p) for s in range(2330000,2330512) for p in range(2)}
        for g in games:
            assert g['turns']==719
            cash[i,j,g['seed']-2330000,g['seat']]=[g['cash'],g['opponent_cash']]
        wins=sum(g['cash']>g['opponent_cash'] for g in games);ties=sum(g['cash']==g['opponent_cash'] for g in games)
        rows.append({'agent':a,'opponent':b,'wtl':[wins,ties,1024-wins-ties],
            'cash':float(cash[i,j,:,:,0].mean()),'margin':float((cash[i,j,:,:,0]-cash[i,j,:,:,1]).mean())})
margin=cash[:,:,:,:,0]-cash[:,:,:,:,1]
utility=(margin>0)+.5*(margin==0)
rng=np.random.default_rng(83017);samples=rng.integers(0,512,size=(4000,512))
groups={}
for label,opps in [('all_seven',slice(None)),('six_neutral_opponents',slice(1,None))]:
    metrics={};contrasts={}
    for i,a in enumerate(variants):
        seed_utility=utility[i,opps].mean(axis=(0,2));seed_margin=margin[i,opps].mean(axis=(0,2))
        metrics[a]={'utility':float(seed_utility.mean()),'margin':float(seed_margin.mean()),'own_cash':float(cash[i,opps,:,:,0].mean())}
        if i<3:
            du=seed_utility-utility[3,opps].mean(axis=(0,2));dm=seed_margin-margin[3,opps].mean(axis=(0,2))
            contrasts[a]={'utility_gain_pp':float(100*du.mean()),
                'utility_gain_95pct_pp':(100*np.quantile(du[samples].mean(axis=1),[.025,.975])).tolist(),
                'margin_gain':float(dm.mean()),'margin_gain_95pct':np.quantile(dm[samples].mean(axis=1),[.025,.975]).tolist()}
    groups[label]={'metrics':metrics,'contrasts_vs_current':contrasts}
report={'games':28672,'rows':rows,'groups':groups,
    'verdict':'Confirmed strong counter; retain as league opponent, do not replace the current reference. Current wins more broadly against the six neutral opponents.',
    'scope':'512fresh seeds/both seats with paired controls; seed-cluster bootstrap. Seven-opponent discovery confirmation, not full population promotion.',
    'new_local_amendment':'Preserving tape1 at day27 underperforms faithful source on this confirmation; not selected.'}
(RUN/'FRESH_RESULTS.json').write_text(json.dumps(report,indent=2)+'\n')
body='''# Fresh Yusuke confirmation

28672complete games;512fresh seeds/both seats; four agents against seven
matching opponents. All271frozen input dependencies remain unchanged.

| Opponent | Yusuke full W/T/L | Current W/T/L | Yusuke margin | Current margin |
| --- | ---: | ---: | ---: | ---: |
'''
for b in opponents:
    a=next(r for r in rows if r['agent']=='yusuke_sep08_m2' and r['opponent']==b)
    p=next(r for r in rows if r['agent']=='empty_sale_slots_m2' and r['opponent']==b)
    body+=f"| {b} | {'/'.join(map(str,a['wtl']))} | {'/'.join(map(str,p['wtl']))} | {a['margin']:+.2f} | {p['margin']:+.2f} |\n"
neutral=groups['six_neutral_opponents'];new=neutral['metrics']['yusuke_sep08_m2'];parent=neutral['metrics']['empty_sale_slots_m2'];delta=neutral['contrasts_vs_current']['yusuke_sep08_m2']
body+=f'''
Yusuke wins1024/1024direct games against the current reference, mean+7598.79.
However, against the six neutral opponents, current utility is
{100*parent['utility']:.3f}% versus {100*new['utility']:.3f}% for Yusuke.
The paired Yusuke-minus-current gain is {delta['utility_gain_pp']:.3f}pp,
95%seed-cluster interval {delta['utility_gain_95pct_pp'][0]:.3f}to{delta['utility_gain_95pct_pp'][1]:.3f}pp.
Mean margin gain is {delta['margin_gain']:+.2f}.

The seven-opponent average includes current self-play and therefore gives the
new counter a large structural advantage on that row. Report both groups;
do not use that average alone to promote a replacement. Keep current reference,
add this strong public opponent, and improve the exposed matchup with broad
regression checks. Yusuke's early-only and local guarded variants also win
every direct game but do not solve its broader deficits. The local guard is
not selected over the faithful source.

Same-game gap_diagnostic identifies production and execution differences.
The offline opening witness in ../animal_group_policy_sep08_001 isolates a
first-turn price/funding effect. Copying both market turns is harmful; changing
only the first round-trip quantity helps in one exposed world. New complete
strongest-parent variants are in ../opening_funding_sep08_001 and still require
operational and broad league checks. No promotion or external submission.
'''
(RUN/'FRESH_RESULTS.md').write_text(body)
print(json.dumps(neutral,indent=2))
