import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v1.pkl'); g = c['g']
top = g[g.source.isin(['top', 'top_vs20'])].copy()
top['team'] = top.label.str.split('|').str[0]
top['land_final'] = top['d29_land']
print('top land at day 29 distribution:\n', top.land_final.value_counts(normalize=True).sort_index().round(3))
print('\nby team: share with 4 quadrants, mean 4th-quadrant day, win rate by land')
t = top.groupby('team').agg(n=('win', 'size'), q4=('land_final', lambda s: (s == 4).mean()), q4_day=('land4_day', 'mean'), win=('win', 'mean'), money=('money', 'mean'))
print(t.round(2))
print('\nwin rate and money by final land (top perspectives):')
print(top.groupby('land_final').agg(n=('win', 'size'), win=('win', 'mean'), money=('money', 'mean'), margin=('margin', 'mean'), opp=('opp_money','mean')).round(2))
print('\n4th quadrant day histogram:'); print(top.land4_day.value_counts().sort_index().head(20))
