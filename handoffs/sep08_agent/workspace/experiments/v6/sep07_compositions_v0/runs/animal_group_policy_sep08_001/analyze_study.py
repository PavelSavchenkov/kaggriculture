"""Report exact controls, paired league changes and estimate-to-play errors."""
from pathlib import Path
from statistics import mean
import json
import random

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
OUT=RUN/'study'
execution=json.loads((OUT/'EXECUTION.json').read_text());assert execution['sources_unchanged']
protocol=json.loads((OUT/'PROTOCOL.json').read_text())
variants=list(dict.fromkeys(a for a,b in protocol['jobs']))
opponents=list(dict.fromkeys(b for a,b in protocol['jobs']))
games={};diagnostics={};controls=0
for a,b in protocol['jobs']:
    path=OUT/f'{a}_vs_{b}.json'
    games[a,b]=json.loads(path.read_text())['games']
    diagnostics[a,b]=json.loads(Path(str(path)+'.diagnostics.json').read_text())['games']
    assert len(games[a,b])==128 and all(g['turns']==719 for g in games[a,b])
    assert [(g['seed'],g['seat']) for g in games[a,b]]==[(d['seed'],d['seat']) for d in diagnostics[a,b]]
    if a=='animal_groups_m0':
        old=EXP/'runs/opening_funding_sep08_001/discovery'/f'empty_sale_slots_m2_vs_{b}.json'
        if old.exists():
            assert games[a,b]==json.loads(old.read_text())['games'],b
            controls+=128
assert controls==1152
def utility(g):return (g['cash']>g['opponent_cash'])+.5*(g['cash']==g['opponent_cash'])
def margin(g):return g['cash']-g['opponent_cash']
def interval(values):
    rng=random.Random(102);keys=sorted(values)
    samples=sorted(mean(values[rng.choice(keys)] for _ in keys) for _ in range(2000))
    return [samples[49],samples[1949]]
rows=[];groups={};neutral=[b for b in opponents if b not in ('empty_sale_slots_m2','investment_context_guarded_001_best','pass')]
for a in variants:
    for b in opponents:
        base=games['animal_groups_m0',b];actual=games[a,b];ds=diagnostics[a,b]
        active=[i for i,d in enumerate(ds) if d['family']>=0]
        row={'agent':a,'opponent':b,'games':len(actual),'wins':sum(utility(g)==1 for g in actual),
            'ties':sum(utility(g)==.5 for g in actual),'losses':sum(utility(g)==0 for g in actual),
            'utility':mean(map(utility,actual)),'mean_cash':mean(g['cash'] for g in actual),'mean_margin':mean(map(margin,actual)),
            'gain_utility_pp':100*mean(utility(g)-utility(c) for g,c in zip(actual,base)),
            'gain_cash':mean(g['cash']-c['cash'] for g,c in zip(actual,base)),
            'gain_margin':mean(margin(g)-margin(c) for g,c in zip(actual,base)),
            'active':len(active),'guard_miss_games':sum(d['missed_days']!=0 for d in ds),
            'advanced_orders':sum(d['advanced_orders'] for d in ds),
            'choices':{f'{f}:{c}':sum(d['family']==f and d['choice']==c for d in ds) for f,c in [(0,1),(0,2),(1,1),(1,2),(1,3),(1,4)]}}
        if active:
            row['activated']={'own_gain':mean(actual[i]['cash']-base[i]['cash'] for i in active),
                'margin_gain':mean(margin(actual[i])-margin(base[i]) for i in active),
                'prediction_own':mean(ds[i]['predicted_own'][ds[i]['choice']] for i in active),
                'own_mae':mean(abs(actual[i]['cash']-base[i]['cash']-ds[i]['predicted_own'][ds[i]['choice']]) for i in active),
                'hire_cost_gain':mean(actual[i]['profile']['hire_cost']-base[i]['profile']['hire_cost'] for i in active),
                'fault_gain':mean(actual[i]['unit_faults']-base[i]['unit_faults'] for i in active),
                'produced_gain':[mean(actual[i]['produced'][p]-base[i]['produced'][p] for i in active) for p in range(12)]}
        rows.append(row)
    group={}
    for name,members in [('neutral',neutral),('all_non_pass',[b for b in opponents if b!='pass'])]:
        seeds={}
        for b in members:
            for g,c in zip(games[a,b],games['animal_groups_m0',b]):seeds.setdefault(g['seed'],[]).append(100*(utility(g)-utility(c)))
        values={k:mean(v) for k,v in seeds.items()}
        group[name]={'opponents':members,'utility':mean(utility(g) for b in members for g in games[a,b]),
            'paired_gain_pp':mean(values.values()),'gain_95pct_pp':interval(values),
            'margin_gain':mean(margin(g)-margin(c) for b in members for g,c in zip(games[a,b],games['animal_groups_m0',b]))}
    groups[a]=group
report={'games':execution['games'],'independent_full_record_controls':controls,'rows':rows,'groups':groups,
    'scope':'Exposed discovery. Confidence intervals cluster both seats and opponents by seed. No promotion; unseen native/fresh/expanded-context audits remain required.'}
(RUN/'STUDY_RESULTS.json').write_text(json.dumps(report,indent=2)+'\n')
lines=['# Animal-course policy discovery','',f"{execution['games']}complete games, seven policies and ten opponents;128games each.",
    f'{controls}unchanged-parent complete records equal the prior independent discovery exactly.',
    'All registered-adapter controls pass; frozen inputs unchanged. This is discovery, not promotion.','',
    '| Agent | Neutral utility | Paired gain, pp | 95% interval, pp | Mean margin gain | Activated / guard misses |',
    '| --- | ---: | ---: | ---: | ---: | ---: |']
for a in variants:
    g=groups[a]['neutral'];rr=[r for r in rows if r['agent']==a]
    lo,hi=g['gain_95pct_pp']
    lines.append(f"| {a} | {g['utility']:.3%} | {g['paired_gain_pp']:+.3f} | [{lo:+.3f},{hi:+.3f}] | {g['margin_gain']:+.2f} | {sum(r['active'] for r in rr)} / {sum(r['guard_miss_games'] for r in rr)} |")
lines+=['','Neutral group excludes current-parent self-play, submitted ancestor and PASS.','',
    '| Agent | Opponent | W/T/L | Paired margin gain | Active | Guard misses |','| --- | --- | ---: | ---: | ---: | ---: |']
for r in rows:
    if r['agent']=='animal_groups_m0':continue
    lines.append(f"| {r['agent']} | {r['opponent']} | {r['wins']}/{r['ties']}/{r['losses']} | {r['gain_margin']:+.2f} | {r['active']} | {r['guard_miss_games']} |")
lines+=['','Complete per-opponent production, labor, forecast error and activation counts are in STUDY_RESULTS.json.',
    'Inspect guard misses and negative contexts before accepting any policy. Broader herd/date/mixed-species choices remain unfinished.']
(RUN/'STUDY_RESULTS.md').write_text('\n'.join(lines)+'\n')
for a,g in groups.items():print(a,g['neutral'])
