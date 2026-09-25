"""Crash behaviour: on days when a product's mean market price is below half its base price, the
share of available stock (shed at dawn + harvested that day) each seat sells. Days 10-28."""
import numpy as np, pandas as pd
BASE = {'carrot': 35, 'tomato': 60, 'strawberry': 120, 'melon': 250, 'egg': 50, 'milk': 160, 'wool': 200}
P = list(BASE)
cols = ['trace', 'seat', 'label', 'day'] + [f'{f}_{p}' for p in P for f in ('sold', 'shed', 'harvested', 'mean_price')]
d = pd.read_csv('labor/l_days.csv', usecols=cols)
d['rank'] = d.label.str.extract(r'\|(\d+)$')[0].astype(float)
d['src'] = np.select([d.label.str.startswith('ours'), d['rank'] <= 10, d['rank'] > 10, d.label.str.startswith('opp:')],
                     ['ours', 'top10', 'rank11+', 'local-LB'], 'other')
d = d[d.day.between(10, 28)]
rows = []
for p in P:
    avail = d[f'shed_{p}'] + d[f'harvested_{p}']
    crash = d[f'mean_price_{p}'] < 0.5 * BASE[p]
    normal = d[f'mean_price_{p}'] >= 0.8 * BASE[p]
    for tag, m in [('crash', crash), ('normal', normal)]:
        x = d[m & (avail >= 5)]
        rows.append(pd.DataFrame({'src': x.src, 'product': p, 'state': tag, 'sold': x[f'sold_{p}'], 'avail': avail[x.index]}))
r = pd.concat(rows)
t = r.groupby(['src', 'state', 'product']).apply(lambda y: pd.Series({'n': len(y), 'sell_through': y.sold.sum() / y.avail.sum()}), include_groups=False)
pd.set_option('display.width', 250)
print(t['sell_through'].unstack('product').round(2).to_string())
print(t['n'].unstack('product').to_string())
