import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
s = pd.read_csv('ledger/s_lb_sales.csv')
ours = {}
grp = {}
for line in open('ledger/sales_lb_list.txt'):
    f = line.split(); ours[f[0]] = 0 if f[1].startswith('ours') else 1; grp[f[0]] = f[3]
s['ours'] = s.seat == s.trace.map(ours)
m = s[s['product'] == 4]
rows = []
for tr, x in m.groupby('trace'):
    x = x.sort_values(['day', 'hour'])
    o, p = x[x.ours], x[~x.ours]
    # Upper bound: all melon units sold in the game, ours first (same total units, same days' demand ignored).
    units_o, units_p = o.units.sum(), p.units.sum()
    rev_o, rev_p = o.revenue.sum(), p.revenue.sum()
    first_hour_o = o.day.min() * 24 + o.hour[o.day == o.day.min()].min() if len(o) else np.nan
    first_hour_p = p.day.min() * 24 + p.hour[p.day == p.day.min()].min() if len(p) else np.nan
    rows.append(dict(trace=tr, group=grp[tr], units_o=units_o, units_p=units_p, rev_o=rev_o, rev_p=rev_p, price_o=rev_o / max(1, units_o),
                     price_p=rev_p / max(1, units_p), first_o=first_hour_o, first_p=first_hour_p))
r = pd.DataFrame(rows)
r['opp'] = r.group.str.split('/').str[2]; r['v'] = r.group.str.split('/').str[0]
# Swap bound: if we had sold at the opponent's average price and it at ours (order swap, same volumes).
r['swap_gain_margin'] = (r.price_p - r.price_o) * r.units_o + (r.price_p - r.price_o) * r.units_p * 0  # own part
print(r.groupby(['v', 'opp'])[['units_o', 'units_p', 'price_o', 'price_p', 'rev_o', 'rev_p', 'first_o', 'first_p']].mean().round(0).to_string())
print('\nmean own melon revenue gap if our units got the opponent average price:',
      ((r.price_p - r.price_o) * r.units_o).mean().round(0))
