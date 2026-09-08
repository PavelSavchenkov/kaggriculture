from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import numpy as np

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
protocol=json.loads((RUN/'FRESH_PROTOCOL.json').read_text())
baseline=protocol['baseline']
opponents=protocol['opponents']
candidates=protocol['candidates']
assert len(json.loads((RUN/'FRESH_COMMANDS.json').read_text()))==32
assert json.loads((RUN/'FINALIST_OPERATIONS.json').read_text())['generic_pair_debug_full_records_equal']
assert json.loads((RUN/'OPERATIONAL_CHECKS.json').read_text())['generic_pair_debug_thread_zero_budget_full_records_equal']
inputs=json.loads((RUN/'FRESH_INPUTS.json').read_text())
for path,value in inputs.items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==value,path
metric=lambda g:np.array([float(g['cash']>g['opponent_cash'])+.5*float(g['cash']==g['opponent_cash']),g['cash']-g['opponent_cash'],g['cash']],dtype=float)
results=[]
rng=np.random.default_rng(2500731)
resamples=rng.integers(0,128,size=(5000,128))
for name in candidates:
    rows=[];changes=[];candidate_values=[]
    for opponent in opponents:
        get=lambda n:json.loads((RUN/'fresh'/f'{n}_vs_{opponent}.json').read_text())['games']
        games,old=get(name),get(baseline)
        assert len(games)==len(old)==256
        assert [(g['seed'],g['seat']) for g in games]==[(g['seed'],g['seat']) for g in old]
        a=np.array([metric(g) for g in games]);b=np.array([metric(g) for g in old]);delta=a-b
        if opponent!='pass':changes.append(delta.reshape(128,2,3).mean(axis=1));candidate_values.append(a.mean(axis=0))
        row={'opponent':opponent,'games':256,
            'wtl':[sum((g['cash']>g['opponent_cash'],g['cash']==g['opponent_cash'],g['cash']<g['opponent_cash'])[i] for g in games) for i in range(3)],
            'utility':float(a[:,0].mean()),'mean_margin':float(a[:,1].mean()),'mean_cash':float(a[:,2].mean()),
            'utility_gain':float(delta[:,0].mean()),'margin_gain':float(delta[:,1].mean()),'cash_gain':float(delta[:,2].mean()),
            'fault_gain':float(np.mean([g['unit_faults']-o['unit_faults'] for g,o in zip(games,old)])),
            'hire_cost_gain':float(np.mean([g['profile']['hire_cost']-o['profile']['hire_cost'] for g,o in zip(games,old)])),
            'production_gain':np.mean([np.array(g['produced'][:9])-o['produced'][:9] for g,o in zip(games,old)],axis=0).tolist(),
            'discard_gain':np.mean([np.array(g['discarded'][:9])-o['discarded'][:9] for g,o in zip(games,old)],axis=0).tolist()}
        rows.append(row)
    clusters=np.mean(changes,axis=0)
    boot=clusters[resamples].mean(axis=1)
    intervals=np.quantile(boot,[.025,.975],axis=0).T.tolist()
    direct=next(r for r in rows if r['opponent']==baseline)
    active=[r for r in rows if r['opponent']!='pass']
    gates={'direct_parent_utility_and_margin':direct['utility']>=.5 and direct['mean_margin']>0,
        'active_field_utility_positive_95pct':intervals[0][0]>0,
        'active_field_margin_positive_95pct':intervals[1][0]>0,
        'each_active_opponent_bounded_regression':all(r['margin_gain']>=-500 and r['utility_gain']>=-.02 for r in active),
        'all_PASS_above_PASS':next(r for r in rows if r['opponent']=='pass')['wtl']==[256,0,0],
        'operational_and_frozen_sources':True}
    results.append({'candidate':name,'rows':rows,'active_mean_gain':clusters.mean(axis=0).tolist(),
        'active_gain_95pct':intervals,'active_candidate_metrics':np.mean(candidate_values,axis=0).tolist(),
        'metric_order':['utility','margin','cash'],'gates':gates,'eligible':all(gates.values())})
eligible=[r for r in results if r['eligible']]
selected=max(eligible,key=lambda r:r['active_candidate_metrics'][:2])['candidate'] if eligible else None
report={'completed_utc':datetime.now(timezone.utc).isoformat(),'games':8192,'results':results,'selected':selected,
    'unchanged_original_source_files':len(inputs),'bootstrap':{'resamples':5000,'seed':2500731,'cluster':'seed averaged across both seats and seven active opponents'},
    'scope':protocol['scope']}
(RUN/'FRESH_ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
selection={'selected':selected,'baseline':baseline,'scope':protocol['scope'],'global_reference_unchanged':True,
    'reason':'All fixed gates pass.' if selected else 'No candidate passes every fixed gate. Retain the previous cold reference and preserve these distinct composition alternatives as experimental league opponents.'}
(RUN/'SELECTION.json').write_text(json.dumps(selection,indent=2)+'\n')
text='# Fresh early-melon comparison\n\n8192 full games,128 unused seeds/both seats/eight opponents, unchanged original source hashes. Fixed gates are evaluated without alteration.\n'
for r in results:
    text+=f"\n{r['candidate']}: eligible={r['eligible']}; active-field gains (utility, margin, owncash)={r['active_mean_gain']};95%intervals={r['active_gain_95pct']}.\n\n| Opponent | W/T/L | Margin | Paired margin gain | Paired cash gain |\n| --- | ---: | ---: | ---: | ---: |\n"
    for row in r['rows']:text+=f"| {row['opponent']} | {row['wtl']} | {row['mean_margin']:.2f} | {row['margin_gain']:+.2f} | {row['cash_gain']:+.2f} |\n"
    text+='\nGates: '+json.dumps(r['gates'])+'\n'
text+=f"\nSelected cold continuation: {selected}. Global reference unchanged.\n"
(RUN/'FRESH_RESULTS.md').write_text(text)
print(text)
