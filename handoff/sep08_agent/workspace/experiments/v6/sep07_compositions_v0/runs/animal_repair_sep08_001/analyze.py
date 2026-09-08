"""Summarize exposed combination comparisons without promotion."""
from pathlib import Path
from statistics import mean
import argparse
import json
import numpy as np

parser=argparse.ArgumentParser()
parser.add_argument('--run',type=Path,default=Path(__file__).resolve().parent)
args=parser.parse_args()
RUN=args.run.resolve();OUT=RUN/'discovery'
execution=json.loads((OUT/'EXECUTION.json').read_text());assert execution['sources_unchanged']
protocol=json.loads((OUT/'PROTOCOL.json').read_text())
agents=list(dict.fromkeys(j['a'] for j in protocol['jobs']))
pairs=[(a,'empty_sale_slots_m2') for a in agents if a!='empty_sale_slots_m2']
pairs += [
    ('animal_repair_premium_m2', 'animal_premium_m2'),
    ('premium_q24_s216', 'opening_funding_q24'),
    ('premium_q24_s216', 'premium_sales_s216'),
    ('animal_repair_q24_premium_m2', 'animal_repair_premium_m2'),
    ('animal_repair_q24_premium_m2', 'premium_q24_s216'),
]
rows=[];series={};raw_metrics={}
def utility(g):return (g['cash']>g['opponent_cash'])+.5*(g['cash']==g['opponent_cash'])
def margin(g):return g['cash']-g['opponent_cash']
def metric(gs):
    n=len(gs);w=sum(utility(g)==1 for g in gs);t=sum(utility(g)==.5 for g in gs)
    tail=max(1,(n+9)//10)
    return {'games':n,'wtl':[w,t,n-w-t],'utility':mean(map(utility,gs)),
        'cash':mean(g['cash'] for g in gs),'margin':mean(map(margin,gs)),
        'cash_cvar10':mean(sorted(g['cash'] for g in gs)[:tail]),'margin_cvar10':mean(sorted(map(margin,gs))[:tail])}
for native in (False,True):
    panel='native' if native else 'fresh';prefix='native_' if native else ''
    opponents=list(dict.fromkeys(j['b'] for j in protocol['jobs'] if j['native']==native))
    for b in opponents:
        data={a:json.loads((OUT/f'{prefix}{a}_vs_{b}.json').read_text())['games'] for a in agents}
        for a,gs in data.items():
            assert all(g['turns']==719 for g in gs)
            raw_metrics[panel,a,b]=metric(gs)
        for a,parent in pairs:
            x,y=data[a],data[parent]
            assert [(g['seed'],g['seat']) for g in x]==[(g['seed'],g['seat']) for g in y]
            assert len(x)%2==0
            delta=np.array([[100*(utility(g)-utility(c)),g['cash']-c['cash'],margin(g)-margin(c)] for g,c in zip(x,y)])
            # Both seats belong to the same seed cluster. Retain per-seed means
            # so opponent rows can also be resampled jointly.
            series[panel,a,parent,b]=delta.reshape(-1,2,3).mean(axis=1)
            am,bm=raw_metrics[panel,a,b],raw_metrics[panel,parent,b]
            rows.append({'panel':panel,'candidate':a,'parent':parent,'opponent':b,'candidate_metrics':am,'parent_metrics':bm,
                'gain_utility_pp':float(delta[:,0].mean()),'gain_cash':float(delta[:,1].mean()),'gain_margin':float(delta[:,2].mean()),
                'gain_hire_cost':mean(g['profile']['hire_cost']-c['profile']['hire_cost'] for g,c in zip(x,y)),
                'gain_faults':mean(g['unit_faults']-c['unit_faults'] for g,c in zip(x,y)),
                'produced_gain':[mean(g['produced'][i]-c['produced'][i] for g,c in zip(x,y)) for i in range(12)],
                'production_equal_games':sum(g['produced']==c['produced'] for g,c in zip(x,y)),
                'full_records_equal':sum(g==c for g,c in zip(x,y))})
groups=[]
for panel in ('fresh','native'):
    for a,parent in pairs:
        available=[key[3] for key in series if key[:3]==(panel,a,parent)]
        for label,members in [('neutral',[b for b in available if b not in (a,parent,'empty_sale_slots_m2','investment_context_guarded_001_best','opening_q32_b13_v1','bohann_opening_v1','pass')]),
            ('all_non_pass',[b for b in available if b!='pass'])]:
            if not members:continue
            values=np.stack([series[panel,a,parent,b] for b in members]).mean(axis=0)
            rng=np.random.default_rng(2340000);indices=rng.integers(0,len(values),size=(2000,len(values)))
            samples=values[indices].mean(axis=1);bounds=np.quantile(samples,[.025,.975],axis=0)
            groups.append({'panel':panel,'candidate':a,'parent':parent,'group':label,'opponents':members,
                'candidate_utility':mean(raw_metrics[panel,a,b]['utility'] for b in members),
                'parent_utility':mean(raw_metrics[panel,parent,b]['utility'] for b in members),
                'gain_utility_pp':float(values[:,0].mean()),'gain_cash':float(values[:,1].mean()),'gain_margin':float(values[:,2].mean()),
                'gain_95pct':{'utility_pp':bounds[:,0].tolist(),'cash':bounds[:,1].tolist(),'margin':bounds[:,2].tolist()}})
report={'games':execution['games'],'rows':rows,'groups':groups,
    'scope':'Exposed discovery panel, paired by seed and both seats. Group CIs resample seed clusters jointly across opponents. Native scenario uses actual engine randomness. No automatic promotion; inspect every-opponent means/tails and broaden league as needed.'}
(RUN/'RESULTS.json').write_text(json.dumps(report,indent=2)+'\n')
body=f"# Discovery paired results\n\n{execution['games']}complete games; frozen input dependencies unchanged.\nNo promotion is asserted by this report.\n\n"
body+='| Panel | Candidate | Parent | Neutral gain, pp | 95% interval, pp | Margin gain |\n| --- | --- | --- | ---: | ---: | ---: |\n'
for g in groups:
    if g['group']!='neutral':continue
    lo,hi=g['gain_95pct']['utility_pp']
    body+=f"| {g['panel']} | {g['candidate']} | {g['parent']} | {g['gain_utility_pp']:+.3f} | [{lo:+.3f},{hi:+.3f}] | {g['gain_margin']:+.2f} |\n"
body+='\nNeutral groups exclude current/parent/self, submitted/opening ancestors and PASS. Membership is explicit in JSON.\n\n'
body+='| Panel | Candidate | Opponent | W/T/L | Paired margin gain |\n| --- | --- | --- | ---: | ---: |\n'
for r in rows:
    if r['parent']!='empty_sale_slots_m2':continue
    body+=f"| {r['panel']} | {r['candidate']} | {r['opponent']} | {'/'.join(map(str,r['candidate_metrics']['wtl']))} | {r['gain_margin']:+.2f} |\n"
(RUN/'RESULTS.md').write_text(body)
for g in groups:
    if g['group']=='neutral':print(g['panel'],g['candidate'],'vs',g['parent'],g['gain_utility_pp'],g['gain_95pct'],g['gain_margin'])
