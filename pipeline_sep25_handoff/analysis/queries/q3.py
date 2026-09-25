import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v1.pkl'); g, d = c['g'], c['d']
d = d.copy(); d['grp'] = np.where(d.source.str.startswith('top'), 'top', d.source)
keep = ['top', 'ours:vadim6:lb', 'ours:v12:lb', 'ours:vadim6:self', 'opp:ahmed-productive-wheat-v54', 'opp:arsgorynich-herd-safe-v3', 'opp:latest-yannik-suffix-5d-28s-v1']
d = d[d.grp.isin(keep)]
n = d.groupby('grp').apply(lambda x: x[['trace', 'seat']].drop_duplicates().shape[0])
for col in ['sold_melon', 'harvested_melon', 'rev_melon']:
    t = d[(d.day >= 9) & (d.day <= 16)].pivot_table(index='grp', columns='day', values=col, aggfunc='sum').div(n, axis=0).reindex(keep)
    print(f'\n{col} per game by day'); print(t.round(1).to_string())
d['p'] = d.rev_melon / d.sold_melon
print('\nmelon avg price by day'); print(d[(d.day >= 9) & (d.day <= 16)].groupby(['grp', 'day']).apply(lambda x: x.rev_melon.sum() / max(1, x.sold_melon.sum())).unstack().reindex(keep).round(0).to_string())
print('\nmelon shed stock at dawn'); print(d[(d.day >= 10) & (d.day <= 16)].pivot_table(index='grp', columns='day', values='shed_melon', aggfunc='mean').reindex(keep).round(1).to_string())
