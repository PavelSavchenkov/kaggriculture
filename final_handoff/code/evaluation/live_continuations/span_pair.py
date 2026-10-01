"""Multi-day span continuations paired (teacher_day TEACHER_DAYS=k csvs <set>_<arm>_<d0>.csv): arm B minus arm A at dawn start + k,
symmetric values (ours: cash + shed at marginal prices + standing yield + held; opponent: the same), mean per game-day with SE
across games, overall and by the opponent's rank band. usage: span_pair.py <run dir> <set> <arm A> <arm B> <rank list: trace seat rank>"""
import glob, os, sys
import numpy as np, pandas as pd
d, st, A, B, lst = sys.argv[1:6]
rank = {os.path.basename(l.split()[0])[:-4]: float(l.split()[2]) for l in open(lst)}
band = lambda r: 'top10' if r <= 10 else '11-30' if r <= 30 else '31-100' if r <= 100 else '101+'
def load(a):
    x = pd.concat([pd.read_csv(f) for f in glob.glob(f'{d}/{st}_{a}_*.csv')])
    x = x[x.who == 'ours'].copy()
    x['ep'] = x.trace.str.rsplit('/', n=1).str[1].str[:-4]
    x['val'] = x.value_next_m + x.field_value_next + x.held_value_next
    x['oval'] = x.opp_value_next_m + x.opp_field_value_next + x.opp_held_value_next
    return x.set_index(['ep', 'day'])
a, b = load(A), load(B)
k = a.index.intersection(b.index)
df = pd.DataFrame({'dv': b.loc[k].val - a.loc[k].val, 'do': b.loc[k].oval - a.loc[k].oval}).reset_index()
df['dm'] = df.dv - df.do
g = df.groupby('ep')[['dv', 'do', 'dm']].mean(); g['band'] = [band(rank.get(e, 999)) for e in g.index]
ms = lambda s: f'{s.mean():+.0f} (SE {s.std(ddof=1) / np.sqrt(len(s)):.0f})' if len(s) > 1 else f'{s.mean():+.0f}'
print(f'set {st}: {B} - {A}: {len(g)} games, {len(df)} game-days (starts {sorted(df.day.unique())}); per game-day at the span end: '
      f'margin {ms(g.dm)}, own {ms(g.dv)}, opp {ms(g.do)}; changed {(df.dm.abs() > 0.5).mean():.0%}')
for bd in ['top10', '11-30', '31-100', '101+']:
    s = g[g.band == bd]
    if len(s): print(f'   {bd:7s} n {len(s):3d}: margin {ms(s.dm)}, own {ms(s.dv)}, opp {ms(s.do)}')
