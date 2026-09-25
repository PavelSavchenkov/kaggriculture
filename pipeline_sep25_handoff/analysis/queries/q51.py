"""Labour: where does the wage gap come from? Days 10-28. Sources: ours:kaggle (Kaggle submission),
ours:x13 (Local-LB), ours:base (C++ mirror), top (top-10 perspectives).
Parts: workload (productive actions per day), actions per worker-turn, hire timing (hour of each
hire, turns and actions per hire by index), unevenness (wage at the game's mean hires per day)."""
import numpy as np, pandas as pd
FIB = [1, 1]
while len(FIB) < 40: FIB.append(FIB[-1] + FIB[-2])
CUM = np.cumsum([0] + FIB)
def wage(h):  # fractional hires: interpolate the cumulative Fibonacci cost
    lo = int(np.floor(h)); return CUM[lo] + (h - lo) * FIB[lo]
u = pd.read_csv('labor/l_units.csv')
u['rank'] = u.label.str.extract(r'\|(\d+)$')[0].astype(float)
u['src'] = np.where(u.label.str.startswith('ours'), u.label, np.where(u['rank'] <= 10, 'top', None))
u = u[u.src.notna() & (u.day >= 10) & (u.day <= 28)]
day = u.groupby(['src', 'trace', 'seat', 'day']).agg(workers=('unit', 'size'), turns=('turns', 'sum'), work=('work', 'sum'),
                                                       moves=('moves', 'sum'), wage=('wage', 'sum')).reset_index()
day['hires'] = day.workers - 1
day['idle'] = day.turns - day.work - day.moves
S = day.groupby('src')
t = pd.DataFrame({'games': S.trace.nunique() , 'hires': S.hires.mean(), 'wage': S.wage.mean(), 'work': S.work.mean(),
                  'turns': S.turns.mean(), 'moves': S.moves.mean(), 'idle': S.idle.mean()})
t['work/turn'] = t.work / t.turns
t['move/work'] = t.moves / t.work
t['$/work'] = t.wage / t.work
print('Per day, days 10-28'); print(t.round(2).to_string())
# Unevenness: actual wage vs wage at each game's mean hires/day.
g = day.groupby(['src', 'trace', 'seat']).agg(hires=('hires', 'mean'), wage=('wage', 'mean'), work=('work', 'mean'),
                                              work_sd=('work', 'std'), hires_sd=('hires', 'std')).reset_index()
g['even_wage'] = g.hires.map(wage)
G = g.groupby('src')
print('\nUnevenness: wage/day actual vs spread evenly at the same hires; daily sd of hires and work')
print(pd.DataFrame({'actual': G.wage.mean(), 'even': G.even_wage.mean(), 'hires_sd': G.hires_sd.mean(),
                    'work_sd': G.work_sd.mean(), 'work_cv': (g.work_sd / g.work).groupby(g.src).mean()}).round(2).to_string())
# Hire timing and output per hire index.
h = u[u.unit > 0].copy()
h['k'] = h.unit  # k-th hire of the day
h['bucket'] = pd.cut(h.hire_hour, [-1, 0, 5, 11, 17, 23], labels=['h0', 'h1-5', 'h6-11', 'h12-17', 'h18-23'])
print('\nHire hour shares by hire index (k-th hire of the day)')
print((pd.crosstab([h.src, h.k.clip(upper=13)], h.bucket, normalize='index') * 100).round(0).loc[:, :].unstack(0).T.to_string() if False else
      (pd.crosstab([h.src, h.k.clip(upper=13)], h.bucket, normalize='index') * 100).round(0).to_string())
print('\nPer hire index: share of days with that hire, turns, productive actions, wage per action')
k = h.groupby(['src', 'k']).agg(n=('k', 'size'), turns=('turns', 'mean'), work=('work', 'mean'), moves=('moves', 'mean'), wage=('wage', 'mean'))
k['days_share'] = k.n / day.groupby('src').size().reindex(k.index.get_level_values('src')).values
k['$/action'] = k.wage / k.work
print(k[k.index.get_level_values('k') >= 8].round(2).to_string())
