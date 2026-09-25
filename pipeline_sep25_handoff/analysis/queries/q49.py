"""Kaggle games vs top teams: hires per day (distribution), wage per day, fertilizer missed by day."""
import pandas as pd, numpy as np
fr = []
for pre in ['ledger/kaggle', 'ledger/top0', 'ledger/top1']:
    d = pd.read_csv(pre + '_days.csv', usecols=['trace', 'seat', 'label', 'day', 'hires', 'hire_cost', 'fert_missed', 'workers'])
    fr.append(d)
d = pd.concat(fr, ignore_index=True)
d['who'] = np.where(d.label.str.startswith('ours'), 'ours', np.where(d.label.str.contains(r'\|'), 'top', 'opp'))
d = d[d.who != 'opp']
d['rank'] = d.label.str.extract(r'\|(\d+)$')[0].astype(float)
d = d[(d.who == 'ours') | (d['rank'] <= 10)]
x = d[(d.day >= 10) & (d.day <= 28)].reset_index(drop=True)
print('hires per day, days 10-28 (share of days)')
print(pd.crosstab(x.who, x.hires.clip(8, 14), normalize='index').round(3))
print(x.groupby('who')[['hires', 'hire_cost', 'fert_missed']].mean().round(2))
print(d.pivot_table(index='day', columns='who', values='hire_cost', aggfunc='mean').round(0).T.to_string())
print(d.pivot_table(index='day', columns='who', values='fert_missed', aggfunc='mean').round(1).T.to_string())
