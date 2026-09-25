"""Kaggle games of our submission: per game, ours vs opponent by product (revenue, units, price),
wages, animals at day 12, melons planted by day 3; sorted by margin."""
import sys
import pandas as pd
P = ['wheat','carrot','tomato','strawberry','melon','egg','milk','wool','fertilizer']
prefix = sys.argv[1] if len(sys.argv) > 1 else 'ledger/kaggle'
d = pd.read_csv(prefix + '_days.csv'); g = pd.read_csv(prefix + '_games.csv')
d['ep'] = d.trace.str.extract(r'(\d+)\.txt$')[0]
tot = d.groupby(['ep', 'label']).agg(**{f'rev_{p}': (f'rev_{p}', 'sum') for p in P}, **{f'sold_{p}': (f'sold_{p}', 'sum') for p in P},
                                     wages=('hire_cost', 'sum'), land=('land_cost', 'sum')).reset_index()
dd = d.set_index(['ep', 'label', 'day'])
rows = []
for ep, x in tot.groupby('ep'):
    o = x[x.label.str.startswith('ours')].iloc[0]; p = x[~x.label.str.startswith('ours')].iloc[0]
    r = dict(ep=ep, opp=p.label[4:24])
    gg = g[g.trace.str.contains(ep) & g.label.str.startswith('ours')].iloc[0]
    r['own'] = gg.money; r['margin'] = gg.money - gg.opp_money
    for q in ['strawberry', 'melon', 'milk', 'wool', 'egg', 'wheat', 'fertilizer', 'tomato', 'carrot']:
        r[q[:5]] = o[f'rev_{q}'] - p[f'rev_{q}']
    r['wages'] = -(o.wages - p.wages)
    for lab, tag in ((o.label, 'o'), (p.label, 'p')):
        r[f'an12{tag}'] = sum(dd.loc[(ep, lab, 12), f'animals_{a}'] for a in ['goose', 'cow', 'sheep'])
        r[f'mel3{tag}'] = dd.loc[(ep, lab, 3), 'plants_melon']
    rows.append(r)
t = pd.DataFrame(rows).sort_values('margin')
pd.set_option('display.width', 250)
print(t.to_string(index=False, float_format=lambda v: f'{v:,.0f}'))
