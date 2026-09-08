from datetime import datetime, timezone
from pathlib import Path
import json
import numpy as np

RUN = Path(__file__).resolve().parent
assert json.loads((RUN/'CONTROL_PARITY.json').read_text())['all_complete_records_equal']
opponents = ['empty_sale_slots_m2','teammate_shoprouter','public_router','public_router_v52',
    'ahmed_v23','junghoon_78','john_131','king_rc4','pass']
metric = lambda g:np.array([float(g['cash']>g['opponent_cash'])+.5*float(g['cash']==g['opponent_cash']),g['cash']-g['opponent_cash'],g['cash']])
rows = []
contexts = []
for opponent in opponents:
    get = lambda mode:json.loads((RUN/'discovery'/f'crop_context_m{mode}_vs_{opponent}.json').read_text())['games']
    games = [get(mode) for mode in range(3)]
    baseline = np.array([metric(g) for g in games[0]])
    for mode in [1,2]:
        actual = np.array([metric(g) for g in games[mode]])
        delta = actual-baseline
        rows.append({'opponent':opponent,'mode':mode,'games':128,
            'wtl':[int(sum((g['cash']>g['opponent_cash'],g['cash']==g['opponent_cash'],g['cash']<g['opponent_cash'])[i] for g in games[mode])) for i in range(3)],
            'gain':delta.mean(axis=0).tolist(), 'positive_margin_games':int(sum(delta[:,1]>0)),
            'negative_margin_games':int(sum(delta[:,1]<0)),
            'production_gain':np.mean([np.array(a['produced'][:9])-b['produced'][:9] for a,b in zip(games[mode],games[0])],axis=0).tolist(),
            'hire_cost_gain':float(np.mean([a['profile']['hire_cost']-b['profile']['hire_cost'] for a,b in zip(games[mode],games[0])]))})
    for i,old in enumerate(games[0]):
        contexts.append({'opponent':opponent,'seed':old['seed'],'seat':old['seat'],
            'seen_shops':old['shops'][:4],
            'tomato_shop_count':sum(s in [2,5] for s in old['shops'][:4]),
            'metrics':[metric(g[i]).tolist() for g in games]})
active = [r for r in contexts if r['opponent']!='pass']
upper = np.mean([np.max([np.array(x)-r['metrics'][0] for x in r['metrics']],axis=0) for r in active],axis=0)
report = {'completed_utc':datetime.now(timezone.utc).isoformat(),'games':4608,
    'control_complete_records_equal':1152,'metric_order':['utility','margin','cash'],
    'rows':rows,'contexts':contexts,'ex_post_componentwise_best_gain':upper.tolist(),
    'scope':'Exposed discovery; selecting a different winner for each realized game and metric is only headroom, not an executable policy. Future shop outcomes are not agent features.'}
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text = '# Mainline crop-choice discovery\n\n4,608 full games;1,152 original-control records exactly equal. Forced choices also update day13 eligibility from the actual crop branch.\n\n| Opponent | Mode | W/T/L | Utility gain | Margin gain | Own cash gain |\n|---|---:|---:|---:|---:|---:|\n'
for row in rows:
    text += f"| {row['opponent']} | {row['mode']} | {row['wtl']} | {100*row['gain'][0]:+.3f}pp | {row['gain'][1]:+.2f} | {row['gain'][2]:+.2f} |\n"
text += f'\nEx-post componentwise best gain across active opponents: {upper.tolist()}. This is a hindsight diagnostic, not a validated selector. Mode1 forces wheat;mode2 requests tomatoes subject to existing entry checks.\n'
(RUN/'RESULTS.md').write_text(text)
print(text,flush=True)
