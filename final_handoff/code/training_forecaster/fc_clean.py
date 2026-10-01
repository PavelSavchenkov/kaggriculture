"""Clean-set comparison: fc_compare-style Poisson loss of band totals, restricted to the groups whose episodes are disjoint from training
(the original 800: mm / top10 / r11_30 / ours), with the live mix renormalised over top10 and r11_30 (11 : 24).
usage: fc_clean.py <eval_a.csv> <eval_b.csv> ... (days 12-27)"""
import sys
import numpy as np
import pandas as pd

GROUPS = ['mm', 'top10', 'r11_30', 'ours']
MIX = {'top10': 11 / 35, 'r11_30': 24 / 35}
names = [a.rsplit('/', 1)[-1].replace('eval_', '').replace('.csv', '') for a in sys.argv[1:]]
score = {}
for n, f in zip(names, sys.argv[1:]):
    d = pd.read_csv(f); d = d[d.day.between(12, 27) & d.group.isin(GROUPS)]
    for (g, dec), x in d.groupby(['group', 'decision']):
        p = np.clip(x[[f'pred_b{k}' for k in range(4)]].values, 1e-6, None); a = x[[f'act_b{k}' for k in range(4)]].values
        score[(n, g, dec)] = float(np.where(~np.isnan(a), p - np.nan_to_num(a) * np.log(p), 0).sum(1).mean())
decs = sorted({k[2] for k in score})
print('clean 800, days 12-27, Poisson loss of band totals (lower is better); columns: ' + ' | '.join(names))
for g in GROUPS + ['mix']:
    cells = []
    for dec in decs:
        v = [sum(w * score[(n, gg, dec)] for gg, w in MIX.items()) if g == 'mix' else score[(n, g, dec)] for n in names]
        cells.append(('D' if dec < 100 else 'next c') + f"{dec % 100:<2d} " + ' '.join(f"{x:7.4f}" for x in v))
    print(f"  {g:6s} " + '   '.join(cells))
