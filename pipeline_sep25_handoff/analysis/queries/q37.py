import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v2.pkl'); g = c['g']; an = c['animals']
top = g[g.source.isin(['top', 'top_vs20'])].copy()
sh = pd.read_csv('ledger/shops.csv')
first_yarn = sh[sh.yarn > 0].groupby('trace').day.min()
top['yarn_day'] = top.trace.map(first_yarn).fillna(99)
top['yarn_by24'] = top.trace.map(sh[sh.day == 24].set_index('trace').yarn)
top['bucket'] = pd.cut(top.yarn_day, [0, 6, 12, 18, 24, 100], labels=['yarn by d6', 'd9-12', 'd15-18', 'd21-24', 'never'])
a = an[an.source.isin(['top', 'top_vs20'])]
sheep = a[a.species == 'sheep'].groupby(['trace', 'seat']).agg(sheep_n=('day', 'size'), wool_collected=('collected', 'sum'), sheep_held_end=('held_end', 'sum'), escaped_sheep=('escaped', lambda s: (s >= 0).sum()))
top = top.join(sheep, on=['trace', 'seat'])
t = top.groupby('bucket').agg(games=('win', 'size'), sheep=('sheep_n', 'mean'), wool_collected=('wool_collected', 'mean'), wool_sold=('sold_wool', 'mean'),
                              wool_price=('price_wool', 'mean'), wool_rev=('rev_wool', 'mean'), wool_disc=('discarded_wool', 'mean'), held_end=('sheep_held_end', 'mean'))
print('Top-team games by first Yarn Store day'); print(t.round(1).to_string())
