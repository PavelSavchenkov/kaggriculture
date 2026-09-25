"""4th quadrant: how often and when top teams buy it, and outcomes with vs without (overall and
within the same team). Also opponents with 4 quadrants in our Kaggle games."""
import numpy as np, pandas as pd
d = pd.read_csv('labor/l_days.csv', usecols=['trace', 'seat', 'label', 'day', 'land', 'land_cost', 'money_dawn'])
g = pd.read_csv('labor/l_games.csv')
d['rank'] = d.label.str.extract(r'\|(\d+)$')[0].astype(float)
top = d[d['rank'] <= 10]
land = top.groupby(['trace', 'seat', 'label']).agg(max_land=('land', 'max'),
                                                   day4=('land', lambda s: top.loc[s.index, 'day'][s >= 4].min() if (s >= 4).any() else np.nan)).reset_index()
land = land.merge(g[['trace', 'seat', 'money', 'opp_money']], on=['trace', 'seat'])
land['margin'] = land.money - land.opp_money
land['team'] = land.label.str.split('|').str[0]
land['q4'] = land.max_land >= 4
print('top-10 perspectives:', len(land), ' share with 4th quadrant %.2f' % land.q4.mean())
print('day of 4th quadrant (quantiles):', land.day4.quantile([0.1, 0.25, 0.5, 0.75, 0.9]).round(1).to_dict())
print(land.groupby('q4')[['money', 'margin']].mean().round(0).to_string())
t = land.groupby(['team', 'q4']).agg(n=('money', 'size'), money=('money', 'mean'), margin=('margin', 'mean')).unstack('q4')
t = t[(t[('n', True)] >= 10) & (t[('n', False)] >= 10)]
t['d_money'] = t[('money', True)] - t[('money', False)]; t['d_margin'] = t[('margin', True)] - t[('margin', False)]
print('within team (>=10 games each way):'); print(t.round(0).to_string())
print('share of 4th-quadrant games by team:'); print(land.groupby('team').q4.agg(['mean', 'size']).round(2).sort_values('mean').to_string())
