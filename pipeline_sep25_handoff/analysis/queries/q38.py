import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v2.pkl'); d = c['d']
d = d[d.source.isin(['top', 'top_vs20'])]
sh = pd.read_csv('ledger/shops.csv')
y24 = sh[sh.day == 24].set_index('trace').yarn
d = d.assign(yarn24=d.trace.map(y24))
print(d.groupby('yarn24').trace.nunique())
for day in (6, 9, 12, 15, 18, 21, 24, 27):
    x = d[d.day == day]
    print(day, x.groupby('yarn24').mean_price_wool.mean().round(1).to_dict(), 'sold/game', x.groupby('yarn24').sold_wool.mean().round(1).to_dict())
