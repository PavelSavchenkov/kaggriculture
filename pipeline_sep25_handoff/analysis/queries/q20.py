import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
from compare import source
d = pd.concat([pd.read_csv('ledger/geo_top_days.csv'), pd.read_csv('ledger/geo_ours_days.csv')])
d['src'] = [source(l, o, g) for l, o, g in zip(d.label, d.opp_label, d.group)]
d = d[d.src.notna()]
d['grp'] = np.where(d.src.str.startswith('top'), 'top', d.src)
keep = ['top', 'ours:vadim6:lb', 'ours:v12:lb', 'ours:w384:lb', 'ours:vadim6:self']
d = d[d.grp.isin(keep)]
d['animal_mean_dist'] = d.animal_dist / d.animal_n.replace(0, np.nan)
d['crop_mean_dist'] = d.crop_dist / d.crop_n.replace(0, np.nan)
d['placed_n'] = d[['placed_goose', 'placed_cow', 'placed_sheep']].sum(axis=1)
d['planted_n'] = d[[f'planted_{c}' for c in ['wheat', 'carrot', 'tomato', 'strawberry', 'melon']]].sum(axis=1)
for col in ['animal_mean_dist', 'crop_mean_dist']:
    print(f'\n{col} at dawn'); print(d[d.day.isin([1, 3, 7, 9, 10, 12, 15, 20, 25])].pivot_table(index='grp', columns='day', values=col, aggfunc='mean').reindex(keep).round(2).to_string())
s = d.groupby('grp')[['placed_dist', 'placed_n', 'planted_dist', 'planted_n']].sum()
s['placed_mean'] = s.placed_dist / s.placed_n; s['planted_mean'] = s.planted_dist / s.planted_n
print('\nmean shed distance of new placements (whole game)'); print(s[['placed_mean', 'planted_mean']].reindex(keep).round(2).to_string())
# placement distance by day of placement
d = d.reset_index(drop=True); d['bucket'] = pd.cut(d.day, [-1, 0, 5, 8, 11, 29]).astype(str)
x = d[d.placed_n > 0].groupby(['grp', 'bucket']).apply(lambda z: z.placed_dist.sum() / z.placed_n.sum()).unstack()
print('\nnew animal placement distance by day range'); print(x.reindex(keep).round(2).to_string())
for col in ['seeds_wheat', 'seeds_carrot', 'seeds_strawberry', 'seeds_tomato', 'seeds_melon', 'shed_wheat', 'shed_fertilizer']:
    print(f'\n{col} at dawn'); print(d[d.day.isin([1, 3, 6, 9, 12, 15, 20, 25])].pivot_table(index='grp', columns='day', values=col, aggfunc='mean').reindex(keep).round(1).to_string())
