import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
from analyze import P, C, A, COST
c = pd.read_pickle('cache_v1.pkl'); d = c['d'].copy()
d['grp'] = np.where(d.source.str.startswith('top'), 'top', d.source)
keep = ['top', 'ours:vadim6:lb', 'ours:v12:lb', 'opp:arsgorynich-herd-safe-v3']
d = d[d.grp.isin(keep)]
d['spend_animals'] = sum(d[f'bought_{a}'] * COST[a] for a in A)
d['spend_seeds'] = d[[f'seed_cost_{x}' for x in C]].sum(axis=1)
cols = ['money_dawn', 'revenue'] + [f'rev_{p}' for p in P] + ['spend_animals', 'spend_seeds', 'land_cost', 'hire_cost', 'buy_cost_wheat', 'buy_cost_fertilizer', 'money_end',
        'shed_dawn', 'stock_value_dawn', 'workers', 'harvested_fertilizer', 'sold_fertilizer', 'harvested_wheat', 'sold_wheat', 'bought_wheat']
for day in (0, 5, 6, 7, 9):
    t = d[d.day == day].groupby('grp')[cols].mean().reindex(keep).T
    print(f'\n== day {day}'); print(t.round(0).to_string())
