import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v2.pkl'); g = c['g']
top = g[g.source.isin(['top', 'top_vs20'])].copy()
sh = pd.read_csv('ledger/shops.csv'); s24 = sh[sh.day == 24].drop(columns='day').set_index('trace')
top = top.join(s24, on='trace')
opp = top.set_index(['trace', 'seat']); opp.index = pd.MultiIndex.from_arrays([opp.index.get_level_values(0), 1 - opp.index.get_level_values(1)])
top = top.set_index(['trace', 'seat'])
o = opp.reindex(top.index)
shops = ['bakery', 'brunch', 'farmers', 'icecream', 'petcafe', 'pizza', 'smoothie', 'yarn']
items = {'sheep': ('d15_animals_sheep', 'price_wool', 'rev_wool'), 'cow': ('d15_animals_cow', 'price_milk', 'rev_milk'),
         'goose': ('d15_animals_goose', 'price_egg', 'rev_egg'), 'strawberry': ('planted_strawberry', 'price_strawberry', 'rev_strawberry')}
X = np.c_[np.ones(len(top)), top[shops].values]
def resid(y):
    ok = ~np.isnan(y)
    b = np.linalg.lstsq(X[ok], y[ok], rcond=None)[0]
    return y - X @ b
for name, (n, price, rev) in items.items():
    own_r = resid(top[n].values.astype(float)); opp_r = resid(o[n].values.astype(float))
    ok = ~np.isnan(own_r) & ~np.isnan(opp_r)
    r_mix = np.corrcoef(own_r[ok], opp_r[ok])[0, 1]
    # price effect of the opponent's producers, controlling for shops and own producers
    Z = np.c_[X, top[n].values, o[n].values]
    y = top[price].values
    okp = ~np.isnan(y) & ~np.isnan(Z).any(axis=1)
    b = np.linalg.lstsq(Z[okp], y[okp], rcond=None)[0]
    yr = top[rev].values
    b2 = np.linalg.lstsq(Z[okp], yr[okp], rcond=None)[0]
    print(f"{name:10s} residual mix correlation own vs opponent {r_mix:+.2f} | price per own producer {b[-2]:+.1f}, per opponent producer {b[-1]:+.1f} | revenue per own producer {b2[-2]:+.0f}, per opponent producer {b2[-1]:+.0f}")
