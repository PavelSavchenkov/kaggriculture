import pandas as pd, numpy as np
t = pd.read_csv('ledger/trip_bundle.csv')
t = t[t.label.str.startswith('ours') & (t.day >= 11) & (t.day <= 27)]
t['tag'] = t.group.str.split('/').str[0]
w = t[t.actions > 0].copy()
w['mix'] = np.where((w.animal_actions > 0) & (w.crop_actions > 0), 'both', np.where(w.animal_actions > 0, 'animals', 'crops'))
print(pd.crosstab(w.tag, w.mix, normalize='index').round(3))
print(w.groupby('tag')[['moves', 'actions', 'tiles', 'mst']].mean().round(2))
