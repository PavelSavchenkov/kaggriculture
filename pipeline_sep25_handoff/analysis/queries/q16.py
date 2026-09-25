import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v2.pkl'); d, an = c['d'].copy(), c['animals'].copy()
for x in (d, an): x['grp'] = np.where(x.source.str.startswith('top'), 'top', x.source)
keep = ['top', 'ours:vadim6:lb', 'ours:v12:lb', 'ours:w384:lb', 'ours:majkel:lb']
d = d[d.grp.isin(keep)]; an = an[an.grp.isin(keep)]
mid = d[(d.day >= 11) & (d.day <= 27)].copy()
mid['hires_n'] = mid.workers - 1
mid['work'] = mid.unit_turns - mid.act_idle - mid.act_move
mid['coll'] = mid.act_harvest_animal
mid['harv'] = mid.act_harvest_crop
per = mid.groupby(['grp', 'trace', 'seat']).agg(h_mean=('hires_n', 'mean'), h_std=('hires_n', 'std'), wage=('hire_cost', 'mean'),
     work_mean=('work', 'mean'), work_std=('work', 'std'), coll_std=('coll', 'std'), coll=('coll', 'mean'), harv=('harv', 'mean'), harv_std=('harv', 'std'),
     move=('act_move', 'mean'), work_per_worker=('work', lambda w: w.sum()), turns=('unit_turns', 'sum'))
per['work_share'] = per.work_per_worker / per.turns
print('days 11-27, per game means:'); print(per.groupby('grp').mean().reindex(keep).drop(columns=['work_per_worker', 'turns']).round(2).T.to_string())
print('\nhires distribution days 11-27:'); print(pd.crosstab(mid.grp, mid.hires_n.clip(8, 14), normalize='index').reindex(keep).round(3).to_string())
fibsum = lambda n: sum([1,1,2,3,5,8,13,21,34,55,89,144,233,377][:n])
print('\nFibonacci day wage by hires:', {n: fibsum(n) for n in range(8, 14)})
# collection lot sizes
an['alive'] = an.fed + an.unfed
g = an.groupby(['grp', 'species']).agg(coll_per_day=('collections', 'sum'), alive=('alive', 'sum'), units=('collected', 'sum'))
g['collections_per_animal_day'] = g.coll_per_day / g.alive
g['units_per_collection'] = g.units / g.coll_per_day
print('\nanimal collection frequency and lot size'); print(g[['collections_per_animal_day', 'units_per_collection']].unstack(0).round(2).to_string())
