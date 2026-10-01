"""Exact Local-LB replays (runs/lbrepro: full_games_dc11 traces <dir>/<seed>_<seat>.txt, our seat = <seat>) through Weaknesses' ledger
(work/sep29_validation/tools/ledger_yield, LEDGER_SALES=1). Per game: our and the opponent's revenue and units by day window
(d0-9 / d10-17 / d18-29) and product, sale-hour bands (h0-2 / h3-11 / h12-20 / h21-23), and the herd at dawn d6 / d10 / d14.
Mode 1 (one arm): the base's losses minus its wins (means per game). Mode 2 (two arms, same seeds): arm B minus arm A per game,
margin, flips, and the same splits.
usage: lbsplit.py <out prefix> <arm A>=<trace dir>[,<trace dir>] [<arm B>=<trace dir>[,...]]"""
import glob, os, subprocess, sys
import numpy as np, pandas as pd
LED = '/home/pavel/Programming/kaggriculture/work/sep29_validation/tools/ledger_yield'
P = ['wheat', 'carrot', 'tomato', 'strawberry', 'melon', 'egg', 'milk', 'wool', 'fertilizer']
WIN = [(0, 9, 'd0-9'), (10, 17, 'd10-17'), (18, 29, 'd18-29')]
BAND = [(0, 2, 'h0-2'), (3, 11, 'h3-11'), (12, 20, 'h12-20'), (21, 23, 'h21-23')]
prefix, specs = sys.argv[1], sys.argv[2:]

def ledger(arm, dirs):
    rows = []
    for d in dirs.split(','):
        opp = os.path.basename(d.rstrip('/')).split('_')[-1]
        for t in sorted(glob.glob(f'{d}/*.txt')):
            seed, seat = os.path.basename(t)[:-4].split('_')
            labels = ('us', 'opp') if seat == '0' else ('opp', 'us')
            rows.append(f'{os.path.abspath(t)} {labels[0]} {labels[1]} {opp}|{seed}|{seat}')
    lst = f'{prefix}_{arm}.list'
    open(lst, 'w').write('\n'.join(rows) + '\n')
    subprocess.run([LED, lst, f'{prefix}_{arm}'], check=True, env={**os.environ, 'LEDGER_SALES': '1'}, stdout=subprocess.DEVNULL)
    g = pd.read_csv(f'{prefix}_{arm}_games.csv'); dy = pd.read_csv(f'{prefix}_{arm}_days.csv'); s = pd.read_csv(f'{prefix}_{arm}_sales.csv')
    lab = g[['trace', 'seat', 'label', 'group']]
    g = g[g.label == 'us'].copy(); g['margin'] = g.money - g.opp_money
    g = g.set_index('group')
    feats = {}
    dy = dy.merge(lab[['trace', 'seat', 'group']], on=['trace', 'seat'], suffixes=('', '_g')) if 'group' not in dy else dy
    for who in ['us', 'opp']:
        x = dy[dy.label == who]
        for lo, hi, w in WIN:
            y = x[x.day.between(lo, hi)].groupby('group')
            for p in P:
                feats[f'{who} rev {w} {p}'] = y[f'rev_{p}'].sum()
                feats[f'{who} units {w} {p}'] = y[f'sold_{p}'].sum()
        for day in [6, 10, 14]:
            y = x[x.day == day].set_index('group')
            for a in ['goose', 'cow', 'sheep']: feats[f'{who} herd d{day} {a}'] = y[f'animals_{a}']
    s = s.merge(lab, on=['trace', 'seat'])
    for who in ['us', 'opp']:
        x = s[s.label == who]
        for lo, hi, w in WIN[1:]:
            for p in ['tomato', 'strawberry', 'egg', 'milk', 'wool']:
                y = x[x.day.between(lo, hi) & (x['product'] == P.index(p))]
                tot = y.groupby('group').units.sum()
                for a, b, band in BAND:
                    feats[f'{who} share {w} {p} {band}'] = (y[y.hour.between(a, b)].groupby('group').units.sum().reindex(tot.index).fillna(0) / tot)
    f = pd.DataFrame(feats).reindex(g.index)
    f['margin'], f['own'], f['oppm'] = g.margin, g.money, g.opp_money
    return f

arms = [s.split('=', 1) for s in specs]
F = {a: ledger(a, d) for a, d in arms}
def show(d, title, se_of=None):
    print(title)
    for who in ['us', 'opp']:
        for lo, hi, w in WIN:
            cols = [c for c in d.index if c.startswith(f'{who} rev {w} ')]
            tot = d[cols].sum()
            parts = '  '.join(f'{c.split()[-1]} {d[c]:+.0f} ({d[c.replace(" rev ", " units ")]:+.1f})' for c in cols if abs(d[c]) >= 50)
            print(f'  {who:3s} {w:6s} revenue {tot:+6.0f}: {parts}')
    for who in ['us', 'opp']:
        print(f'  {who:3s} herd ' + '  '.join(f'd{day} g/c/s {d[f"{who} herd d{day} goose"]:+.1f}/{d[f"{who} herd d{day} cow"]:+.1f}/{d[f"{who} herd d{day} sheep"]:+.1f}' for day in [6, 10, 14]))
    for who in ['us', 'opp']:
        for w in ['d10-17', 'd18-29']:
            print(f'  {who:3s} {w} sale-hour share h0-2/h3-11/h12-20/h21-23: ' + '  '.join(
                f'{p} ' + '/'.join(f'{d.get(f"{who} share {w} {p} {b}", np.nan):+.2f}' for _, _, b in BAND) for p in ['strawberry', 'egg', 'milk', 'wool']))
def gap(x, title):  # us minus opponent inside each game (same world), means over the games
    print(title)
    for lo, hi, w in WIN:
        cols = [c for c in x.columns if c.startswith(f'us rev {w} ')]
        d = {c.split()[-1]: (x[c] - x[c.replace('us ', 'opp ', 1)]).mean() for c in cols}
        u = {c.split()[-1]: (x[c.replace(' rev ', ' units ')] - x[c.replace(' rev ', ' units ').replace('us ', 'opp ', 1)]).mean() for c in cols}
        print(f'  {w:6s} us - opp revenue {sum(d.values()):+6.0f}: ' + '  '.join(f'{p} {v:+.0f} ({u[p]:+.1f})' for p, v in d.items() if abs(v) >= 50))
    print('  herd us / opp ' + '  '.join(f'd{day} g/c/s {x[f"us herd d{day} goose"].mean():.1f}/{x[f"us herd d{day} cow"].mean():.1f}/{x[f"us herd d{day} sheep"].mean():.1f}'
                                        f' | {x[f"opp herd d{day} goose"].mean():.1f}/{x[f"opp herd d{day} cow"].mean():.1f}/{x[f"opp herd d{day} sheep"].mean():.1f}' for day in [6, 10, 14]))
    for w in ['d10-17', 'd18-29']:
        print(f'  {w} sale-hour share us | opp h0-2/h3-11/h12-20/h21-23: ' + '  '.join(
            f'{p} ' + '/'.join(f'{x.get(f"us share {w} {p} {b}", pd.Series(dtype=float)).mean():.2f}' for _, _, b in BAND) + ' | ' +
            '/'.join(f'{x.get(f"opp share {w} {p} {b}", pd.Series(dtype=float)).mean():.2f}' for _, _, b in BAND) for p in ['strawberry', 'egg', 'milk', 'wool']))
if len(arms) == 1:
    f = F[arms[0][0]]
    for opp in sorted(set(i.split('|')[0] for i in f.index)) + ['ALL']:
        x = f if opp == 'ALL' else f[[i.startswith(opp + '|') for i in f.index]]
        L, W = x[x.margin < 0], x[x.margin > 0]
        print(f'==== {arms[0][0]} vs {opp}: {len(W)} wins (margin {W.margin.mean():+.0f}, own {W.own.mean():.0f}) / {len(L)} losses (margin '
              f'{L.margin.mean():+.0f}, own {L.own.mean():.0f})')
        gap(L, f' -- losses ({len(L)})')
        gap(W, f' -- wins ({len(W)})')
else:
    (a, _), (b, _) = arms
    A, B = F[a], F[b]
    k = A.index.intersection(B.index)
    for opp in sorted(set(i.split('|')[0] for i in k)) + ['ALL']:
        kk = [i for i in k if opp == 'ALL' or i.startswith(opp + '|')]
        dm = B.loc[kk].margin - A.loc[kk].margin
        flips = f'L->W {((A.loc[kk].margin < 0) & (B.loc[kk].margin > 0)).sum()}, W->L {((A.loc[kk].margin > 0) & (B.loc[kk].margin < 0)).sum()}'
        print(f'==== {b} - {a} vs {opp}: n {len(kk)}, margin {dm.mean():+.0f} (SE {dm.std(ddof=1) / np.sqrt(len(kk)):.0f}), own '
              f'{(B.loc[kk].own - A.loc[kk].own).mean():+.0f}, opp {(B.loc[kk].oppm - A.loc[kk].oppm).mean():+.0f}; wins {(A.loc[kk].margin > 0).sum()} -> '
              f'{(B.loc[kk].margin > 0).sum()}; {flips}')
        show((B.loc[kk] - A.loc[kk]).mean(numeric_only=True), '')
