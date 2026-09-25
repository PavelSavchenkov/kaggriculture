"""All Kaggle losses of our submission in one table: ours minus opponent by cause."""
import csv, glob
import numpy as np, pandas as pd
P = ['wheat', 'carrot', 'tomato', 'strawberry', 'melon', 'egg', 'milk', 'wool', 'fertilizer']
lb = {r['TeamName']: int(r['Rank']) for r in csv.DictReader(open(glob.glob('kaggle_ours/lb/*.csv')[0], encoding='utf-8-sig'))}
meta = {r['episode']: r for r in csv.DictReader(open('kaggle_ours/meta_56553038.csv'))}
d = pd.read_csv('ledger/kaggle_days.csv'); s = pd.read_csv('ledger/kaggle_sales.csv'); g = pd.read_csv('ledger/kaggle_games.csv')
d['ep'] = d.trace.str.extract(r'(\d+)\.txt$')[0]; s['ep'] = s.trace.str.extract(r'(\d+)\.txt$')[0]; g['ep'] = g.trace.str.extract(r'(\d+)\.txt$')[0]
rows = []
for ep, m in meta.items():
    if float(m['own']) >= float(m['opp']): continue
    x = d[d.ep == ep]; ours = x.label.str.startswith('ours')
    o, p = x[ours], x[~ours]
    r = dict(ep=ep, opp=m['opp_team'][:14], rank=lb.get(m['opp_team']), margin=float(m['own']) - float(m['opp']))
    for q in ['melon', 'milk', 'wool', 'egg', 'strawberry', 'fertilizer']:
        r[q[:5]] = o[f'rev_{q}'].sum() - p[f'rev_{q}'].sum()
    r['crops'] = sum(o[f'rev_{q}'].sum() - p[f'rev_{q}'].sum() for q in ['wheat', 'carrot', 'tomato']) - (o.buy_cost_wheat.sum() - p.buy_cost_wheat.sum())
    r['wages'] = -(o.hire_cost.sum() - p.hire_cost.sum()); r['land'] = -(o.land_cost.sum() - p.land_cost.sum())
    r['fmiss'] = int(o.fert_missed.sum())
    sm = s[(s.ep == ep) & (s['product'] == 4) & (s.units > 0)]
    oseat = int(g[(g.ep == ep) & g.label.str.startswith('ours')].seat.iloc[0])
    t = lambda y: (y.day * 24 + y.hour).min()
    fo, fp = t(sm[sm.seat == oseat]), t(sm[sm.seat != oseat])
    r['mel_first'] = f"{int(fo) % 24 if fo == fo else -1}/{int(fp) % 24 if fp == fp else -1}"
    r['mel_n'] = f"{int(o[o.day == 3].plants_melon.iloc[0])}/{int(p[p.day == 3].plants_melon.iloc[0])}"
    h = lambda y: '/'.join(str(int(y[y.day == 12][f'animals_{a}'].iloc[0])) for a in ['goose', 'cow', 'sheep'])
    r['herd12'] = h(o) + ' vs ' + h(p)
    rows.append(r)
t = pd.DataFrame(rows).sort_values('rank')
pd.set_option('display.width', 250)
print(t.to_string(index=False, float_format=lambda v: f'{v:,.0f}'))
num = ['melon', 'milk', 'wool', 'egg', 'straw', 'ferti', 'crops', 'wages', 'land']
print('\nmean over losses:'); print(t[num].mean().round(0).to_string())
print('share of losses where the item is negative:'); print((t[num] < 0).mean().round(2).to_string())
