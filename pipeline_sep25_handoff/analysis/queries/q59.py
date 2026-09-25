"""Marginal value of each animal type: top-10 perspectives, own final money regressed on geese, cows,
sheep at day 12 plus shop counts at day 12 (from the shops audit) and land at day 15."""
import numpy as np, pandas as pd
d = pd.read_csv('labor/l_days.csv', usecols=['trace', 'seat', 'label', 'day', 'animals_goose', 'animals_cow', 'animals_sheep', 'land'])
g = pd.read_csv('labor/l_games.csv', usecols=['trace', 'seat', 'label', 'money', 'opp_money'])
d['rank'] = d.label.str.extract(r'\|(\d+)$')[0].astype(float)
x = d[(d.day == 12) & (d['rank'] <= 10)].merge(g, on=['trace', 'seat', 'label'])
land = d[d.day == 15][['trace', 'seat', 'land']].rename(columns={'land': 'land15'})
x = x.merge(land, on=['trace', 'seat'])
sh = pd.read_csv('ledger/shops_top.csv') if __import__('os').path.exists('ledger/shops_top.csv') else None
print('perspectives', len(x))
X = np.column_stack([np.ones(len(x)), x.animals_goose, x.animals_cow, x.animals_sheep, x.land15])
for target in ['money', 'opp_money']:
    y = x[target].to_numpy(float)
    b, *_ = np.linalg.lstsq(X, y, rcond=None)
    res = y - X @ b; se = np.sqrt(np.diag(np.linalg.inv(X.T @ X)) * res.var())
    print(target, dict(zip(['const', 'goose', 'cow', 'sheep', 'land15'], [f'{v:,.0f} (se {s:,.0f})' for v, s in zip(b, se)])))
print(x[['animals_goose', 'animals_cow', 'animals_sheep']].mean().round(2).to_dict())
