"""Poisson loss of band totals (lower is better) per opponent group present in the files x decision hour, days 12-27; columns = files.
usage: fc_groups.py <eval_a.csv> <eval_b.csv> ..."""
import sys
import numpy as np
import pandas as pd

names = [a.rsplit('/', 1)[-1].replace('eval_', '').replace('.csv', '') for a in sys.argv[1:]]
score, groups = {}, set()
for n, f in zip(names, sys.argv[1:]):
    d = pd.read_csv(f); d = d[d.day.between(12, 27) & (d.decision < 100)]
    for (g, dec), x in d.groupby(['group', 'decision']):
        p = np.clip(x[[f'pred_b{k}' for k in range(4)]].values, 1e-6, None); a = x[[f'act_b{k}' for k in range(4)]].values
        score[(n, g, dec)] = float(np.where(~np.isnan(a), p - np.nan_to_num(a) * np.log(p), 0).sum(1).mean()); groups.add(g)
print('days 12-27, Poisson loss of band totals (lower is better); columns: ' + ' | '.join(names))
for g in sorted(groups):
    print(f"  {g:8s} " + '   '.join(f"D{dec:<2d} " + ' '.join(f"{score.get((n, g, dec), float('nan')):7.4f}" for n in names) for dec in (0, 6, 12, 18)))
