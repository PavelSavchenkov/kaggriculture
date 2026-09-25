import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v1.pkl'); an, g = c['animals'].copy(), c['g']
an['grp'] = np.where(an.source.str.startswith('top'), 'top', an.source)
keep = ['top', 'ours:vadim6:lb', 'ours:v12:lb', 'ours:vadim6:self']
an = an[an.grp.isin(keep)]
price = g.copy(); price['grp'] = np.where(price.source.str.startswith('top'), 'top', price.source)
pp = {p: price.groupby('grp')[f'price_{p}'].mean() for p in ['egg', 'milk', 'wool', 'wheat']}
prod = {'goose': 'egg', 'cow': 'milk', 'sheep': 'wool'}
an['value'] = [r.collected * pp[prod[r.species]][r.grp] - r.fed * pp['wheat'][r.grp] for r in an.itertuples()]
an['phase'] = pd.cut(an.day, [-1, 0, 5, 6, 8, 11, 14, 29], labels=['d0', 'd1-5', 'd6', 'd7-8', 'd9-11', 'd12-14', 'd15+'])
t = an.groupby(['species', 'phase', 'grp'], observed=True).agg(n=('day', 'size'), collected=('collected', 'mean'), fed=('fed', 'mean'), cared=('cared', 'mean'),
     cap_lost=('cap_lost', 'mean'), bonus_lost=('bonus_lost', 'mean'), escaped=('escaped', lambda s: (s >= 0).mean()), esc_day=('escaped', lambda s: s[s >= 0].mean()),
     fert=('fert', 'mean'), value=('value', 'mean'))
print(t.round(2).to_string())
