import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
t = pd.read_csv('ledger/s_top_sales.csv')
m = t[t['product'] == 4].copy()
m['h'] = m.day * 24 + m.hour
# per perspective: first melon sale hour, melon revenue/unit; compare seats within the same game
f = m.groupby(['trace', 'seat']).agg(first=('h', 'min'), units=('units', 'sum'), rev=('revenue', 'sum')).reset_index()
f['price'] = f.rev / f.units
w = f.pivot(index='trace', columns='seat')
w = w.dropna()
earlier = np.where(w['first'][0] < w['first'][1], 0, np.where(w['first'][1] < w['first'][0], 1, -1))
w['earlier_price'] = np.where(earlier == 0, w['price'][0], w['price'][1])
w['later_price'] = np.where(earlier == 0, w['price'][1], w['price'][0])
x = w[earlier >= 0]
print('top-vs-top games (400 traces): melon price of the seat that sells first vs second')
print(len(x), 'games; first seller', x.earlier_price.mean().round(1), 'second seller', x.later_price.mean().round(1))
print('first-sale hour distribution (day 10 hours):'); print(((f['first'] - 240)).clip(-1, 30).value_counts().sort_index().head(26).to_string())
