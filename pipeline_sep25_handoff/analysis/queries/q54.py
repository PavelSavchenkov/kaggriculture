"""Selling into a crash: units sold on day d at an average price p when the market price reaches
>= 1.5 p within the next 5 days (max hourly price). Per game: units, revenue given up (units x
(future max - p), an upper bound), by source and product. Days 10-28."""
import sys
import numpy as np, pandas as pd
P = ['carrot', 'tomato', 'strawberry', 'melon', 'egg', 'milk', 'wool']
pre = sys.argv[1] if len(sys.argv) > 1 else 'labor/l'
cols = ['trace', 'seat', 'label', 'day'] + [f'{f}_{p}' for p in P for f in ('sold', 'rev', 'max_price')]
d = pd.read_csv(pre + '_days.csv', usecols=cols)
d['rank'] = d.label.str.extract(r'\|(\d+)$')[0].astype(float)
d['src'] = np.where(d.label.str.startswith('ours'), d.label, np.where(d['rank'] <= 10, 'top', 'other'))
d = d.sort_values(['trace', 'seat', 'day'])
rows = []
for p in P:
    fut = d.groupby(['trace', 'seat'])[f'max_price_{p}'].transform(lambda s: s[::-1].rolling(5, min_periods=1).max()[::-1].shift(-1))
    price = d[f'rev_{p}'] / d[f'sold_{p}'].replace(0, np.nan)
    crash = (d[f'sold_{p}'] > 0) & (fut >= 1.5 * price) & d.day.between(10, 28)
    rows.append(pd.DataFrame({'src': d.src, 'trace': d.trace, 'seat': d.seat, 'product': p,
                              'units': np.where(crash, d[f'sold_{p}'], 0), 'lost': np.where(crash, d[f'sold_{p}'] * (fut - price), 0),
                              'sold': d[f'sold_{p}']}))
x = pd.concat(rows)
per_game = x.groupby(['src', 'trace', 'seat', 'product'])[['units', 'lost', 'sold']].sum().reset_index()
t = per_game.groupby(['src', 'product'])[['units', 'lost', 'sold']].mean().unstack('product')
pd.set_option('display.width', 250)
print('units sold into a crash per game'); print(t['units'].round(1).to_string())
print('upper-bound revenue given up per game ($)'); print(t['lost'].round(0).to_string())
tot = per_game.groupby(['src', 'trace', 'seat']).lost.sum().groupby('src').agg(['mean', 'median', 'count'])
print(tot.round(0).to_string())
