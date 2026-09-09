from datetime import datetime,timezone
from pathlib import Path
import json
import numpy as np

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
source=EXP/'runs/cold_renewal_sep08_001/fresh'
opponents=['empty_sale_slots_m2','teammate_shoprouter','public_router','public_router_v52','joint_routes_p362_m0','service_bank_p362_m2']
protocol={'created_utc':datetime.now(timezone.utc).isoformat(),'source':str(source.relative_to(EXP)),
    'baseline':'service_bank_p362_m2','candidate':'cold_renewal_p98','train_seeds':[2400000,2400063],'validation_seeds':[2400064,2400127],
    'features':'Counts among only first two observed shops: milk, yarn, egg, carrot shop instances.',
    'rules':'Single count>=1, >=2 or ==0; two predicates on different features combined by AND. Trigger16..48 of64training seeds. Include never-switch control.',
    'selection':'Largest training utility gain, then margin gain, then fewer conditions. Validation is evaluated only after rule choice.',
    'limitations':'Retrospective seed split of previously exposed audit data, not a fresh or blind strength claim. Standalone outcome mixing additionally requires the common legal prefix check.'}
with (RUN/'FIT_PROTOCOL.json').open('x') as f:json.dump(protocol,f,indent=2)
changes=[];shops=None
for opponent in opponents:
    get=lambda name:json.loads((source/f'{name}_vs_{opponent}.json').read_text())['games']
    a,b=get(protocol['candidate']),get(protocol['baseline'])
    assert [(g['seed'],g['seat']) for g in a]==[(g['seed'],g['seat']) for g in b]
    observed=np.array([g['shops'][:2] for g in a])
    assert observed.tolist()==[g['shops'][:2] for g in b]
    if shops is None:shops=observed
    else:assert np.array_equal(shops,observed)
    score=lambda g:np.array([float(g['cash']>g['opponent_cash'])+.5*float(g['cash']==g['opponent_cash']),g['cash']-g['opponent_cash'],g['cash']])
    changes.append(np.array([score(x)-score(y) for x,y in zip(a,b)]).reshape(128,2,3).mean(axis=1))
assert np.array_equal(shops[::2],shops[1::2])
shops=shops[::2]
features=np.array([[sum(s in ids for s in pair) for ids in [{3,5,6},{7},{0,1},{2,4}]] for pair in shops])
labels=['milk','yarn','egg','carrot']
predicates=[{'feature':i,'operator':op,'threshold':v} for i in range(4) for op,v in [('ge',1),('ge',2),('eq',0)]]
rules=[[]]+[[p] for p in predicates]+[[p,q] for i,p in enumerate(predicates) for q in predicates[i+1:] if p['feature']!=q['feature']]
deltas=np.mean(changes,axis=0)
training=[]
for rule in rules:
    mask=np.ones(128,dtype=bool) if rule else np.zeros(128,dtype=bool)
    for p in rule:
        v=features[:,p['feature']];mask &= v>=p['threshold'] if p['operator']=='ge' else v==p['threshold']
    n=int(mask[:64].sum())
    if rule and not 16<=n<=48:continue
    gain=(deltas[:64]*mask[:64,None]).mean(axis=0)
    training.append({'rule':rule,'train_trigger_seeds':n,'train_gain':gain.tolist(),'mask':mask.tolist()})
winner=max(training,key=lambda r:(r['train_gain'][0],r['train_gain'][1],-len(r['rule'])))
mask=np.array(winner['mask'])
validation=(deltas[64:]*mask[64:,None])
rng=np.random.default_rng(241611)
samples=rng.integers(0,64,size=(5000,64))
ci=np.quantile(validation[samples].mean(axis=1),[.025,.975],axis=0).T.tolist()
report={'protocol':protocol,'rules_screened':len(training),'selected_rule':winner['rule'],
    'feature_names':labels,'train_gain':winner['train_gain'],'validation_gain':validation.mean(axis=0).tolist(),
    'validation_gain_95pct':ci,'metric_order':['utility','margin','cash'],
    'train_trigger_seeds':winner['train_trigger_seeds'],'validation_trigger_seeds':int(mask[64:].sum()),
    'validation_per_opponent':{o:(d[64:]*mask[64:,None]).mean(axis=0).tolist() for o,d in zip(opponents,changes)},
    'validation_has_positive_utility_lower_bound':ci[0][0]>0}
(RUN/'TRAINING.json').write_text(json.dumps(training,indent=2)+'\n')
(RUN/'FIT_RESULT.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2),flush=True)
