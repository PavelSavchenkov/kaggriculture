import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v1.pkl'); d = c['d'].copy()
d['grp'] = np.where(d.source.str.startswith('top'), 'top', d.source)
keep = ['top', 'ours:vadim6:lb', 'ours:v12:lb', 'ours:vadim6:self']
d = d[d.grp.isin(keep)]
d['idle_share'] = d.act_idle / d.unit_turns
d['work'] = d.unit_turns - d.act_idle - d.act_move
for col in ['workers', 'hire_cost', 'idle_share', 'act_idle', 'work', 'act_move']:
    t = d.pivot_table(index='grp', columns='day', values=col, aggfunc='mean').reindex(keep)
    print(f'\n{col}'); print(t.round(2 if col == 'idle_share' else 1).to_string())
print('\nworkers distribution days 12-28:')
x = d[(d.day >= 12) & (d.day <= 28)]
print(pd.crosstab(x.grp, x.workers.clip(upper=15), normalize='index').round(3).to_string())
