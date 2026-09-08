"""Expose both production profiles for the same discovery head-to-head games."""
from pathlib import Path
from statistics import mean
import json
import subprocess

RUN=Path(__file__).resolve().parent
out=RUN/'gap_diagnostic';out.mkdir(exist_ok=False)
binary=json.loads((RUN/'OPERATIONAL_BINARIES.json').read_text())['generic']['binary']
path=out/'reverse.json'
cmd=['conda','run','-n','kaggriculture',binary,'--a','empty_sale_slots_m2','--b','yusuke_sep08_m2',
    '--games','64','--seed-start','1000','--seat-mode','both','--threads','4','--validate','--profile','--output',str(path)]
with (out/'reverse.log').open('x') as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
forward=json.loads((RUN/'discovery/yusuke_sep08_m2_vs_empty_sale_slots_m2.json').read_text())['games']
reverse={(g['seed'],g['seat']):g for g in json.loads(path.read_text())['games']}
pairs=[]
for a in forward:
    b=reverse[a['seed'],a['seat']^1]
    assert (a['cash'],a['opponent_cash'],a['action_hash'],a['opponent_action_hash'])==(b['opponent_cash'],b['cash'],b['opponent_action_hash'],b['action_hash'])
    pairs.append((a,b))
items='WHEAT CARROT TOMATO STRAWBERRY MELON EGG MILK WOOL FERTILIZER GOOSE COW SHEEP'.split()
rows=[]
for i,item in enumerate(items):
    for name,read in [('produced',lambda g:g['produced'][i]),('bought',lambda g:g['profile']['buys'][i]),('sold',lambda g:g['sold'][i])]:
        a=mean(read(a) for a,b in pairs);b=mean(read(b) for a,b in pairs)
        rows.append({'metric':f'{name}_{item}','yusuke':a,'current':b,'difference':a-b})
for name in ('hires','hire_cost','land_cost','weed_digs','shed_ge90'):
    a=mean(a['profile'][name] for a,b in pairs);b=mean(b['profile'][name] for a,b in pairs)
    rows.append({'metric':name,'yusuke':a,'current':b,'difference':a-b})
for name in ('unit_faults','revenue','spend','cash'):
    a=mean(a[name] for a,b in pairs);b=mean(b[name] for a,b in pairs)
    rows.append({'metric':name,'yusuke':a,'current':b,'difference':a-b})
def herd(g,day):
    return tuple(sum(life[0]==item and life[3]<=day*24<life[4] for life in g['profile']['lives']) for item in (10,11,9))
contexts=[]
for yarn in (False,True):
    subset=[(a,b) for a,b in pairs if (7 in a['shops'][:2])==yarn]
    contexts.append({'yarn_in_first_two':yarn,'games':len(subset),
        'mean_margin':mean(a['cash']-b['cash'] for a,b in subset),
        'herd15_yusuke':list({herd(a,15) for a,b in subset}),
        'herd15_current':list({herd(b,15) for a,b in subset})})
(out/'REPORT.json').write_text(json.dumps({'command':cmd,'exact_swapped_controls':len(pairs),'metrics':rows,'contexts':contexts},indent=2)+'\n')
body='''# Same-game gap against Yusuke

The reverse128-game run matches both agents' cash and complete action hashes
after swapping reported seats. This exposes both production arrays for exactly
the discovery games. Differences are descriptive; interventions must identify
which component causes them.

| Metric | Yusuke full | Current reference | Difference |
| --- | ---: | ---: | ---: |
'''
for row in rows:body+=f"| {row['metric']} | {row['yusuke']:.2f} | {row['current']:.2f} | {row['difference']:+.2f} |\n"
(out/'REPORT.md').write_text(body)
print(json.dumps({'contexts':contexts,'economics':rows[-9:]},indent=2))
