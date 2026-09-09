"""Analyze frozen paired cow-service execution and selection experiments."""
from pathlib import Path
import hashlib
import json
import numpy as np

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]
OUT = RUN / 'discovery_2390000'
protocol = json.loads((OUT / 'PROTOCOL.json').read_text())
execution = json.loads((OUT / 'EXECUTION.json').read_text())
assert execution['sources_unchanged'] and execution['games'] == protocol['games']
frozen = json.loads((OUT / 'freeze/FROZEN.json').read_text())['files_sha256']
assert all(hashlib.sha256((ROOT / p).read_bytes()).hexdigest() == h for p, h in frozen.items())
parent, fixed, repriced = protocol['agents']
pairs = [(fixed, parent), (repriced, parent), (repriced, fixed)]
rows, groups, missed = [], [], []

def features(games):
    return np.array([[float(g['cash'] > g['opponent_cash']) + .5*(g['cash'] == g['opponent_cash']),
        g['cash'], g['cash']-g['opponent_cash']] for g in games])

for native in (False, True):
    prefix = 'native_' if native else ''
    panel = 'native' if native else 'fresh'
    all_data = {}
    for opponent in protocol['opponents']:
        games, diagnostics = {}, {}
        for agent in protocol['agents']:
            path = OUT / f'{prefix}{agent}_vs_{opponent}.json'
            games[agent] = json.loads(path.read_text())['games']
            diagnostics[agent] = json.loads(path.with_suffix('.json.diagnostics.json').read_text())['games']
            assert [(g['seed'],g['seat']) for g in games[agent]] == [(d['seed'],d['seat']) for d in diagnostics[agent]]
            for d in diagnostics[agent]:
                if d['missed_days']:
                    missed.append(dict(d, panel=panel, candidate=agent, opponent=opponent))
        for candidate, baseline in pairs:
            a, b = games[candidate], games[baseline]
            da, db = diagnostics[candidate], diagnostics[baseline]
            assert [(g['seed'],g['seat']) for g in a] == [(g['seed'],g['seat']) for g in b]
            x, y = features(a), features(b)
            difference = x-y
            all_data[candidate,baseline,opponent] = difference.reshape(-1,2,3).mean(axis=1)
            same_choices = [(x['family'],x['choice']) == (y['family'],y['choice']) for x,y in zip(da,db)]
            if candidate == fixed and baseline == parent:
                assert all(same_choices), 'Fixed-values control changed an investment choice.'
            tail = (len(a)+9)//10
            rows.append({'panel':panel,'candidate':candidate,'parent':baseline,'opponent':opponent,
                'games':len(a),'win_score':float(x[:,0].mean()),'parent_win_score':float(y[:,0].mean()),
                'gain_win_pp':float(difference[:,0].mean()*100),'gain_cash':float(difference[:,1].mean()),
                'gain_margin':float(difference[:,2].mean()),
                'gain_cash_tail':float(np.sort(x[:,1])[:tail].mean()-np.sort(y[:,1])[:tail].mean()),
                'gain_margin_tail':float(np.sort(x[:,2])[:tail].mean()-np.sort(y[:,2])[:tail].mean()),
                'changed_choices':sum(not same for same in same_choices),
                'cow3':sum(d['family']==0 and d['choice']==2 for d in da),
                'parent_cow3':sum(d['family']==0 and d['choice']==2 for d in db),
                'gain_hire_cost':float(np.mean([g['profile']['hire_cost']-h['profile']['hire_cost'] for g,h in zip(a,b)])),
                'gain_faults':float(np.mean([g['unit_faults']-h['unit_faults'] for g,h in zip(a,b)])),
                'produced_gain':np.mean([np.array(g['produced'])-h['produced'] for g,h in zip(a,b)],axis=0).tolist()})
        
    for candidate, baseline in pairs:
        values = np.stack([all_data[candidate,baseline,opponent] for opponent in protocol['primary']]).mean(axis=0)
        rng = np.random.default_rng(2390000)
        sample = values[rng.integers(0,len(values),size=(4000,len(values)))].mean(axis=1)
        bounds = np.quantile(sample,[.025,.975],axis=0)
        groups.append({'panel':panel,'candidate':candidate,'parent':baseline,
            'gain_win_pp':float(values[:,0].mean()*100),'gain_cash':float(values[:,1].mean()),
            'gain_margin':float(values[:,2].mean()),'win_gain_95pct_pp':(bounds[:,0]*100).tolist(),
            'margin_gain_95pct':bounds[:,2].tolist(),
            'meets_numeric_discovery_advance_rule':bool(values[:,0].mean()>=0 and bounds[0,2]>0)})
report = {'games':protocol['games'],'rows':rows,'groups':groups,'missed_guards':missed,
    'scope':'Discovery evidence only. Frozen candidate/parent source hashes verified. Final guard and per-opponent review required; no promotion.'}
(RUN / 'RESULTS.json').write_text(json.dumps(report,indent=2)+'\n')
body = '# Cow-course execution and repricing discovery\n\n13,824 games; all frozen inputs unchanged. No promotion.\n\n'
body += '| Panel | Candidate / parent | Win gain pp | Mean margin gain | Margin 95% interval |\n| --- | --- | ---: | ---: | --- |\n'
for g in groups:
    body += f"| {g['panel']} | {g['candidate']} / {g['parent']} | {g['gain_win_pp']:+.3f} | {g['gain_margin']:+.2f} | {g['margin_gain_95pct']} |\n"
body += '\nFull per-opponent choices, hires, output, cash, tails and guard masks are in RESULTS.json.\n'
(RUN / 'RESULTS.md').write_text(body)
print(json.dumps({'groups':groups,'missed_guards':len(missed)},indent=2))
