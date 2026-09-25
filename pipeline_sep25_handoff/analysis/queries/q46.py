import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
from compare import source
t = pd.concat([pd.read_csv('ledger/trip_top.csv'), pd.read_csv('ledger/trip_ours.csv')], ignore_index=True)
t['src'] = [source(l, o, g) for l, o, g in zip(t.label, t.opp_label, t.group)]
t = t[t.src.notna()]
t['grp'] = np.where(t.src.str.startswith('top'), 'top', 'ours (baseline LB)')
m = t[(t.day >= 11) & (t.day <= 27) & (t.ends_at_shed == 1)]
games = t.groupby('grp').apply(lambda x: x[['trace', 'seat']].drop_duplicates().shape[0]) * 17
m['bucket'] = pd.cut(m.end_hour, [-1, 5, 9, 13, 17, 19, 21, 23], labels=['0-5', '6-9', '10-13', '14-17', '18-19', '20-21', '22-23'])
dep = m.pivot_table(index='grp', columns='bucket', values='cargo_end', aggfunc='sum').div(games, axis=0)
print('units deposited per game-day by hour of deposit (days 11-27)'); print(dep.round(1).to_string())
print('\nshare:'); print(dep.div(dep.sum(axis=1), axis=0).round(2).to_string())
