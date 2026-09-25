import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v2.pkl'); d = c['d'].copy()
keep = ['ours:vadim6:lb', 'ours:majkel:lb', 'ours:w384:lb', 'ours:v12:lb']
d = d[d.source.isin(keep)]
d['animals'] = d[['animals_goose', 'animals_cow', 'animals_sheep']].sum(axis=1)
d['bought_animals'] = d[['bought_goose', 'bought_cow', 'bought_sheep']].sum(axis=1)
d['planted'] = d[['planted_wheat', 'planted_carrot', 'planted_tomato', 'planted_strawberry', 'planted_melon']].sum(axis=1)
for col in ['animals', 'bought_animals', 'bought_sheep', 'bought_cow', 'bought_goose', 'planted_melon', 'planted_strawberry', 'planted', 'money_dawn', 'revenue', 'land']:
    t = d[d.day <= 12].pivot_table(index='source', columns='day', values=col, aggfunc='mean').reindex(keep)
    print(f'\n{col}'); print(t.round(1).to_string())
