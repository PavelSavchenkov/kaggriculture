import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
o = pd.read_csv('ledger/s_ours_sales.csv'); t = pd.read_csv('ledger/s_top_sales.csv')
# our seat: label from list
lab = {}
for line in open('ledger/sales_ours_list.txt'):
    f = line.split(); lab[f[0]] = 0 if f[1].startswith('ours') else 1
o['who'] = np.where(o.seat == o.trace.map(lab), 'ours', 'opp')
t['who'] = 'top'
s = pd.concat([o, t])
m = s[(s['product'] == 4) & (s.day.isin([10, 11]))]
n = s.groupby('who').trace.nunique() * pd.Series({'ours': 1, 'opp': 1, 'top': 2})
print('melon units sold per game by day/hour (day 10-11)')
x = m.pivot_table(index=['who'], columns=['day', 'hour'], values='units', aggfunc='sum').div(n, axis=0).fillna(0)
cols = [c for c in x.columns if x[c].max() >= 0.5]
print(x[cols].round(1).to_string())
print('\nmelon price by day/hour')
y = m.groupby(['who', 'day', 'hour']).apply(lambda z: z.revenue.sum() / z.units.sum()).unstack([1, 2])
print(y[[c for c in cols if c in y.columns]].round(0).to_string())
