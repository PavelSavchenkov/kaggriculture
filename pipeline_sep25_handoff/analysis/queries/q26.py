import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v2.pkl'); d = c['d']
g = c['g'][['trace', 'seat', 'source']]
top = d[d.source.str.startswith('top')][['trace', 'seat']].drop_duplicates()
# opponent rows = the other seat of the same trace (present in the ledger days file for top traces and our games)
from analyze import load
raw = pd.concat([load(p)['days'] for p in ['ledger/top0', 'ledger/top1', 'ledger/ours0', 'ledger/ours1']], ignore_index=True)
raw['kind'] = np.where(raw.label.str.startswith('opp:'), 'LB/C++ opponent', np.where(raw.label.str.contains(r'\|'), 'top team', 'other'))
raw = raw[raw.kind != 'other'].sort_values(['trace', 'seat', 'day'])
prods = ['carrot', 'tomato', 'strawberry', 'melon', 'egg', 'milk', 'wool', 'fertilizer']
rows = []
for p in prods:
    x = raw[['trace', 'seat', 'day', 'kind', f'sold_{p}', f'harvested_{p}']].copy()
    x['trail'] = x.groupby(['trace', 'seat'])[f'sold_{p}'].transform(lambda s: s.shift(1).rolling(3, min_periods=1).mean()).fillna(0)
    x = x[(x.day >= 4) & (x.day <= 28)]
    frac = (x[f'sold_{p}'].sum() / max(1, x[f'harvested_{p}'].sum()))
    for kind, y in x.groupby('kind'):
        f = y[f'sold_{p}'].sum() / max(1, y[f'harvested_{p}'].sum())
        err_trail = (y[f'sold_{p}'] - y.trail).abs().sum() / max(1, y[f'sold_{p}'].sum())
        err_harv = (y[f'sold_{p}'] - f * y[f'harvested_{p}']).abs().sum() / max(1, y[f'sold_{p}'].sum())
        surprise = y[(y[f'sold_{p}'] > 2 * y.trail + 3)][f'sold_{p}'].sum() / max(1, y[f'sold_{p}'].sum())
        rows.append(dict(product=p, opponent=kind, units_per_day=y[f'sold_{p}'].mean(), err_trailing=err_trail, err_today_harvest=err_harv, surprise_volume=surprise))
r = pd.DataFrame(rows).set_index(['product', 'opponent'])
print('Opponent daily sales per product, days 4-28. Error = sum |actual - forecast| / sum actual.')
print('surprise_volume = share of sales on days with sales > 2 x trailing + 3')
print(r.round(2).to_string())
