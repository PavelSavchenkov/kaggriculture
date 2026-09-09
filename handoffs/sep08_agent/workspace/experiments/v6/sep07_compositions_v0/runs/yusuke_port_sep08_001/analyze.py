"""Compare discovery components on common games; preserve exact source lineage."""
from pathlib import Path
from statistics import mean
import json

RUN=Path(__file__).resolve().parent
out=RUN/'discovery'
execution=json.loads((out/'EXECUTION.json').read_text())
assert execution['sources_unchanged'] and execution['games']==3968
opponents=['empty_sale_slots_m2','teammate_shoprouter','public_router','public_router_v52','ahmed_v23','junghoon_wool_sales','king_rc4']
def load(mode,opponent):return json.loads((out/f'yusuke_sep08_m{mode}_vs_{opponent}.json').read_text())
def metrics(data):
    games=data['games'];assert len(games)==128 and all(g['turns']==719 for g in games)
    wins=sum(g['cash']>g['opponent_cash'] for g in games);ties=sum(g['cash']==g['opponent_cash'] for g in games)
    return {'wtl':[wins,ties,len(games)-wins-ties],'cash':mean(g['cash'] for g in games),
        'margin':mean(g['cash']-g['opponent_cash'] for g in games),'utility':(wins+.5*ties)/len(games)}
rows={opponent:metrics(load(2,opponent)) for opponent in opponents}
effects={}
for parent,child,label in [(0,1,'day6_yarn'),(1,2,'day27_source'),(2,3,'preserve_alternate_farm')]:
    diffs=[]
    for opponent in opponents:
        a,b=load(parent,opponent),load(child,opponent)
        assert [(g['seed'],g['seat']) for g in a['games']]==[(g['seed'],g['seat']) for g in b['games']]
        ma,mb=metrics(a),metrics(b)
        diffs.append({'cash':mb['cash']-ma['cash'],'margin':mb['margin']-ma['margin'],
            'utility_pp':100*(mb['utility']-ma['utility']),
            'faults':mean(g['unit_faults'] for g in b['games'])-mean(g['unit_faults'] for g in a['games'])})
    effects[label]={key:mean(d[key] for d in diffs) for key in diffs[0]}
duels={f'yusuke_sep08_m{i}':metrics(load(2,f'yusuke_sep08_m{i}')) for i in (0,1,3)}
report={'games':3968,'opponents':rows,'component_effects':effects,'direct_siblings':duels,
    'decision':'Strong new opponent: confirm on fresh common scenarios with current strongest controls. No promotion yet.',
    'interpretation':'The local family-preservation amendment lowers mean cash and margin; incompatible tape prefixes alone did not prove harmful execution.'}
(RUN/'RESULTS.json').write_text(json.dumps(report,indent=2)+'\n')
body='''# Yusuke Shop Router0908: discovery results

The faithful C++ source port is mode2.2876literal source actions,24routing cases,
24resets and48full operational games pass.3968common-seed discovery games finish
with271frozen dependencies unchanged. These are exposed64seeds/both seats.

| Opponent | Full port W/T/L | Mean margin |
| --- | ---: | ---: |
'''
for opponent,row in rows.items():body+=f"| {opponent} | {'/'.join(map(str,row['wtl']))} | {row['margin']:+.2f} |\n"
body+='''
| Component change | Own cash | Match margin | Utility pp | Failed unit actions |
| --- | ---: | ---: | ---: | ---: |
'''
for label,row in effects.items():body+=f"| {label} | {row['cash']:+.2f} | {row['margin']:+.2f} | {row['utility_pp']:+.3f} | {row['faults']:+.2f} |\n"
body+='''
This is a strong new opponent, including128/128wins against empty_sale_slots_m2.
The base tape already wins116/128against it. Therefore the two routing decisions
alone do not explain the gap. Investigate farm production, schedules, opening
transactions and the current agent's branch activations with paired controls.

The local mode3 preservation rule reduces full-source mean cash and margin. Its
plausible prefix concern is not a proved bug. Keep the faithful source and the
negative amendment. Source day27 changes raise mean margin but slightly lower
league utility; mode1 also deserves independent confirmation.

Fresh confirmation compares modes1/2/3 and the current reference on identical
scenarios/opponents. This discovery does not promote a new strongest agent or
establish a current official rating. Full source/model attribution is in
LINEAGE.json; original replay IDs remain unknown.
'''
(RUN/'RESULTS.md').write_text(body)
(RUN/'README.md').write_text('''# Yusuke public router port

Read RESULTS.md for the completed3968-game discovery. Mode2is the faithful
upstream; modes0/1isolate its base and early routing, mode3is a local negative
amendment. All four are local C++ packages under proposals/. LINEAGE.json
records the exact notebook archive, four tapes, model and unknown provenance.

prepare.py exports the reviewed immutable tables and registers the packages.
verify_source.py checks all source actions and routing thresholds;
check_operations.py builds generic/pair/debug arenas and runs self/PASS/thread
checks. run_screen.py freezes actual dependencies and runs the component league.
analyze.py reports paired effects. Run through conda's kaggriculture environment.

The full source wins128/128exposed games against our accepted best. Fresh
confirmation is the next gate; no promotion, submission or catalog copy occurs.
''')
print(json.dumps(report['opponents'],indent=2))
