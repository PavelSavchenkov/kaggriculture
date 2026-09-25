import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
from analyze import P
c = pd.read_pickle('cache_v2.pkl'); g = c['g'].copy()
ours = g[g.source.str.match(r'ours:(vadim6|v12|w384|v11|majkel):lb')].copy()
opp = g[g.source.str.startswith('opp:')].set_index(['trace', 'seat'])
ours['oseat'] = 1 - ours.seat
o = opp.reindex(pd.MultiIndex.from_arrays([ours.trace, ours.oseat]))
for p in P:
    ours[f'gap_{p}'] = ours[f'rev_{p}'].values - o[f'rev_{p}'].values
ours['gap_spend'] = -(ours.spend.values - o.spend.values)
ours['animals15'] = ours[['d15_animals_goose', 'd15_animals_cow', 'd15_animals_sheep']].sum(axis=1)
ours['opp_animals15'] = (o[['d15_animals_goose', 'd15_animals_cow', 'd15_animals_sheep']].sum(axis=1)).values
ours['lost'] = ours.margin < 0
cols = ['margin', 'animals15', 'opp_animals15'] + [f'gap_{p}' for p in P] + ['gap_spend']
print('mean revenue gap (ours - opponent) by product, lost vs won games, Local-LB, all 5 variants')
print(ours.groupby('lost')[cols].mean().T.round(0).to_string())
print('\nn lost', ours.lost.sum(), 'of', len(ours))
print('\nlost games by opponent:'); print(ours[ours.lost].opp_label.value_counts())
print('\ncorrelation of margin with gaps:'); print(ours[cols].corr()['margin'].round(2).to_string())
