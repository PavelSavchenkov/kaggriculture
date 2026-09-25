"""Gap 3: are units held overnight worth holding? Per (game, seat, day d, product), held units H =
shed stock at dawn d+1; realized = average price of the first H units sold on day d+1 (in hour
order); evening = average price of that seat's last selling hour on day d (the marginal alternative).
Ratio realized/evening > 1/0.95 means holding paid more than the DP's hold discount requires."""
import sys
import pandas as pd, numpy as np
prefix = sys.argv[1] if len(sys.argv) > 1 else 'hold/h'
P = ['wheat','carrot','tomato','strawberry','melon','egg','milk','wool','fertilizer']
days = pd.read_csv(prefix + '_days.csv')
sales = pd.read_csv(prefix + '_sales.csv')
sales = sales[sales.units > 0]
days['who'] = np.where(days.label.str.startswith('ours'), 'ours', np.where(days.group == 'top', 'top', 'opp'))
top_mask = days.group == 'top'
# top perspectives: label ends with |rank; keep all seats of top games as 'top' (both are top-10 teams)
shed = days.set_index(['trace', 'seat', 'day'])
S = {k: g.sort_values('hour') for k, g in sales.groupby(['trace', 'seat', 'day', 'product'])}
rows = []
for (trace, seat, day), r in shed.iterrows():
    if not (5 <= day <= 27): continue
    for q, name in enumerate(P):
        if name in ('wheat', 'fertilizer'): continue
        nxt = (trace, seat, day + 1)
        if nxt not in shed.index: continue
        H = int(shed.loc[nxt, 'shed_' + name])
        ev = S.get((trace, seat, day, q))
        tm = S.get((trace, seat, day + 1, q))
        if H <= 0 or ev is None or tm is None: continue
        last = ev.iloc[-1]
        evening = last.revenue / last.units
        need, got, units, morning = H, 0.0, 0, 0
        for _, s in tm.iterrows():
            take = min(need, s.units)
            got += take * s.revenue / s.units; units += take; need -= take
            if s.hour < 12: morning += take
            if need <= 0: break
        if units == 0: continue
        rows.append(dict(who=r.who, product=name, day=day, H=H, sold=units, morning=morning, realized=got / units,
                         evening=evening, ev_hour=last.hour))
d = pd.DataFrame(rows)
d['gain'] = (d.realized - d.evening) * d.sold
g = d.groupby(['who', 'product'])
out = pd.DataFrame({'cases': g.size(), 'held/day': g.H.mean(), 'sold_next%': 100 * g.sold.sum() / g.H.sum(),
                    'morning%': 100 * g.morning.sum() / g.sold.sum(),
                    'realized': g.apply(lambda x: (x.realized * x.sold).sum() / x.sold.sum()),
                    'evening': g.apply(lambda x: (x.evening * x.sold).sum() / x.sold.sum()),
                    'ev_hour': g.ev_hour.median()})
out['ratio'] = out.realized / out.evening
out['gain/unit'] = out.realized - out.evening
print(out.round(2).to_string())
print(d.groupby('who').apply(lambda x: pd.Series({'units': x.sold.sum(), 'gain': x.gain.sum() / x.groupby([]).ngroups if False else x.gain.sum()})))
