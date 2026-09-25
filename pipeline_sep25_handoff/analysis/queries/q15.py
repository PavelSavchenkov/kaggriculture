import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
P = ["wheat", "carrot", "tomato", "strawberry", "melon", "egg", "milk", "wool", "fertilizer"]
s = pd.read_csv('ledger/s_lb_sales.csv')
ours = {}
for line in open('ledger/sales_lb_list.txt'):
    f = line.split(); ours[f[0]] = 0 if f[1].startswith('ours') else 1
s['who'] = np.where(s.seat == s.trace.map(ours), 'ours', 'opp')
day = s.groupby(['trace', 'day', 'product', 'who']).agg(units=('units', 'sum'), rev=('revenue', 'sum'), first=('hour', 'min'),
                                                           mean_hour=('hour', lambda h: np.average(h, weights=s.loc[h.index, 'units']))).unstack('who')
day.columns = [f'{a}_{b}' for a, b in day.columns]
day = day.dropna()
day = day[(day.units_ours >= 3) & (day.units_opp >= 3)]
day['p_ours'] = day.rev_ours / day.units_ours
day['p_opp'] = day.rev_opp / day.units_opp
day['we_first'] = day.first_ours < day.first_opp
r = day.reset_index()
r['prod'] = r['product'].map(dict(enumerate(P)))
t = r.groupby('prod').agg(days=('p_ours', 'size'), p_ours=('p_ours', 'mean'), p_opp=('p_opp', 'mean'),
                         we_first=('we_first', 'mean'), hour_ours=('mean_hour_ours', 'mean'), hour_opp=('mean_hour_opp', 'mean'),
                         units_ours=('units_ours', 'mean'), units_opp=('units_opp', 'mean'))
t['price_gap'] = t.p_ours - t.p_opp
print(t.round(2).to_string())
