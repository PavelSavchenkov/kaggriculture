import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v2.pkl'); d = c['d'].copy()
d['grp'] = np.where(d.source.str.startswith('top'), 'top', d.source)
keep = ['top', 'ours:vadim6:lb', 'ours:v12:lb', 'ours:vadim6:self']
d = d[d.grp.isin(keep) & (d.day >= 10) & (d.day <= 27)]
prods = ['carrot', 'tomato', 'strawberry', 'egg', 'milk', 'wool', 'fertilizer', 'wheat']
rows = []
for p in prods:
    avail = d[f'shed_{p}'] + d[f'harvested_{p}'] + (d[f'bought_{p}'] if f'bought_{p}' in d else 0)
    x = d.assign(avail=avail, sold=d[f'sold_{p}'], stock=d[f'shed_{p}'])
    x = x[x.avail >= 3]
    x['frac'] = (x.sold / x.avail).clip(0, 1)
    g = x.groupby('grp').agg(dawn_stock=('stock', 'mean'), frac_mean=('frac', 'mean'), frac_std=('frac', 'std'),
                             frac_all=('frac', lambda f: (f >= 0.95).mean()), frac_none=('frac', lambda f: (f <= 0.05).mean()))
    g['product'] = p
    rows.append(g.reset_index())
r = pd.concat(rows).set_index(['product', 'grp']).reindex(pd.MultiIndex.from_product([prods, keep]))
print('days 10-27, product-days with >= 3 units available (dawn stock + harvest + buys)')
print(r.round(2).to_string())
