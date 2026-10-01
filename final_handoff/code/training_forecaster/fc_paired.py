"""Episode-paired comparison of two forecasters (fc_eval.py outputs on the same rows): per opponent group and decision hour, the mean over
episodes of (loss_B - loss_A) with its SE, where each episode's loss is its mean Poisson loss of band totals per product-day (days 12-27).
Negative = B better. usage: fc_paired.py <eval_A.csv> <eval_B.csv> [decisions, default 0 6 12 18]"""
import sys
import numpy as np
import pandas as pd


def per_episode(f):
    d = pd.read_csv(f); d = d[d.day.between(12, 27) & (d.decision < 100)]
    p = np.clip(d[[f'pred_b{k}' for k in range(4)]].values, 1e-6, None); a = d[[f'act_b{k}' for k in range(4)]].values
    d['loss'] = np.where(~np.isnan(a), p - np.nan_to_num(a) * np.log(p), 0).sum(1)
    return d.groupby(['group', 'decision', 'trace', 'target']).loss.mean()


a, b = per_episode(sys.argv[1]), per_episode(sys.argv[2])
decs = [int(x) for x in sys.argv[3:]] or [0, 6, 12, 18]
diff = (b - a).dropna().reset_index()
name = lambda f: f.rsplit('/', 1)[-1].replace('eval_', '').replace('.csv', '')
print(f'{name(sys.argv[2])} - {name(sys.argv[1])}, episode-paired mean (SE), days 12-27; negative = {name(sys.argv[2])} better')
for g, x in diff.groupby('group'):
    cells = []
    for dec in decs:
        y = x[x.decision == dec].loss
        cells.append(f'D{dec:<2d} {y.mean():+.4f} ({y.std() / np.sqrt(len(y)):.4f})')
    print(f'  {g:8s} n {len(x[x.decision == decs[0]]):4d}  ' + '   '.join(cells))
