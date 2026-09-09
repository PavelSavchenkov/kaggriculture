"""Report exact eligibility and distinct later schedules from the completed survey."""
from collections import Counter
from pathlib import Path
import json

RUN=Path(__file__).resolve().parent
out=RUN/'probe_v2'
execution=json.loads((out/'EXECUTION.json').read_text())
assert execution['sources_unchanged'] and execution['games']==2048
rows=[]
for path in out.glob('*.json'):
    data=json.loads(path.read_text())
    if 'records' not in data:continue
    assert len(data['records'])==256 and data['controls']==8
    for i in range(6):
        exact=[r for r in data['records'] if r['entries'][i]['exact']]
        day=20 if i<4 else 15
        rows.append({'opponent':data['opponent'],'name':data['records'][0]['entries'][i]['name'],
            'games':len(data['records']),'exact':len(exact),
            'tile_matches':sum(r['entries'][i]['tiles']==0 for r in data['records']),
            'berry_true':sum(r['berry20']>=4 for r in exact),
            'unit_calendars':len({tuple(r['days'][day:]) for r in exact}),
            'herds':dict(Counter('/'.join(map(str,r['entries'][i]['herd'])) for r in exact)),
            'witnesses':[{'seed':r['seed'],'seat':r['seat'],'berry20':r['berry20']} for r in exact]})
assert len(rows)==48
(RUN/'ENTRY_SURVEY.json').write_text(json.dumps({'games':2048,'standard_control_games':64,'rows':rows},indent=2)+'\n')
body='''# Animal-course entry survey

2048 complete unchanged strongest-agent games,128 exposed seeds/both seats,
eight opponents.64 independent standard-runner controls match both complete
action hashes and final cash. Frozen source hashes are unchanged. No candidate
investment was executed; these are eligibility rates, not win rates.

| Opponent | Sheep exact /256 | Cow exact /256 |
| --- | ---: | ---: |
'''
for opponent in dict.fromkeys(r['opponent'] for r in rows):
    sheep=next(r for r in rows if r['opponent']==opponent and r['name']=='sheep_3')
    cow=next(r for r in rows if r['opponent']==opponent and r['name']=='cow_9')
    body+=f"| {opponent} | {sheep['exact']} | {cow['exact']} |\n"
body+='''
The sheep entry has8cows/7sheep/3geese.37games match exactly;12of these have
day20strawberry demand at least4. Two distinct later unit calendars correspond
to that observed gate. Compile both leaves. The original false-leaf witness is
public_router seed1016seat0; true-leaf source can use PASS seed1048seat0, which
matches the same physical entry exactly. All four sheep modifications have the
same eligibility counts in this survey.

The cow entry has6cows/11sheep/0geese, identifying the imported wool farm rather
than the general crop/animal branch.223games match exactly.20have high day20
strawberry demand, but unit calendars do not change with that gate in this farm.
Only day23 has two unit calendars;56of223matches differ from seed1014seat0.
Inspect original tile work and inventories before assuming this is a semantic
branch. Public_router seed1019seat0 supplies an alternate witness.

Cow guards miss12additional tile-matching games each againstV52andAhmed due
to stock conditions. Do not remove those inventory checks without a funded,
physically validated entry repair. Broader herd contexts need new schedules;
the current narrow entry coverage is a measured limitation, not the final scope.
'''
(RUN/'ENTRY_SURVEY.md').write_text(body)
print('Entry survey complete:48course/opponent records,2048games,64controls')
