import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
from compare import source
t = pd.concat([pd.read_csv('ledger/trip_top.csv'), pd.read_csv('ledger/trip_ours.csv')], ignore_index=True)
t['src'] = [source(l, o, g) for l, o, g in zip(t.label, t.opp_label, t.group)]
t = t[t.src.notna()]
t['grp'] = np.where(t.src.str.startswith('top'), 'top', 'ours')
m = t[(t.day >= 11) & (t.day <= 27)].copy()
m['work'] = m.actions > 0
games = m.groupby('grp').apply(lambda x: x[['trace', 'seat']].drop_duplicates().shape[0])
days = games * 17
w = m[m.work]
print('trips with farm work, per game-day:'); print((w.groupby('grp').size() / days).round(1))
print('trips without farm work (pure shed runs / walks), per game-day:'); print((m[~m.work & (m.moves > 0)].groupby('grp').size() / days).round(1))
cols = ['moves', 'actions', 'tiles', 'mst', 'animal_actions', 'crop_actions', 'cargo_end']
print('\nper working trip (means):'); print(w.groupby('grp')[cols].mean().round(2).T.to_string())
w = w.assign(detour=w.moves / w.mst.replace(0, np.nan), moves_per_tile=w.moves / w.tiles)
print('\nmoves / trip MST (detour over the tour lower bound), moves per tile:'); print(w.groupby('grp')[['detour', 'moves_per_tile']].median().round(2).T.to_string())
print('\ntrip size distribution (tiles per working trip):'); print(pd.crosstab(w.grp, w.tiles.clip(upper=12), normalize='index').round(3).to_string())
w['mix'] = np.where((w.animal_actions > 0) & (w.crop_actions > 0), 'both', np.where(w.animal_actions > 0, 'animals', 'crops'))
print('\ntrip type shares:'); print(pd.crosstab(w.grp, w.mix, normalize='index').round(3).to_string())
print('\nmean moves / tiles by trip type:'); print(w.groupby(['grp', 'mix'])[['moves', 'tiles', 'mst', 'actions']].mean().round(2).to_string())
# worker-day level
wd = m.groupby(['grp', 'trace', 'seat', 'day', 'worker']).agg(trips=('work', 'sum'), moves=('moves', 'sum'), actions=('actions', 'sum'), start=('start_hour', 'min'), end=('end_hour', 'max'))
print('\nper worker-day:'); print(wd.groupby('grp')[['trips', 'moves', 'actions', 'start']].mean().round(2).T.to_string())
