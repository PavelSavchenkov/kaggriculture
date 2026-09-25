import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v2.pkl'); d = c['d'].copy()
d['grp'] = np.where(d.source.str.startswith('top'), 'top', d.source)
keep = ['top', 'ours:vadim6:lb', 'ours:v12:lb']
d = d[d.grp.isin(keep)]
print(d[d.day.isin([10, 12, 14, 16, 18, 20, 22, 24, 26, 28])].pivot_table(index='grp', columns='day', values='shed_dawn', aggfunc='mean').reindex(keep).round(1).to_string())
print('\nnight carried units per night (days 10-27):'); print(d[(d.day >= 10) & (d.day <= 27)].groupby('grp').night_carried.mean().reindex(keep).round(1))
print('\nsold at hours 0-5 per day (days 10-27):'); print(d[(d.day >= 10) & (d.day <= 27)].groupby('grp').sold_h0.mean().reindex(keep).round(1))
