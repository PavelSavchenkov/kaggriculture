import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v2.pkl'); g = c['g']
top = g[g.source.isin(['top', 'top_vs20'])].copy()
sh = pd.read_csv('ledger/shops.csv')
s12 = sh[sh.day == 24].drop(columns='day').set_index('trace')
top = top.join(s12, on='trace')
tests = {'wool / sheep': ('rev_wool', 'd15_animals_sheep', 'price_wool', ['yarn', 'yarn']),
         'milk / cow': ('rev_milk', 'd15_animals_cow', 'price_milk', ['pizza', 'icecream', 'smoothie']),
         'egg / goose': ('rev_egg', 'd15_animals_goose', 'price_egg', ['bakery', 'brunch']),
         'strawberry / plant': ('rev_strawberry', 'planted_strawberry', 'price_strawberry', ['brunch', 'icecream', 'smoothie', 'farmers']),
         'tomato / plant': ('rev_tomato', 'planted_tomato', 'price_tomato', ['pizza', 'farmers']),
         'carrot / plant': ('rev_carrot', 'planted_carrot', 'price_carrot', ['petcafe', 'petcafe', 'farmers'])}
rows = []
for name, (rev, n, price, shops) in tests.items():
    x = top[top[n] > 0].copy()
    x['demand'] = sum(x[s] for s in shops).clip(upper=4)
    x['rev_per'] = x[rev] / x[n]
    t = x.groupby('demand').agg(games=(rev, 'size'), producers=(n, 'mean'), price=(price, 'mean'), rev_per_producer=('rev_per', 'mean'))
    t['test'] = name
    rows.append(t.reset_index())
r = pd.concat(rows).set_index(['test', 'demand'])
print('Top-team games: realised price and revenue per producer by shop demand units unlocked by day 24 (all shops)')
print(r.round(1).to_string())
