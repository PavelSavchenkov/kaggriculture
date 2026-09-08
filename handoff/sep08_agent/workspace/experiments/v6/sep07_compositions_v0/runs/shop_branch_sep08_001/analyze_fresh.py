from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import numpy as np

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]
protocol = json.loads((RUN/'FRESH_PROTOCOL.json').read_text())
assert len(json.loads((RUN/'FRESH_COMMANDS.json').read_text())) == 54
inputs = json.loads((RUN/'FRESH_INPUTS.json').read_text())
for path,value in inputs.items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest() == value,path
candidate = protocol['candidate']
references = [protocol['baseline'],protocol['current_cold']]
metric = lambda g:np.array([float(g['cash']>g['opponent_cash'])+.5*float(g['cash']==g['opponent_cash']),g['cash']-g['opponent_cash'],g['cash']])
rng = np.random.default_rng(2600711)
samples = rng.integers(0,128,size=(5000,128))
results = []
for reference in references:
    rows = []
    clusters = []
    native_deltas = []
    for native in range(2):
        folder = RUN/('native' if native else 'fresh')
        seeds = 16 if native else 128
        for opponent in protocol['opponents']:
            get = lambda name:json.loads((folder/f'{name}_vs_{opponent}.json').read_text())['games']
            games,old = get(candidate),get(reference)
            assert len(games) == len(old) == seeds*2
            assert [(g['seed'],g['seat']) for g in games] == [(g['seed'],g['seat']) for g in old]
            a = np.array([metric(g) for g in games])
            b = np.array([metric(g) for g in old])
            delta = a-b
            rows.append({'opponent':opponent,'native':bool(native),'games':len(games),
                'wtl':[sum((g['cash']>g['opponent_cash'],g['cash']==g['opponent_cash'],g['cash']<g['opponent_cash'])[i] for g in games) for i in range(3)],
                'metrics':a.mean(axis=0).tolist(),'gain':delta.mean(axis=0).tolist()})
            if opponent != 'pass':
                (native_deltas if native else clusters).append(delta.reshape(seeds,2,3).mean(axis=1))
    changes = np.mean(clusters,axis=0)
    intervals = np.quantile(changes[samples].mean(axis=1),[.025,.975],axis=0).T
    native_gain = np.mean(native_deltas,axis=(0,1))
    direct = next(r for r in rows if not r['native'] and r['opponent'] == reference)
    active = [r for r in rows if not r['native'] and r['opponent'] != 'pass']
    gates = {'direct_reference_utility_margin':direct['metrics'][0]>=.5 and direct['metrics'][1]>0,
        'active_utility_positive_95pct':bool(intervals[0,0]>0),
        'active_margin_positive_95pct':bool(intervals[1,0]>0),
        'each_active_opponent_bounded_regression':all(r['gain'][0]>=-.02 and r['gain'][1]>=-500 for r in active),
        'all_PASS_won':all(r['wtl']==[r['games'],0,0] for r in rows if r['opponent']=='pass'),
        'native_mean_bounded_regression':bool(native_gain[0]>=-.02 and native_gain[1]>=-500),
        'operational_parity_frozen_sources':True}
    results.append({'reference':reference,'rows':rows,'active_gain':changes.mean(axis=0).tolist(),
        'active_gain_95pct':intervals.tolist(),'native_active_gain':native_gain.tolist(),
        'gates':gates,'passes':all(gates.values())})
selected = candidate if results[1]['passes'] else protocol['current_cold']
report = {'completed_utc':datetime.now(timezone.utc).isoformat(),'games':7776,
    'metric_order':['utility','margin','cash'],'results':results,'selected_cold':selected,
    'source_dependencies':len(inputs),'global_reference_unchanged':True,
    'scope':'Separate causal test of the faithful fitted shop rule and comparison with the improved cold reference. No mainline promotion.'}
(RUN/'FRESH_ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
(RUN/'SELECTION.json').write_text(json.dumps({'selected':selected,'candidate':candidate,
    'old_pair_gates_pass':results[0]['passes'],'current_cold_gates_pass':results[1]['passes'],
    'global_reference_unchanged':True},indent=2)+'\n')
text = '# Fresh shop-selector result\n\n7,776 full games, fixed rule and source dependencies unchanged.\n'
for result in results:
    text += f"\nVersus {result['reference']}: gains (utility,margin,cash)={result['active_gain']}, 95% intervals={result['active_gain_95pct']}.\n\n| Opponent | W/T/L | Margin | Utility gain | Margin gain |\n|---|---:|---:|---:|---:|\n"
    for row in result['rows']:
        if not row['native']:
            text += f"| {row['opponent']} | {row['wtl']} | {row['metrics'][1]:.2f} | {100*row['gain'][0]:+.3f}pp | {row['gain'][1]:+.2f} |\n"
    text += '\nGates: '+json.dumps(result['gates'])+'\n'
text += f'\nRetained cold reference: {selected}. Mainline reference unchanged.\n'
(RUN/'FRESH_RESULTS.md').write_text(text)
print(text,flush=True)
