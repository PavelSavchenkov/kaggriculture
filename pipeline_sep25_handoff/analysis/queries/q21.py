import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
from compare import source
import time, os
d = pd.concat([pd.read_csv('ledger/layout_top.csv'), pd.read_csv('ledger/layout_ours.csv')])
d['src'] = [source(l, o, g) for l, o, g in zip(d.label, d.opp_label, d.group)]
d = d[d.src.notna()]
d['grp'] = np.where(d.src.str.startswith('top'), 'top', d.src)
keep = ['top', 'ours:vadim6:lb', 'ours:v12:lb', 'ours:w384:lb', 'ours:vadim6:self']
m = d[(d.day >= 11) & (d.day <= 27) & d.grp.isin(keep)].copy()
m['mst_per_tile'] = m.mst / m.worked_tiles
m['moves_per_action'] = m.moves / m.actions
m['actions_per_tile'] = m.actions / m.worked_tiles
m['moves_per_mst'] = m.moves / m.mst
print(m.groupby('grp')[['worked_tiles', 'animal_tiles', 'crop_tiles', 'actions', 'actions_per_tile', 'moves', 'mst', 'mst_per_tile', 'mean_shed_dist', 'moves_per_action', 'moves_per_mst']].mean().reindex(keep).round(2).T.to_string())
print('\n(rows:', m.groupby('grp').size().reindex(keep).tolist(), ')')
