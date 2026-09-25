import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v2.pkl'); cr, g = c['crops'].copy(), c['g']
cr['grp'] = np.where(cr.source.str.startswith('top'), 'top', cr.source)
keep = ['top', 'ours:vadim6:lb', 'ours:v12:lb', 'ours:w384:lb']
cr = cr[cr.grp.isin(keep)]
n = g.assign(grp=np.where(g.source.str.startswith('top'), 'top', g.source)).groupby('grp').size()
t = cr.groupby(['crop', 'grp'])[['n', 'harvested', 'decay_lost', 'cap_lost', 'fate_harvested', 'fate_dug', 'fate_weed_dry', 'fate_weed_decay', 'fate_end', 'ferts', 'waters']].sum()
t = t.div(t.index.get_level_values('grp').map(n), axis=0)
t['per_plant'] = t.harvested / t.n
print('per game totals'); print(t.round(2).to_string())
