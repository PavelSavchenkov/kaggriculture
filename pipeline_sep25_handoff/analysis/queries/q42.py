import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
P = ["wheat", "carrot", "tomato", "strawberry", "melon", "egg", "milk", "wool", "fertilizer"]
f = pd.read_csv('ledger/fc.csv')
f['kind'] = np.where(f.label.str.startswith('opp:'), np.where(f.group.str.contains('/lb/'), 'LB agent', 'zoo BC'),
                     np.where(f.label.str.contains(r'\|'), 'top team', 'ours'))
f = f[f.kind != 'ours'].sort_values(['trace', 'seat', 'product', 'day'])
f['trail'] = f.groupby(['trace', 'seat', 'product']).sold.transform(lambda s: s.shift(1).rolling(3, min_periods=1).mean()).fillna(0)
f = f[(f.day >= 4) & (f.day <= 27) & f['product'].isin([1, 2, 3, 4, 5, 6, 7])]
rows = []
for (kind, p), x in f.groupby(['kind', 'product']):
    act = x.sold; tot = max(1, act.sum())
    r = dict(kind=kind, product=P[p], units_day=act.mean(), trailing=(act - x.trail).abs().sum() / tot)
    for a in (0.3, 0.6, 1.0):
        r[f'max(tr,{a}vis)'] = (act - np.maximum(x.trail, a * x.visible)).abs().sum() / tot
    for b in (0.5, 1.0):
        r[f'{b}vis'] = (act - b * x.visible).abs().sum() / tot
    r['0.5tr+0.5vis'] = (act - 0.5 * x.trail - 0.5 * x.visible).abs().sum() / tot
    rows.append(r)
r = pd.DataFrame(rows).set_index(['kind', 'product'])
print('WAPE of opponent daily sales forecasts (lower is better), days 4-27')
print(r.round(2).to_string())
