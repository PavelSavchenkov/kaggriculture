"""Paired component effects and interactions in the frozen V25 discovery."""
from pathlib import Path
import csv
import hashlib
import json
import numpy as np

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]
OUT = RUN / 'discovery_2400000'
protocol = json.loads((OUT/'PROTOCOL.json').read_text())
execution = json.loads((OUT/'EXECUTION.json').read_text())
assert execution['sources_unchanged'] and execution['games'] == protocol['games']
frozen = json.loads((OUT/'freeze/FROZEN.json').read_text())['files_sha256']
assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest() == h for p,h in frozen.items())
agents, opponents = protocol['agents'], protocol['opponents']
data, summaries, rows = {}, [], []
for mask, agent in enumerate(agents):
    for opponent in opponents:
        games = json.loads((OUT/f'{agent}_vs_{opponent}.json').read_text())['games']
        assert [(g['seed'],g['seat']) for g in games] == [(s,p) for s in range(protocol['seed_start'],protocol['seed_start']+protocol['seeds']) for p in (0,1)]
        values = np.array([[float(g['cash']>g['opponent_cash'])+.5*(g['cash']==g['opponent_cash']),
            g['cash'],g['cash']-g['opponent_cash'],g['profile']['hire_cost'],g['unit_faults'],*g['produced']] for g in games])
        data[mask,opponent] = values
        summaries.append({'mask':mask,'opponent':opponent,'games':len(games),'win_score':float(values[:,0].mean()),
            'cash':float(values[:,1].mean()),'margin':float(values[:,2].mean())})

# Each bit is tested in every context of the other two bits, not only alone.
pairs = [(mask,mask^bit,f'bit_{bit}') for bit in (1,2,4) for mask in range(8) if mask&bit]
pairs.append((7,0,'all_components'))
groups = []
for candidate,parent,label in pairs:
    for opponent in opponents:
        a,b = data[candidate,opponent],data[parent,opponent]
        d = a-b
        tail = (len(a)+9)//10
        rows.append({'effect':label,'candidate_mask':candidate,'parent_mask':parent,'opponent':opponent,
            'win_gain_pp':float(d[:,0].mean()*100),'cash_gain':float(d[:,1].mean()),'margin_gain':float(d[:,2].mean()),
            'hire_cost_gain':float(d[:,3].mean()),'faults_gain':float(d[:,4].mean()),
            'cash_tail_gain':float(np.sort(a[:,1])[:tail].mean()-np.sort(b[:,1])[:tail].mean()),
            'margin_tail_gain':float(np.sort(a[:,2])[:tail].mean()-np.sort(b[:,2])[:tail].mean()),
            'produced_gain':d[:,5:].mean(axis=0).tolist()})
    for name,panel in [('neutral7',opponents[:7]),('submitted',[opponents[7]]),('yusuke',[opponents[8]])]:
        values = np.stack([(data[candidate,o]-data[parent,o]).reshape(-1,2,data[candidate,o].shape[1]).mean(axis=1) for o in panel]).mean(axis=0)
        rng = np.random.default_rng(protocol['seed_start'])
        samples = values[rng.integers(0,len(values),size=(4000,len(values)))].mean(axis=1)
        bounds = np.quantile(samples,[.025,.975],axis=0)
        groups.append({'effect':label,'candidate_mask':candidate,'parent_mask':parent,'panel':name,
            'win_gain_pp':float(values[:,0].mean()*100),'cash_gain':float(values[:,1].mean()),'margin_gain':float(values[:,2].mean()),
            'win_gain_95pct_pp':(bounds[:,0]*100).tolist(),'margin_gain_95pct':bounds[:,2].tolist(),
            'hire_cost_gain':float(values[:,3].mean()),'faults_gain':float(values[:,4].mean()),'produced_gain':values[:,5:].mean(axis=0).tolist()})

report = {'games':protocol['games'],'summaries':summaries,'groups':groups,'rows':rows,
    'scope':'Discovery, 64 common seeds and both seats. Bootstrap resamples seeds, preserving both seats and all opponents. No promotion or attribution outside these fixed four-tape policies.'}
(RUN/'RESULTS.json').write_text(json.dumps(report,indent=2)+'\n')
with (RUN/'EFFECTS.csv').open('w') as out:
    writer = csv.DictWriter(out,fieldnames=rows[0].keys());writer.writeheader();writer.writerows(rows)
body = '# V25 component discovery\n\n9,216 games; 64 seeds, both seats, nine opponents. Frozen inputs unchanged. No promotion.\n\n'
body += 'Bit1: weed repair. Bit2: sale leading. Bit4: last-turn delivery/liquidation. All eight masks retain the same four tapes and observed routing rules.\n\n'
body += '| Panel | Added component | Masks | Win gain pp | Margin gain | Margin 95% interval |\n| --- | --- | --- | ---: | ---: | --- |\n'
for g in groups:
    if g['candidate_mask'] == 7:
        body += f"| {g['panel']} | {g['effect']} | 7 / {g['parent_mask']} | {g['win_gain_pp']:+.3f} | {g['margin_gain']:+.2f} | {g['margin_gain_95pct']} |\n"
body += '\nRESULTS.json contains all component contexts, per-opponent regressions, output, hires, faults and tails. EFFECTS.csv is the full 117-row paired table.\n'
(RUN/'RESULTS.md').write_text(body)
print(json.dumps([g for g in groups if g['candidate_mask']==7],indent=2))
