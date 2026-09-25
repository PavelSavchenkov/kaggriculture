"""Per-product linear forecast of the opponent's daily sales: sold ~ a*trailing + b*visible (no
intercept, non-negative), fitted on top-team perspectives (odd traces), evaluated (WAPE) on held-out
top-team traces and on Local-LB and zoo opponents."""
import warnings; warnings.filterwarnings("ignore")
import numpy as np, pandas as pd
from scipy.optimize import nnls
P = ["wheat", "carrot", "tomato", "strawberry", "melon", "egg", "milk", "wool", "fertilizer"]
f = pd.read_csv('ledger/fc.csv')
f['kind'] = np.where(f.label.str.startswith('opp:'), np.where(f.group.str.contains('/lb/'), 'LB agent', 'zoo BC'),
                     np.where(f.label.str.contains(r'\|'), 'top team', 'ours'))
f = f[f.kind != 'ours'].sort_values(['trace', 'seat', 'product', 'day'])
f['trail'] = f.groupby(['trace', 'seat', 'product']).sold.transform(lambda s: s.shift(1).rolling(3, min_periods=1).mean()).fillna(0)
f = f[(f.day >= 4) & (f.day <= 27)]
f['fold'] = f.trace.map(hash) % 2
coef, rows = {}, []
for p in range(1, 8):
    x = f[(f['product'] == p) & (f.kind == 'top team') & (f.fold == 0)]
    a, _ = nnls(np.c_[x.trail, x.visible], x.sold.values.astype(float))
    coef[P[p]] = a
    for kind in ['top team', 'LB agent', 'zoo BC']:
        y = f[(f['product'] == p) & (f.kind == kind) & ((f.fold == 1) | (kind != 'top team'))]
        tot = max(1, y.sold.sum())
        rows.append(dict(product=P[p], kind=kind, trailing=(y.sold - y.trail).abs().sum() / tot,
                         blend=(y.sold - 0.5 * y.trail - 0.5 * y.visible).abs().sum() / tot,
                         linear=(y.sold - a[0] * y.trail - a[1] * y.visible).abs().sum() / tot))
print('coefficients (trailing, visible):', {k: tuple(np.round(v, 2)) for k, v in coef.items()})
print(pd.DataFrame(rows).set_index(['product', 'kind']).round(2).to_string())
