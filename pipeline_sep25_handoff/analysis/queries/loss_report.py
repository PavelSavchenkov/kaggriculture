"""One Kaggle game of our submission, ours vs opponent: revenue by product, costs, dawn money,
herd and crops, melon sales, contested-product prices by hour, crash selling, animal service.
usage: loss_report.py <episode>   (needs kaggle_ours/meta_56553038.csv and traces; runs tools/ledger3)"""
import csv, subprocess, sys
from pathlib import Path
import numpy as np, pandas as pd
ep = sys.argv[1]
W = Path(__file__).resolve().parent
meta = next(r for r in csv.DictReader(open(W / 'kaggle_ours/meta_56553038.csv')) if r['episode'] == ep)
seat = int(meta['seat'])
labels = ['ours', 'opp'] if seat == 0 else ['opp', 'ours']
(W / 'ledger').mkdir(exist_ok=True)
lst = W / f'ledger/ep_{ep}_list.txt'
lst.write_text(f"{W}/kaggle_ours/traces/{ep}.txt {labels[0]} {labels[1]} kaggle\n")
env = dict(**__import__('os').environ, LEDGER_SALES='1', LEDGER_HIRES='1')
subprocess.run([str(W / 'tools/ledger3'), str(lst), str(W / f'ledger/ep_{ep}')], check=True, env=env, capture_output=True)
P = ['wheat', 'carrot', 'tomato', 'strawberry', 'melon', 'egg', 'milk', 'wool', 'fertilizer']
BASE = dict(zip(P, [25, 35, 60, 120, 250, 50, 160, 200, 100]))
d = pd.read_csv(W / f'ledger/ep_{ep}_days.csv'); s = pd.read_csv(W / f'ledger/ep_{ep}_sales.csv'); s = s[s.units > 0]
s['who'] = s.seat.map(dict(enumerate(labels)))
pd.set_option('display.width', 250)
print(f"episode {ep} vs {meta['opp_team']}: ours {meta['own']} opp {meta['opp']} margin {float(meta['own']) - float(meta['opp']):+.0f}\n")
g = d.groupby('label').agg(**{p: (f'rev_{p}', 'sum') for p in P}, wages=('hire_cost', 'sum'), land=('land_cost', 'sum'),
                           wheat_buy=('buy_cost_wheat', 'sum'), fert_buy=('buy_cost_fertilizer', 'sum'))
g.loc['ours-opp'] = g.loc['ours'] - g.loc['opp']
print('Revenue and costs'); print(g.round(0).to_string(), '\n')
d['animals'] = d[['animals_goose', 'animals_cow', 'animals_sheep']].sum(axis=1)
d['herd'] = d.animals_goose.astype(str) + '/' + d.animals_cow.astype(str) + '/' + d.animals_sheep.astype(str)
for c in ['money_dawn', 'herd', 'plants_melon', 'plants_strawberry', 'plants_wheat', 'hires', 'revenue', 'shed_dawn']:
    t = d.pivot_table(index='label', columns='day', values=c, aggfunc='first')
    print(c); print(t.to_string(), '\n')
x = s[s['product'] == 4][['who', 'day', 'hour', 'units', 'revenue']].copy(); x['price'] = (x.revenue / x.units).round(0)
print('Melon sales'); print(x.drop(columns='revenue').to_string(index=False), '\n')
s['bucket'] = pd.cut(s.hour, [-1, 5, 11, 17, 21, 23], labels=['0-5', '6-11', '12-17', '18-21', '22-23'])
for p in ['milk', 'wool', 'strawberry', 'egg']:
    y = s[s['product'] == P.index(p)]
    t = y.groupby(['who', 'bucket'], observed=False).agg(units=('units', 'sum'), rev=('revenue', 'sum'))
    t['price'] = (t.rev / t.units).round(0)
    tot = y.groupby('who').agg(units=('units', 'sum'), rev=('revenue', 'sum')); tot['price'] = (tot.rev / tot.units).round(1)
    print(p, dict(zip(tot.index, zip(tot.units, tot.price)))); print(t.unstack('bucket')[['units', 'price']].to_string(), '\n')
# crash selling (price >= 1.5x within 5 days)
rows = []
for p in ['strawberry', 'milk', 'wool', 'melon']:
    for lab, y in d.sort_values('day').groupby('label'):
        fut = y[f'max_price_{p}'][::-1].rolling(5, min_periods=1).max()[::-1].shift(-1)
        price = y[f'rev_{p}'] / y[f'sold_{p}'].replace(0, np.nan)
        m = (y[f'sold_{p}'] > 0) & (fut >= 1.5 * price) & y.day.between(10, 28)
        rows.append((lab, p, int(y[f'sold_{p}'][m].sum()), round(float((y[f'sold_{p}'] * (fut - price))[m].sum()))))
print('Crash selling (units, upper-bound $ given up):', rows, '\n')
a = d.groupby('label')[['fed', 'cared', 'unfed', 'fert_missed', 'bonus_lost'] + [f'discarded_{p}' for p in P]].sum()
print(a.loc[:, (a != 0).any()].to_string())
