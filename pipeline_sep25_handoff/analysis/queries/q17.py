import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
from analyze import load, perspectives
from compare import source
parts = [load('ledger/exp12'), load('ledger/ours?')]
data = {k: pd.concat([x[k] for x in parts], ignore_index=True) for k in parts[0]}
g, d = perspectives(data, source)
keep = ['ours:vadim6:lb', 'ours:x1_d0:lb', 'ours:x2_melon:lb']
g = g[g.source.isin(keep)]
for day in (1, 6, 7, 9, 12, 15):
    g[f'animals_d{day}'] = sum(g[f'd{day}_animals_{a}'] for a in ['goose', 'cow', 'sheep']) if f'd{day}_animals_goose' in g else np.nan
g['animals_d1'] = g.d0_animals_goose * 0 + d[d.day == 1].groupby(['trace', 'seat'])[['animals_goose', 'animals_cow', 'animals_sheep']].sum().sum(axis=1).reindex(pd.MultiIndex.from_frame(g[['trace', 'seat']])).values
cols = ['margin', 'money', 'opp_money', 'revenue', 'spend', 'animals_d1', 'd3_animals_sheep', 'animals_d6', 'animals_d9', 'animals_d12', 'animals_d15',
        'planted_melon', 'sold_melon', 'price_melon', 'rev_melon', 'rev_wool', 'rev_milk', 'rev_egg', 'rev_fertilizer', 'hire_cost', 'd6_money_dawn', 'd1_money_dawn' if 'd1_money_dawn' in g else 'd3_money_dawn']
print(g.groupby('source')[cols].mean().reindex(keep).T.round(1).to_string())
