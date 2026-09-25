import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
from analyze import load, perspectives, P, A
data = load('ledger/zoosales')
g, d = perspectives(data, lambda l, o, gr: l)
g['cand'] = g.group.str.split('/').str[0]; g['model'] = g.group.str.split('/').str[1]
g['side'] = np.where(g.label.str.startswith('ours'), 'ours', 'opp')
for day in (6, 9, 15):
    g[f'an{day}'] = sum(g[f'd{day}_animals_{a}'] for a in A)
g = g.set_index(['trace', 'seat'])
ours = g[g.side == 'ours'].copy()
opp = g[g.side == 'opp'].copy(); opp.index = pd.MultiIndex.from_arrays([opp.index.get_level_values(0), 1 - opp.index.get_level_values(1)])
o = opp.reindex(ours.index)
x = ours[(ours.cand == 'x10') & (ours.model != 'cand_v12_vadim6')].copy()
ox = o.loc[x.index]
x['lost'] = x.margin < 0
cols = ['an6', 'an9', 'an15', 'revenue', 'spend', 'hire_cost', 'sold_h0', 'sold_h18', 'price_melon'] + [f'rev_{p}' for p in P]
diff = x[cols] - ox[cols].values
diff['lost'] = x.lost; diff['margin'] = x.margin; diff['model'] = x.model
print('X10 in zoo (6 opponents): ours - opponent, lost vs won')
print(diff.groupby('lost')[['margin'] + cols].mean().round(0).T.to_string())
print('\nlost games by opponent:', diff[diff.lost].model.value_counts().to_dict())
print('\nvs zoo_dsm only (ours - opponent), lost vs won:')
print(diff[diff.model == 'zoo_dsm'].groupby('lost')[['margin'] + cols].mean().round(0).T.to_string())
print('\ncorrelation with margin (X10 zoo games):'); print(diff.drop(columns=['lost', 'model']).corr()['margin'].round(2).sort_values().to_string())

print('\n--- wool/milk/strawberry: units and price, ours vs opponent, X10 zoo lost vs won')
for p in ['wool', 'milk', 'strawberry', 'egg']:
    t = pd.DataFrame({'lost': x.lost, 'units_ours': x[f'sold_{p}'], 'units_opp': ox[f'sold_{p}'].values, 'price_ours': x[f'price_{p}'], 'price_opp': ox[f'price_{p}'].values,
                      'animals_ours': x['d15_animals_sheep' if p == 'wool' else 'd15_animals_cow' if p == 'milk' else 'd15_animals_goose'] if p != 'strawberry' else x['planted_strawberry'],
                      'animals_opp': (ox['d15_animals_sheep' if p == 'wool' else 'd15_animals_cow' if p == 'milk' else 'd15_animals_goose'] if p != 'strawberry' else ox['planted_strawberry']).values})
    print(p); print(t.groupby('lost').mean().round(1).to_string())
