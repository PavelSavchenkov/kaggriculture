import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
P = ["wheat", "carrot", "tomato", "strawberry", "melon", "egg", "milk", "wool", "fertilizer"]
s = pd.read_csv('ledger/zoosales_sales.csv')
lst = {}
for line in open('ledger/zoo_list.txt'):
    f = line.split(); lst[f[0]] = (0 if f[1].startswith('ours') else 1, f[3])
s['our_seat'] = s.trace.map(lambda t: lst[t][0]); s['group'] = s.trace.map(lambda t: lst[t][1])
s = s[s.group.str.startswith('x10/') & ~s.group.str.endswith('cand_v12_vadim6')]
s['who'] = np.where(s.seat == s.our_seat, 'ours', 'opp')
import csv, glob
from pathlib import Path
margin = {}
for f in glob.glob('zoo/x10/*.csv'):
    for r in csv.DictReader(open(f)):
        margin[f"zoo/x10/{Path(f).stem}/{r['seed']}_{r['seat']}.txt"] = float(r['margin'])
s['lost'] = s.trace.map(lambda t: margin.get(t, np.nan) < 0)
day = s.groupby(['trace', 'lost', 'day', 'product', 'who']).agg(units=('units', 'sum'), rev=('revenue', 'sum'), first=('hour', 'min'),
                                                                 hour=('hour', lambda h: np.average(h, weights=s.loc[h.index, 'units']))).unstack('who')
day.columns = [f'{a}_{b}' for a, b in day.columns]
day = day.dropna()
day = day[(day.units_ours >= 3) & (day.units_opp >= 3)].reset_index()
day['gap'] = day.rev_ours / day.units_ours - day.rev_opp / day.units_opp
day['first_ours'] = (day.first_ours < day.first_opp).astype(float)
day['first_opp'] = (day.first_opp < day.first_ours).astype(float)
day['prod'] = day['product'].map(dict(enumerate(P)))
t = day[day['prod'].isin(['wool', 'milk', 'strawberry', 'egg', 'melon'])].groupby(['prod', 'lost']).agg(days=('gap', 'size'), price_gap=('gap', 'mean'), we_first=('first_ours', 'mean'),
     they_first=('first_opp', 'mean'), hour_ours=('hour_ours', 'mean'), hour_opp=('hour_opp', 'mean'))
print('X10 zoo games, product-days where both sell >= 3 units: price gap ours - opp and who sells first')
print(t.round(2).to_string())
w = day[day['prod'].isin(['wool', 'milk'])]
print('\nwool+milk: price gap when we sell first vs when they sell first:')
print(w.groupby(np.select([w.first_ours == 1, w.first_opp == 1], ['we first', 'they first'], 'same hour')).gap.agg(['size', 'mean']).round(2).to_string())
