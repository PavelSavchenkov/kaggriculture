import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v2.pkl'); g = c['g'].copy()
g = g.set_index(['trace', 'seat'])
opp = g.copy(); opp.index = pd.MultiIndex.from_arrays([opp.index.get_level_values(0), 1 - opp.index.get_level_values(1)])
items = {'cow': 'd15_animals_cow', 'sheep': 'd15_animals_sheep', 'goose': 'd15_animals_goose', 'strawberry': 'planted_strawberry',
         'tomato': 'planted_tomato', 'carrot': 'planted_carrot', 'melon': 'planted_melon'}
def within(df, key):
    rows = {}
    for name, col in items.items():
        x = df[col]; y = opp.reindex(df.index)[col]
        ok = y.notna()
        x, y, k = x[ok], y[ok], df.loc[ok, key]
        xd = x - x.groupby(k).transform('mean'); yd = y - y.groupby(k).transform('mean')
        rows[name] = np.corrcoef(xd, yd)[0, 1]
    return pd.Series(rows)
top = g[g.source.isin(['top', 'top_vs20'])].copy(); top['team'] = top.label.str.split('|').str[0]
# opponent must be a top-list team too (its row exists in the ledger for both seats of top traces)
res = {'top teams (within team, opponent any)': within(top, 'team')}
ours = g[g.source.str.match(r'ours:(vadim6|v12|w384):lb')].copy(); ours['k'] = ours.source
res['ours vs Local-LB (within variant)'] = within(ours, 'k')
print('correlation of own count with the opponent count (demeaned within team/variant); negative = avoid the opponent\'s mix')
print(pd.DataFrame(res).round(3).to_string())
# does opponent mix predict own mix in the first days? (decision happens early: animals by day 12, crops by planting)
