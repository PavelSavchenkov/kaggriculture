import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v1.pkl'); g, d, an = c['g'], c['d'], c['animals']
g = g.copy(); g['team'] = np.where(g.source.str.startswith('top'), g.label.str.split('|').str[0], g.source)
d = d.merge(g[['trace', 'seat', 'team']], on=['trace', 'seat'])
d['animals'] = d[['animals_goose', 'animals_cow', 'animals_sheep']].sum(axis=1)
keep = ['DSM', 'DECEM', 'Majkel1337', 'Vadim_Vasilenko', 'Unknown_Mother-Goose', 'M_&_M_&_P_&_Q', 'ours:vadim6:lb', 'ours:v12:lb', 'ours:vadim6:self', 'ours:vadim6:cpp', 'ours:v12:cpp']
dd = d[d.team.isin(keep)]
for col in ['animals', 'animals_goose', 'animals_cow', 'animals_sheep']:
    t = dd[dd.day.isin([1, 3, 6, 7, 9, 10, 12, 15, 20, 25, 28])].pivot_table(index='team', columns='day', values=col, aggfunc='mean').reindex(keep)
    print(f'\n{col} at dawn'); print(t.round(1).to_string())
top = g[g.source.str.startswith('top')]
top['animals12'] = top[['d12_animals_goose', 'd12_animals_cow', 'd12_animals_sheep']].sum(axis=1)
print('\ntop: win rate by animals at day 12 (bins)')
print(top.groupby(pd.cut(top.animals12, [0, 12, 15, 18, 20, 22, 25, 40])).agg(n=('win', 'size'), win=('win', 'mean'), money=('money', 'mean')).round(2))
